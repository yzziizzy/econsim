#include <stdlib.h>
#include <stdio.h>


#include "econ.h"



#define MAX_ACTORS       1024*1024
#define MAX_ITEMS        1024*1024
#define MAX_CASHFLOW     1024*1024
#define MAX_ASSETS       1024*1024
#define MAX_COMMODITIES  1024


#define CLAMP(v, min, max) (v < min ? v : (v > max ? max : v))



static char g_CompTypeIsPtr[] = {
	[CT_NULL] = 0,
#define X(x, a, b, c) [CT_##x] = a,
	COMP_TYPE_LIST
#undef X
	[CT_MAXVALUE] = 0,
};




int Economy_LoadConfig(Economy* ec, char* path) {
	json_file_t* jsf = json_load_path(path);
	if(!jsf) {
		LOG("File not found: '%s'\n", path);
		return 1;
	}
	
	int ret = Economy_LoadConfigJSON(ec, jsf->root);
	
	json_file_free(jsf);
	
	return ret;
}


int Economy_LoadConfigJSON(Economy* ec, json_value_t* root) {
	
	if(root->type != JSON_TYPE_OBJ) {
		LOG("Invalid config format\n");
		return 1;
	}
	

	// entity id references are done through string aliases
	// a lookup table of actual id's is used by a list of
	//   reference locations to match strings to id's 
	//   without regard to declaration order 
	HT(econid_t) nameLookup;
	HT(Conversion*) conversionLookup;
	VEC(struct fixes {char* name; econid_t* target;}) fixes;
	VEC(struct invDefer {Inventory* inv; char* name; long count;}) invDefer;
	VEC(struct convDefer {char* name; Conversion** target;}) convDefer;
	
	HT_init(&nameLookup, 1024);	
	HT_init(&conversionLookup, 1024);	
	VEC_INIT(&fixes);
	VEC_INIT(&invDefer);
	VEC_INIT(&convDefer);

	
	// component definitions
	json_value_t* j_cdefs = json_obj_get_val(root, "component_defs");
	if(j_cdefs) {
		if(j_cdefs->type != JSON_TYPE_ARRAY) {
			LOG("Invalid component_defs format\n");
			return 2;
		}
	
		json_link_t* link = j_cdefs->arr.head;
		while(link) {
			// parse a component definition
			CompDef* cd = Economy_NewCompDef(ec);
			
			cd->name = strdup(json_obj_get_str(link->v, "name"));			
			
			int off = 0;
			char* typestr = json_obj_get_str(link->v, "type");
			if(typestr[0] == 0) {
				LOG("Invalid component type\n");
				return 3;
			}
			
			if(typestr[0] == '^') {
				cd->isArray = 1;
				off = 1;
			}
			
			cd->type = CompInternalTypeFromName(typestr + off);
			if(cd->type == 0) {
				LOG("Invalid component internal type: %s\n", typestr);
				return 4;
			} 
			
			cd->isPtr = g_CompTypeIsPtr[cd->type];
						
			link = link->next;
		}
		
		
	}
	
	// entity definitions
	json_value_t* j_edefs = json_obj_get_val(root, "entity_defs");
	if(j_edefs) {
		if(j_edefs->type != JSON_TYPE_ARRAY) {
			LOG("Invalid entity_defs format\n");
			return 2;
		}
	
		json_link_t* link = j_edefs->arr.head;
		while(link) {
			// parse a component definition
			EntityDef* ed = Economy_NewEntityDef(ec);
			
			ed->name = strdup(json_obj_get_str(link->v, "name"));
			
			char* fuseName = json_obj_get_str(link->v, "fusedInv");
			if(fuseName) ed->fusedInv = Econ_CompTypeFromName(ec, fuseName);
			
//			LOG(" + %s", fuseName);
//			if(fuseName) {
//				VEC_PUSH(&fixes, ((struct fixes){fuseName, &ed->fusedInv}));
//			}
			
			link = link->next;
		}
		
		
	}
	
	
	
	
	// load the entities themselves
	json_value_t* j_ents = json_obj_get_val(root, "entities");
	if(j_ents) {
		if(j_ents->type != JSON_TYPE_ARRAY) {
			LOG("Invalid entity format");
			return 2;
		}
	
		json_link_t* link = j_ents->arr.head;
		while(link) {
			json_value_t* j_ent = link->v;
			
			// read the entity type and create it
			char* typeName = json_obj_get_str(j_ent, "type");
			Entity* e = Economy_NewEntityName(ec, typeName, "");
			if(!e) {
				LOG("Failed to create entity type %s", typeName);
				exit(3);
			}
			
			// put the id reference string in the lookup for later
			char* idString = json_obj_get_str(j_ent, "id");
			if(idString) {
				HT_set(&nameLookup, idString, e->id);
			}
			
			// read the component values
			json_value_t* j_comps = json_obj_get_val(j_ent, "comps");
			json_link_t* clink = j_comps->arr.head;
			while(clink) {
				json_value_t* j_comp = clink->v;
				
				// get the component name and type, then create it
				char* compName = j_comp->arr.head->v->s;
				CompDef* cd = Econ_GetCompDefName(ec, compName);
				Comp* c = Entity_AssertComp(e, cd->id);
				
				if(cd->isPtr) {
					c->vp = calloc(1, InternalCompTypeSize(cd->type));
				}
								
				// read and set the component value
				json_value_t* j_cval = j_comp->arr.head->next->v;
							
				switch(cd->type) {
					default:
						LOG("unknown component type: %d", cd->type);
						exit(1);
					
					case CT_float: c->d = json_as_double(j_cval); break;
					
					case CT_int: c->n = json_as_int(j_cval); break;
					
					case CT_str: c->str = strdup(j_cval->s); break;
					
					case CT_id: 
						VEC_PUSH(&fixes, ((struct fixes){j_cval->s, &c->id}));
						break;
						
					case CT_itemRate:
						LOG("Deferring '%s' at line %d", j_cval->arr.head->v->s, __LINE__);
						VEC_PUSH(&fixes, ((struct fixes){j_cval->arr.head->v->s, &c->itemRate->item}));
						c->itemRate->rate = json_as_float(j_cval->arr.head->next->v);
						break;
					
					case CT_itemPrice:
						LOG("Deferring '%s' at line %d", j_cval->arr.head->v->s, __LINE__);
						VEC_PUSH(&fixes, ((struct fixes){j_cval->arr.head->v->s, &c->itemPrice->item}));
						c->itemPrice->price = json_as_int(j_cval->arr.head->next->v);
						break;
					
					case CT_conversion:
						c->convertRate->acc = 0;
						c->convertRate->rate = json_as_float(j_cval->arr.head->v);
						LOG("Deferring '%s' at line %d", j_cval->arr.head->next->v->s, __LINE__);
						VEC_PUSH(&convDefer, ((struct convDefer){j_cval->arr.head->next->v->s, &c->convertRate->c}));		
						break;
						
					case CT_roadspan:
						c->roadSpan->a.x = json_as_float(j_cval->arr.head->v);
						c->roadSpan->a.y = json_as_float(j_cval->arr.head->next->v);
						c->roadSpan->b.x = json_as_float(j_cval->arr.head->next->next->v);
						c->roadSpan->b.y = json_as_float(j_cval->arr.head->next->next->next->v);
						break;
					
					case CT_roadconnect:
						
						break;	
				}
				
				
				clink = clink->next;
			}
			
			
			// read the inventory
			json_value_t* j_inv = json_obj_get_val(j_ent, "inv");
			if(j_inv) {
				json_link_t* ilink = j_inv->arr.head;
				while(ilink) {
					json_value_t* j_item = ilink->v;
					
					// get the component name and type, then create it
					char* itemName;
					long count;
					itemName = j_item->arr.head->v->s;
					count = json_as_int(j_item->arr.head->next->v);
					
					if(!e->inv) e->inv = Inv_New();
					LOG("Deferring '%s' at line %d", itemName, __LINE__);
					VEC_PUSH(&invDefer, ((struct invDefer){e->inv, itemName, count}));
					
					ilink = ilink->next;
				}
			}	
			
			link = link->next;
		}
		
	}
	
	
	// load conversions
	json_value_t* j_convs = json_obj_get_val(root, "conversions");
	if(j_convs) {
		if(j_convs->type != JSON_TYPE_ARRAY) {
			LOG("Invalid conversion format");
			exit(1);
		}
	
		json_link_t* link = j_convs->arr.head;
		while(link) {
			json_value_t* j_conv = link->v;
		
			// get the component name and type, then create it
			Conversion* c = Econ_NewConversion(ec);
			c->name = json_obj_get_strdup(j_conv, "name");
			
			char* idString = json_obj_get_str(j_conv, "id");
			if(idString) {
				HT_set(&conversionLookup, idString, c);
			}
			
			// add the inputs
			json_value_t* j_ins = json_obj_get_val(j_conv, "input");
			if(!j_ins || j_ins->type != JSON_TYPE_ARRAY || j_ins->len == 0) {
				LOG("Conversion with invalid input");
				exit(1);
			}
			
			c->inputCnt = j_ins->len;
			c->inputs = calloc(1, sizeof(*c->inputs) * c->inputCnt);
						
			int n = 0;
			json_link_t* ilink = j_ins->arr.head;
			while(ilink) {
				json_value_t* v = ilink->v;
				
				char* idString = v->arr.head->v->s;
				LOG("Deferring '%s' at line %d", idString, __LINE__);
				VEC_PUSH(&fixes, ((struct fixes){idString, &c->inputs[n].item}));
				c->inputs[n].count = json_as_int(v->arr.tail->v);
				
				n++;
				ilink = ilink->next;
			}
			
			
			// add the outputs
			json_value_t* j_outs = json_obj_get_val(j_conv, "output");
			if(!j_outs || j_outs->type != JSON_TYPE_ARRAY || j_outs->len == 0) {
				LOG("Conversion with invalid output");
				exit(1);
			}			
			
			c->outputCnt = j_outs->len;
			c->outputs = calloc(1, sizeof(*c->outputs) * c->outputCnt);
						
			n = 0;
			ilink = j_outs->arr.head;
			while(ilink) {
				json_value_t* v = ilink->v;
				
				char* idString = v->arr.head->v->s;
				LOG("Deferring '%s' at line %d", idString, __LINE__);
				VEC_PUSH(&fixes, ((struct fixes){idString, &c->outputs[n].item}));
				c->outputs[n].count = json_as_int(v->arr.tail->v);
				
				n++;
				ilink = ilink->next;
			}
			
		
			link = link->next;	
		}
	}
	
	
	json_value_t* j_market = json_obj_get_val(root, "market");
	if(j_market) {
		
		// market sinks (infinite buyers)
		json_value_t* j_sinks = json_obj_get_val(j_market, "sinks");
		if(j_sinks) {
			json_link_t* link = j_sinks->arr.head;
			while(link) {
				json_value_t* v = link->v;
				
				long price = json_obj_get_int(v, "maxBuyPrice", 0);
				MarketSink* s = Market_AddSink(ec->m, 0, price);
				
				s->name = json_obj_get_strdup(v, "name");
				s->maxBuysPerTick = json_obj_get_int(v, "maxBuysPerTick", -1);
				
				
				char* item = json_obj_get_str(v, "item");
				
				LOG("Deferring '%s' at line %d", item, __LINE__);
				VEC_PUSH(&fixes, ((struct fixes){item, &s->item}));
						
				
				link = link->next;
			}
		}
		
	}
	
		
	
	// fix all the id string references
	VEC_EACH(&fixes, i, fix) {
		econid_t id;
		if(HT_get(&nameLookup, fix.name, &id)) {
			LOG("Unknown entity reference: '%s'", fix.name);
			continue;
		}
		
		*fix.target = id;
	}
	
	VEC_EACH(&invDefer, i, defer) {
		econid_t id;
		if(HT_get(&nameLookup, defer.name, &id)) {
			LOG("Unknown entity reference: '%s'", defer.name);
			continue;
		}
		
		Inv_AddItem(defer.inv, id, defer.count);
	}
	
	VEC_EACH(&convDefer, i, defer) {
		Conversion* c;
		if(HT_get(&conversionLookup, defer.name, &c)) {
			LOG("Unknown conversion reference: '%s'", defer.name);
			continue;
		}
		
		*defer.target = c;
	}
	
	
	// fuse inventories
	VECMP_EACH(&ec->entities, i, e) {
		EntityDef* ed = Economy_GetEntityDef(ec, e->type);
		
		if(ed->fusedInv) {
			Comp* c = Entity_GetComp(e, ed->fusedInv);
			Entity* e_loc = Econ_GetEntity(ec, c->id);
			
			Entity_FuseInventories(e_loc, e);
		}
		
	}
	
	
	
	
	VEC_FREE(&convDefer);
	VEC_FREE(&invDefer);
	VEC_FREE(&fixes);
	HT_destroy(&nameLookup);
	HT_destroy(&conversionLookup);

	return 0;
}



