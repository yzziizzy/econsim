#include <stdlib.h>
#include <stdio.h>


#include "econ.h"


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


EntityDef* Economy_NewEntityDef(Economy* ec) {
	EntityDef* ed;
	econid_t id;
	
	VECMP_INC(&ec->entityDefs); \
	id = VECMP_LAST_INS_INDEX(&ec->entityDefs); \
	ed = &VECMP_ITEM(&ec->entityDefs, id); \
	ed->id = id;
	
	return ed;
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
	
//	Inv_Init(&e->inv);
	
	return e;
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




InvItem* Entity_InvAddItem(Entity* e, econid_t id, long count) {
	if(!e->inv) {
		e->inv = Inv_New();
	}
	return Inv_AddItem(e->inv, id, count);
}


Inventory* Inv_New() {
	Inventory* inv = calloc(1, sizeof(*inv));
	
	Inv_Init(inv);
	
	return inv;
}

void Inv_Init(Inventory* inv) {
	VECMP_INIT(&inv->items, 1024); // max types of items in inv 
}


InvItem* Inv_GetItemP(Inventory* inv, econid_t id) {
	if(!inv) return NULL;
	
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





