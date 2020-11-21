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
		fprintf(stderr, "File not found: '%s'\n", path);
		return 1;
	}
	
	int ret = Economy_LoadConfigJSON(ec, jsf->root);
	
	json_file_free(jsf);
	
	return ret;
}


int Economy_LoadConfigJSON(Economy* ec, json_value_t* root) {
	
	if(root->type != JSON_TYPE_OBJ) {
		fprintf(stderr, "Invalid config format\n");
		return 1;
	}
	
	
	// component definitions
	json_value_t* j_cdefs = json_obj_get_val(root, "component_defs");
	if(j_cdefs) {
		if(j_cdefs->type != JSON_TYPE_ARRAY) {
			fprintf(stderr, "Invalid component_defs format\n");
			return 2;
		}
	
		json_array_node_t* link = j_cdefs->v.arr->head;
		while(link) {
			// parse a component definition
			CompDef* cd = Economy_NewCompDef(ec);
			
			cd->name = strdup(json_obj_get_string(link->value, "name"));			
			
			int off = 0;
			char* typestr = json_obj_get_string(link->value, "type");
			if(typestr[0] == 0) {
				fprintf(stderr, "Invalid component type\n");
				return 3;
			}
			
			if(typestr[0] == '^') {
				cd->isArray = 1;
				off = 1;
			}
			
			cd->type = CompInternalTypeFromName(typestr + off);
			if(cd->type == 0) {
				fprintf(stderr, "Invalid component internal type: %s\n", typestr);
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
			fprintf(stderr, "Invalid entity_defs format\n");
			return 2;
		}
	
		json_array_node_t* link = j_edefs->v.arr->head;
		while(link) {
			// parse a component definition
			EntityDef* ed = Economy_NewEntityDef(ec);
			
			ed->name = strdup(json_obj_get_string(link->value, "name"));
						
			link = link->next;
		}
		
		
	}
	
	
	// entity id references are done through string aliases
	// a lookup table of actual id's is used by a list of
	//   reference locations to match strings to id's 
	//   without regard to declaration order 
	HT(econid_t) nameLookup;
	VEC(struct fixes {char* name; econid_t* target;}) fixes;
	VEC(struct invDefer {Inventory* inv; char* name; long count;}) invDefer;
	VEC(struct convDefer {char* name; Conversion** target;}) convDefer;
	
	HT_init(&nameLookup, 1024);	
	VEC_INIT(&fixes);
	VEC_INIT(&invDefer);
	
	
	// load the entities themselves
	json_value_t* j_ents = json_obj_get_val(root, "entities");
	if(j_ents) {
		if(j_ents->type != JSON_TYPE_ARRAY) {
			fprintf(stderr, "Invalid entity format\n");
			return 2;
		}
	
		json_array_node_t* link = j_ents->v.arr->head;
		while(link) {
			json_value_t* j_ent = link->value;
			
			// read the entity type and create it
			char* typeName = json_obj_get_string(j_ent, "type");
			Entity* e = Economy_NewEntityName(ec, typeName, "");
			
			// put the id reference string in the lookup for later
			char* idString = json_obj_get_string(j_ent, "id");
			if(idString) {
				HT_set(&nameLookup, idString, e->id);
			}
			
			// read the component values
			json_value_t* j_comps = json_obj_get_val(j_ent, "comps");
			json_array_node_t* clink = j_comps->v.arr->head;
			while(clink) {
				json_value_t* j_comp = clink->value;
				
				// get the component name and type, then create it
				char* compName;
				json_as_string(j_comp->v.arr->head->value, &compName);
				CompDef* cd = Econ_GetCompDefName(ec, compName);
				Comp* c = Entity_AssertComp(e, cd->id);
				
				if(cd->isPtr) {
					c->vp = calloc(1, InternalCompTypeSize(cd->type));
				}
								
				// read and set the component value
				json_value_t* j_cval = j_comp->v.arr->head->next->value;
							
				switch(cd->type) {
					default:
						fprintf(stderr, "unknown component type: %d\n", cd->type);
						exit(1);
					
					case CT_float: json_as_double(j_cval, &c->d); break;
					
					case CT_int: json_as_int(j_cval, &c->n); break;
					
					case CT_str: c->str = strdup(j_cval->v.str); break;
					
					case CT_id: 
						VEC_PUSH(&fixes, ((struct fixes){j_cval->v.str, &c->id}));
						break;
					case CT_itemRate:
						VEC_PUSH(&fixes, ((struct fixes){j_cval->v.str, &c->itemRate->item}));
						json_as_float(j_cval->v.arr->head->next->value, &c->itemRate->rate);
						break;
					
					case CT_conversion:
						
						break;	
				}
				
				
				clink = clink->next;
			}
			
			
			// read the inventory
			json_value_t* j_inv = json_obj_get_val(j_ent, "inv");
			if(j_inv) {
				json_array_node_t* ilink = j_inv->v.arr->head;
				while(ilink) {
					json_value_t* j_item = ilink->value;
					
					// get the component name and type, then create it
					char* itemName;
					long count;
					json_as_string(j_item->v.arr->head->value, &itemName);
					json_as_int(j_item->v.arr->head->next->value, &count);
					
					VEC_PUSH(&invDefer, ((struct invDefer){&e->inv, itemName, count}));
					
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
			fprintf(stderr, "Invalid conversion format\n");
			return 2;
		}
	
		json_array_node_t* link = j_convs->v.arr->head;
		while(link) {
			json_value_t* j_conv = link->value;
		
			// get the component name and type, then create it
			Conversion* c = Econ_NewConversion(ec);
			c->name = json_obj_get_string(j_conv, "name");
			
			char* idString = json_obj_get_string(j_conv, "id");
			if(idString) {
				HT_set(&nameLookup, idString, c->id);
			}
			
			// add the inputs
			json_value_t* j_ins = json_obj_get_val(j_conv, "input");
			if(!j_ins || j_ins->type != JSON_TYPE_ARRAY || j_ins->v.arr->length == 0) {
				fprintf(stderr, "Conversion with invalid input\n");
				return 5;
			}
			
			c->inputCnt = j_ins->v.arr->length;
			c->inputs = calloc(1, sizeof(*c->inputs) * c->inputCnt);
						
			int n = 0;
			json_array_node_t* ilink = j_ins->v.arr->head;
			while(ilink) {
				json_value_t* v = ilink->value;
				
				char* idString = v->v.arr->head->value->v.str;
				VEC_PUSH(&fixes, ((struct fixes){idString, &c->inputs[n].id}));
				json_as_int(v->v.arr->tail->value, &c->inputs[n].count);
				
				n++;
				ilink = ilink->next;
			}
			
			
			// add the outputs
			json_value_t* j_outs = json_obj_get_val(j_conv, "output");
			if(!j_outs || j_outs->type != JSON_TYPE_ARRAY || j_outs->v.arr->length == 0) {
				fprintf(stderr, "Conversion with invalid output\n");
				return 5;
			}			
			
			c->outputCnt = j_outs->v.arr->length;
			c->outputs = calloc(1, sizeof(*c->outputs) * c->outputCnt);
						
			n = 0;
			ilink = j_outs->v.arr->head;
			while(ilink) {
				json_value_t* v = ilink->value;
				
				char* idString = v->v.arr->head->value->v.str;
				VEC_PUSH(&fixes, ((struct fixes){idString, &c->outputs[n].id}));
				json_as_int(v->v.arr->tail->value, &c->outputs[n].count);
				
				n++;
				ilink = ilink->next;
			}
			
		
			link = link->next;	
		}
	}
		
	
	// fix all the id string references
	VEC_EACH(&fixes, i, fix) {
		econid_t id;
		if(HT_get(&nameLookup, fix.name, &id)) {
			fprintf(stderr, "Unknown entity reference: '%s'\n", fix.name);
			continue;
		}
		
		*fix.target = id;
	}
	
	VEC_EACH(&invDefer, i, defer) {
		econid_t id;
		if(HT_get(&nameLookup, defer.name, &id)) {
			fprintf(stderr, "Unknown entity reference: '%s'\n", defer.name);
			continue;
		}
		
		Inv_AddItem(defer.inv, id, defer.count);
	}
	
	
	VEC_FREE(&invDefer);
	VEC_FREE(&fixes);
	HT_destroy(&nameLookup);

	return 0;
}



void Economy_tick(Economy* ec) {
	
	/*
	// process cashflows
	VECMP_EACH(&ec->cashflow, ind, cf) {
		// TODO: handle overflow
		if(cf->from == ECON_MAGIC) {
			VECMP_ITEM(&ec->cash, cf->to) += cf->amount;
		}
		else if(cf->to == ECON_MAGIC) {
			VECMP_ITEM(&ec->cash, cf->from) -= cf->amount;
		}
		else {
			money_t tmp = VECMP_ITEM(&ec->cash, cf->from);
			tmp = CLAMP(cf->amount, tmp, ECON_CASHMAX);
			VECMP_ITEM(&ec->cash, cf->to) += tmp;
			VECMP_ITEM(&ec->cash, cf->from) -= tmp;
			
			// TODO: trigger bankruptcy 
		}
		
	} 
	
	*/
	
	// transport should tick first or last
/*	
	tickMine(ec);
	tickFactory(ec);
	tickStore(ec);
	tickCompany(ec);
	tickPerson(ec);
	*/
	// external interaction
	/*
	if(frandNorm() > .9) {
		entityid_t companyid = Econ_CreateCompany(ec, "Initech");
	}
	*/
}



	

void Economy_init(Economy* ec) {
	memset(ec, 0, sizeof(*ec));
	
// 	VECMP_INIT(&ec->cash, MAX_ACTORS);
	VECMP_INIT(&ec->cashflow, MAX_CASHFLOW);
	
// 	VECMP_INIT(&ec->assets, MAX_ASSETS);
	
// 	VECMP_INIT(&ec->actors, MAX_ACTORS);
// 	VECMP_INIT(&ec->cfDescriptions, MAX_CASHFLOW);
	
	VECMP_INIT(&ec->entities, 16384);
	VECMP_INIT(&ec->conversions, 16384);
	VECMP_INIT(&ec->compDefs, 16384);
	VECMP_INIT(&ec->entityDefs, 16384);
	
	Economy_LoadConfig(ec, "defs.json");
	
	// fill in id 0
	Econ_NewEntity(ec, 0, "Null Entity");
	
	
}



