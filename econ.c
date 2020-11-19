#include <stdlib.h>
#include <stdio.h>


#include "econ.h"



#define MAX_ACTORS       1024*1024
#define MAX_ITEMS        1024*1024
#define MAX_CASHFLOW     1024*1024
#define MAX_ASSETS       1024*1024
#define MAX_COMMODITIES  1024


#define CLAMP(v, min, max) (v < min ? v : (v > max ? max : v))



static char* g_CompTypeLookup[] = {
	[CT_NULL] = "CT_NULL",
#define X(x, a, b, c) [CT_##x] = #x,
	COMP_TYPE_LIST
#undef X
	[CT_MAXVALUE] = "CT_MAXVALUE",
};


static char g_CompTypeIsPtr[] = {
	[CT_NULL] = 0,
#define X(x, a, b, c) [CT_##x] = a,
	COMP_TYPE_LIST
#undef X
	[CT_MAXVALUE] = 0,
};

static size_t g_CompTypeSizes[] = {
	[CT_NULL] = 0,
#define X(x, a, b, c) [CT_##x] = sizeof(b),
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
	
	// load the entities themselves
	json_value_t* j_ents = json_obj_get_val(root, "entities");
	if(j_ents) {
		if(j_ents->type != JSON_TYPE_ARRAY) {
			fprintf(stderr, "Invalid entity format\n");
			return 2;
		}
		
		HT_init(&nameLookup, j_ents->v.arr->length);	
		VEC_INIT(&fixes);
		VEC_INIT(&invDefer);
	
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
		
	}
	
	return 0;
}




int CompInternalTypeFromName(char* t) {
	for(int i = 1; i < CT_MAXVALUE; i++) {
		if(0 == strcmp(t, g_CompTypeLookup[i])) return i;
	}

	return 0;
} 

char* CompInternalType_GetName(int compInternalType) {
	return g_CompTypeLookup[compInternalType];
}

CompDef* Econ_GetCompDef(Economy* ec, int compType) {
	return &VECMP_ITEM(&ec->compDefs, compType);
}

CompDef* Econ_GetCompDefName(Economy* ec, char* compName) {
	return Econ_GetCompDef(ec, Econ_CompTypeFromName(ec, compName));
}

int Econ_CompTypeFromName(Economy* ec, char* compName) {
	VECMP_EACH(&ec->compDefs, i, cd) {
		if(0 == strcmp(cd->name, compName)) return cd->id;
	}
	
	return -1;
}


Comp* Entity_SetCompName(Economy* ec, Entity* e, char* compName, ...) {
	int ctype = CompInternalTypeFromName(compName);
	
	va_list va;
	va_start(va, compName);
	Comp* c = Entity_SetComp(ec, e, ctype, va);
	va_end(va);
	
	return c;
}

Comp* Entity_SetComp(Economy* ec, Entity* e, int ctype, ...) {
	va_list va;
	va_start(va, ctype);
	Comp* c = Entity_SetComp(ec, e, ctype, va);
	va_end(va);
	
	return c;
}

Comp* Entity_SetComp_va(Economy* ec, Entity* e, int ctype, va_list va) {
	Comp* c = Entity_AssertComp(e, ctype);
	
	// TODO: support arrays
	CompDef* cd = Econ_GetCompDef(ec, ctype);
	
	if(cd->isPtr && !c->vp) {
		c->vp = calloc(1, InternalCompTypeSize(cd->type));
	}
	
	switch(cd->type) {
		default:
			fprintf(stderr, "unknown component type: %d\n", cd->type);
			exit(1);
			
		case CT_int: c->n = va_arg(va, int64_t); break;
		case CT_float: c->d = va_arg(va, double); break;
		case CT_str: c->str = strdup(va_arg(va, char*)); break;
		case CT_id: c->id = va_arg(va, econid_t); break;
		case CT_itemRate: *c->itemRate = va_arg(va, ItemRate); break;
	}
	
	return c;
} 


int Economy_EntityType(Economy* ec, char* typeName) {
	VECMP_EACH(&ec->entityDefs, i, ed) {
		if(0 == strcmp(typeName, ed->name)) return ed->id;
	}
	
	return -1;
}