void Economy_tick(Economy* ec) {
	ec->tick++;
	
	int prodtypeid = Econ_CompTypeFromName(ec, "produces");
	int convtypeid = Econ_CompTypeFromName(ec, "converts");
	int sellsid = Econ_CompTypeFromName(ec, "sells");
	
	
	VECMP_EACH(&ec->entities, i, e) {
		Comp* c;
		
		// production
		c = Entity_GetComp(e, prodtypeid);
		if(c) {
			if(++c->itemRate->acc >= c->itemRate->rate) {
				int n = c->itemRate->acc / c->itemRate->rate;
				Entity_InvAddItem(e, c->itemRate->item, n);
				
				c->itemRate->acc -= c->itemRate->rate * n;
			}
		}
		
		// conversion
		c = Entity_GetComp(e, convtypeid);
		if(c) {
			ConvertRate* cr = c->convertRate;
			Conversion* v = cr->c;
			
			if(++cr->acc >= cr->rate) {
				
				int cnt = Conv_MaxAvail(v, e->inv);
				if(cnt > 0) {
					int n = cr->acc / cr->rate;
					n = MIN(n, cnt);
					Conv_DoConversion(v, e->inv, n);
					
					cr->acc -= cr->rate * n;
				}
			}
		}
		
		
		// selling
		c = Entity_GetComp(e, sellsid);
		if(c) {
			InvItem* i = Inv_GetItemP(e->inv, c->itemPrice->item);
			
			if(i)
				Market_AddSellOrder(ec->m, e, i->item, i->count, c->itemPrice->price);
		}
		
		
		
		Market_RunSinks(ec->m);
	}
	
	
	
}



	

void Economy_init(Economy* ec) {
	memset(ec, 0, sizeof(*ec));
	
	ec->m = Market_New();
	
	VECMP_INIT(&ec->entities, 16384);
	VECMP_INIT(&ec->conversions, 16384);
	VECMP_INIT(&ec->compDefs, 16384);
	VECMP_INIT(&ec->entityDefs, 16384);
	
	
	// fill in id 0
	Econ_NewEntity(ec, 0, "Null Entity");
	
	// special entities
	ec->m->sinkEntity = Econ_NewEntity(ec, 0, "Market Sink Entity");
	ec->m->sinkEntity->inv = Inv_New();
	
	
	Economy_LoadConfig(ec, "defs.json");	
}