Entity* Economy_NewEntityName(Economy* ec, char* typeName, char* name) {
	int type = Economy_EntityType(ec, typeName);
	return Econ_NewEntity(ec, type, name);
}



CompDef* Economy_NewCompDef(Economy* ec) {
	CompDef* cd;
	econid_t id;
	
	VECMP_INC(&ec->compDefs); \
	id = VECMP_LAST_INS_INDEX(&ec->compDefs); \
	cd = &VECMP_ITEM(&ec->compDefs, id); \
	cd->id = id;
	
	return cd;
}

EntityDef* Economy_NewEntityDef(Economy* ec) {
	EntityDef* ed;
	econid_t id;
	
	VECMP_INC(&ec->entityDefs); \
	id = VECMP_LAST_INS_INDEX(&ec->entityDefs); \
	ed = &VECMP_ITEM(&ec->entityDefs, id); \
	ed->id = id;
	
	return ed;
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



	
	
	
Entity* Econ_NewEntity(Economy* ec, int type, char* name) {
	econid_t id;
	Entity* e;
	
	VECMP_INC(&ec->entities);
	id = VECMP_LAST_INS_INDEX(&ec->entities);
	e = &VECMP_ITEM(&ec->entities, id);
	memset(e, 0, sizeof(*e));
	
	e->id = id;
	e->type = type;
	e->name = name;
	e->born = ec->tick;
	
	Inv_Init(&e->inv);
	
	return e;
}


Comp* Entity_GetComp(Entity* e, int compType) {
	VEC_EACHP(&e->comps, i, cp) {
		if(cp->type == compType) return cp;
	}
	
	return NULL;
}

Comp* Entity_GetCompName(Economy* ec, Entity* e, char* compName) {
	int type = Econ_CompTypeFromName(ec, compName);
	if(type < 0) return NULL;
	return Entity_GetComp(e, type);	
}


Comp* Entity_SetCompInt(Entity* e, int compType, int64_t val) {
	
	return NULL;
}

// return value is only valid temporarily
Comp* Entity_AddComp(Entity* e, int compType) {
	Comp* c;
	
	VEC_INC(&e->comps);
	c = &VEC_TAIL(&e->comps);
	
	c->type = compType;
	
	return c;
}

// return value is only valid temporarily
Comp* Entity_AssertComp(Entity* e, int compType) {
	Comp* c;
	
	c = Entity_GetComp(e, compType);
	if(!c) {
		c = Entity_AddComp(e, compType);
	}
	
	return c;
}

size_t InternalCompTypeSize(int internalType) {
	return g_CompTypeSizes[internalType];
}


econid_t Econ_FindItem(Economy* ec, char* name) {
	int etype = Economy_EntityType(ec, "Item");
	
	VECMP_EACH(&ec->entities, i, e) {
		if(e->type != etype) continue;
		
		Comp* c = Entity_GetCompName(ec, e, "name");
		if(c && 0 == strcmp(c->str, name)) {
			return e->id;
		}
	}
	
	return 0;
}



void Economy_init(Economy* ec) {
	memset(ec, 0, sizeof(*ec));
	
// 	VECMP_INIT(&ec->cash, MAX_ACTORS);
	VECMP_INIT(&ec->cashflow, MAX_CASHFLOW);
	
// 	VECMP_INIT(&ec->assets, MAX_ASSETS);
	
// 	VECMP_INIT(&ec->actors, MAX_ACTORS);
// 	VECMP_INIT(&ec->cfDescriptions, MAX_CASHFLOW);
	
	VECMP_INIT(&ec->entities, 16384);
	VECMP_INIT(&ec->compDefs, 16384);
	VECMP_INIT(&ec->entityDefs, 16384);
	
	Economy_LoadConfig(ec, "defs.json");
	
	// fill in id 0
	Econ_NewEntity(ec, 0, "Null Entity");
	
	
}


void Inv_Init(Inventory* inv) {
	VECMP_INIT(&inv->items, 1024); // max types of items in inv 
}


InvItem* Inv_GetItemP(Inventory* inv, econid_t id) {
	VECMP_EACH(&inv->items, i, item) {
		if(item->item == id) return item;
	}
	return NULL;
}

InvItem* Inv_AssertItemP(Inventory* inv, econid_t id) {
	VECMP_EACH(&inv->items, i, item) {
		if(item->item == id) return item;
	}
	
	VECMP_INSERT(&inv->items, ((InvItem){id, 0}));
	
	return VECMP_LAST_INSERT(&inv->items);
}


InvItem* Inv_AddItem(Inventory* inv, econid_t id, long count) {
	InvItem* item = Inv_GetItemP(inv, id);
	if(!item) {
		if(count < 0) count = 0;
		VECMP_INSERT(&inv->items, ((InvItem){id, count}));
	}
	else {
		item->count += count;
		if(item->count < 0) item->count = 0;
	}
	
	return VECMP_LAST_INSERT(&inv->items);
}







econid_t Economy_AddCashflow(
	Economy* ec, 
	money_t amount, 
	econid_t from, 
	econid_t to, 
	uint32_t freq, 
	char* desc
) {
	econid_t id;
	
	VECMP_INSERT(&ec->cashflow, ((EcCashflow){
		.from = from,
		.to = to,
		.amount = amount,
		.frequency = freq
	}));
	
	id = VECMP_LAST_INS_INDEX(&ec->cashflow);
	
// 	VECMP_ITEM(&ec->cfDescriptions, id) = strdup(desc);
	
	
	return id;
}


/*
econid_t Economy_AddAsset(Economy* ec, EcAsset* ass) {
	econid_t id;
	
	VECMP_INSERT(&ec->assets, *ass);
	ass = VECMP_LAST_INSERT(&ec->assets);
	id = VECMP_LAST_INS_INDEX(&ec->cash);
	ass->id = id;
	
	return id;
}


int Economy_BuyAsset(Economy* ec, EcAsset* ass, EcActor* buyer, money_t price) {
	
	money_t* bal = &VEC_ITEM(&ec->cash, buyer->id);
	if(*bal < price) {
		// TODO: log fault
		return 1;
	}
	
	ass->purchasePrice = price;
	ass->curEstValue = price;
	ass->buyNowPrice = price * 2;
	ass->owner = buyer->id;
	
	*bal -= price;
	
	return 0;
}

*/


/*
entityid_t buyLand(Economy* ec, entityid_t buyer, Parcel** parOut) {
	Entity* bent =  Econ_GetEntity(ec, buyer);
	Money* m =  Econ_GetCompMoney(ec, bent->money);
	
	VECMP_EACH(&ec->Parcel, pi, p) {
		if(p->owner) continue;
		
		if(1 || m->cash > p->price) {
			
			m->cash -= p->price;
			p->owner = buyer;
			
			if(parOut) *parOut = p;
			return p->id;
		}
	}
	
	return 0;
}

*/

// tick functions
/*
void doMine(Economy* ec, Mine* m) {
	Entity* pent = Econ_GetEntity(ec, m->parcel);
	Parcel* p = pent->userData;
	
	p->minerals -= 10;
	m->metals += 3;
	Econ_CommodityExNihilo(ec, CT_Ore, -10);
	Econ_CommodityExNihilo(ec, CT_Metal, 3);

	
	if(m->metals > 10) {
		m->metals -= 10;
		
		Entity* cent = Econ_GetEntity(ec, m->owner);
		Company* c = cent->userData;
		
		c->metalsInTransit += 10;
	}
}





entityid_t buildMine(Economy* ec, Company* c, Parcel* p) {
	entityid_t mineID = Econ_CreateMine(ec, p->id, "Smaller Hole");
	Mine* mine = Econ_GetEntity(ec, mineID)->userData;
	
	mine->owner = c->id;
	p->structure = mineID;
	
	return mineID;
}

entityid_t buildFactory(Economy* ec, Company* c, Parcel* p) {
	entityid_t fid = Econ_CreateFactory(ec, p->id, "Smog Mill");
	Factory* f = Econ_GetEntity(ec, fid)->userData;
	
	f->owner = c->id;
	p->structure = fid;
	
	return fid;
}


entityid_t buildStore(Economy* ec, Company* c, Parcel* p) {
	entityid_t fid = Econ_CreateStore(ec, p->id, "SMart");
	Store* f = Econ_GetEntity(ec, fid)->userData;
	
	f->owner = c->id;
	p->structure = fid;
	
	return fid;
}



void doCompany(Economy* ec, Company* c) {
	Entity* cent = Econ_GetEntity(ec, c->id);
	Money* money = Econ_GetCompMoney(ec, cent->money);
	
	
	char hasMine = 1;
	char hasFactory = 1;
	char hasStore = 1;
	Parcel* emptyParcel = NULL;
	
	if(VEC_LEN(&c->mines) == 0) hasMine = 0; 
	if(VEC_LEN(&c->factories) == 0) hasFactory = 0; 
	if(VEC_LEN(&c->stores) == 0) hasStore = 0; 
	
	VEC_EACH(&c->parcels, pi, pid) {
		Entity* pent = Econ_GetEntity(ec, pid);
		Parcel* p = pent->userData;
		
		if(p->structure == 0) {
			emptyParcel = p;
			break;
		}
	}
	
	
	if((!hasMine || !hasFactory || !hasStore) && emptyParcel == NULL) {
		Parcel* par;
		entityid_t parid = buyLand(ec, c->id, &par);
		
		VEC_PUSH(&c->parcels, parid);
		
		return;
	}
	
	if(!hasMine) {
		entityid_t mid = buildMine(ec, c, emptyParcel);
		VEC_PUSH(&c->mines, mid);
		return;
	}
	
	if(!hasFactory) {
		entityid_t mid = buildFactory(ec, c, emptyParcel);
		VEC_PUSH(&c->factories, mid);
		return;
	}
	
	if(!hasStore) {
		entityid_t mid = buildStore(ec, c, emptyParcel);
		VEC_PUSH(&c->stores, mid);
		return;
	}
	
	
	// at this point we have a factory and a mine and a store
	// move some metal to the factory
	Entity* fent = Econ_GetEntity(ec, VEC_ITEM(&c->factories, 0));
	Entity* sent = Econ_GetEntity(ec, VEC_ITEM(&c->stores, 0));
	Factory* f = fent->userData;
	Factory* s = sent->userData;
	
	if(c->metalsInTransit > 0) {
		f->metals += c->metalsInTransit;
		c->metalsInTransit = 0;
	}
	
	if(c->widgetsInTransit > 0) {
		s->widgets += c->widgetsInTransit;
		c->widgetsInTransit = 0;
	}
	
	
	// check if have mine -> build mine
		// check if have land -> buy land
		// check if have workers -> hire workers
	
	// check if have factory -> build factory
		// check if have land -> buy land
		// check if have workers -> hire workers

	// check if have store -> build store
		// check if have land -> buy land
		// check if have workers -> hire workers
	
	// create products
		// transport metals to factory
		// transport products to store
		// produce items
	
	// sell products
	// ???
	// profit!!!
	
}


void doPerson(Economy* ec, Person* p) {
	// find job
	// find house
	// buy widgets
}


void doHouse(Economy* ec, House* p) {
	// random maintenance events
}

void doStore(Economy* ec, Store* s) {
	if(s->widgets > 0) {
		s->widgets--;
		s->sales += 30;
	}
	
}


void doFactory(Economy* ec, Factory* f) {
	if(f->metals > 10) {
		f->metals -= 10;
		f->widgets++;
		Econ_CommodityExNihilo(ec, CT_Widget, 1);
		Econ_CommodityExNihilo(ec, CT_Metal, -10);
	}
	
	if(f->widgets >= 10) {
		Entity* cent = Econ_GetEntity(ec, f->owner);
		Company* c = cent->userData;
		
		c->widgetsInTransit += 10;
	}
	
}

void doParcel(Economy* ec, Parcel* p) {}
void doFarm(Economy* ec, Farm* f) {}


// tick loop functions

#define X(type, needsDoFn) void tick##type(Economy* ec) { \
	VECMP_EACH(&ec->type, ci, c) { \
		do##type(ec, c); \
	} \
}
	ENTITY_TYPE_LIST
#undef X

#define X(type, a) compid_t Econ_NewComp##type(Economy* ec, type** out) { \
	compid_t id; \
	type* e; \
	VECMP_INC(&(ec)->type); \
	id = VECMP_LAST_INS_INDEX(&ec->type); \
	e = &VECMP_ITEM(&ec->type, id); \
	memset(e, 0, sizeof(*e)); \
	if(out) *out = e; \
	\
	return id; \
}
	COMP_TYPE_LIST
#undef X


#define X(type, a) compid_t Econ_NewEnt##type(Economy* ec, type** out) { \
	compid_t id; \
	type* e; \
	VECMP_INC(&(ec)->type); \
	id = VECMP_LAST_INS_INDEX(&ec->type); \
	e = &VECMP_ITEM(&ec->type, id); \
	memset(e, 0, sizeof(*e)); \
	if(out) *out = e; \
	\
	return id; \
}
	ENTITY_TYPE_LIST
#undef X


#define X(type, a) type* Econ_GetEnt##type(Economy* ec, compid_t id) { \
	type* e; \
	e = &VECMP_ITEM(&(ec->type), id); \
	return e; \
}
	ENTITY_TYPE_LIST
#undef X


#define X(type, a) type* Econ_GetComp##type(Economy* ec, compid_t id) { \
	type* e; \
	e = &VECMP_ITEM(&(ec->type), id); \
	return e; \
}
	COMP_TYPE_LIST
#undef X





static size_t CommoditySet_find_index(CommoditySet* cs, commodityid_t comm) {
	ptrdiff_t R = cs->fill - 1;
	ptrdiff_t L = 0;
	ptrdiff_t i;
	
	while(R - L > 0) {
		
		// midpoint
		i = L + ((R - L) / 2);
		if(cs->buckets[i].comm < comm) {
			L = i + 1;
		}
		else if(cs->buckets[i].comm > comm) {
			R = i - 1;
		}
		else {
			return i;
		}
	}
	
	return (cs->buckets[L].comm < comm) ? L + 1 : L;
} 



CommoditySet* CommoditySet_New(int size) {
	CommoditySet* cs = calloc(1, sizeof(*cs) + sizeof(cs->buckets[0]) * size);
	cs->length = size;
	return cs;
}


// must never be given a null pointer
// returns zero for success, 1 if the set could not accept more
int CommoditySet_Add(CommoditySet* cs, commodityid_t comm, qty_t qty, int compact) {
		// find the slot
	size_t i = CommoditySet_find_index(cs, comm);
	if(cs->buckets[i].comm == comm) {
		cs->buckets[i].qty += qty; // TODO: check saturation
		
		// purge zero-value items, if desired
		if(compact && cs->buckets[i].qty == 0) {
			memmove(&cs->buckets[i], &cs->buckets[i + 1], (cs->fill - i - 1) * sizeof(cs->buckets[0]));
			cs->fill--;
		}
		
		return 0;
	}
	
	// check for room
	if(cs->fill >= cs->length) return 1; // overflow
	
	memmove(&cs->buckets[i + 1], &cs->buckets[i], (cs->fill - i) * sizeof(cs->buckets[0]));
	cs->buckets[i].comm = comm;
	cs->buckets[i].qty = qty;
	cs->fill++;
	
	return 0;
}


// must never be given a null pointer
qty_t CommoditySet_Get(CommoditySet* cs, commodityid_t comm) {
	// find the slot
	size_t i = CommoditySet_find_index(cs, comm);
	if(cs->buckets[i].comm == comm) {
		return cs->buckets[i].qty;
	}
	
	return 0;
}



void Econ_CommodityExNihilo(Economy* ec, commodityid_t comm, qty_t qty) {
	ec->commodityTotals[comm] += qty;
}


*/

