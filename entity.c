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
	if(type < 0) return NULL;
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


EntityDef* Economy_GetEntityDef(Economy* ec, int typeid) {
	VECMP_EACH(&ec->entityDefs, i, ed) {
		if(typeid == ed->id) return ed;
	}
	
	return NULL;
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

Entity* Econ_GetEntity(Economy* ec, econid_t id) {
	return &VECMP_ITEM(&ec->entities, id);
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
	VEC_INIT(&inv->escrow);
}

void Inv_Destroy(Inventory* inv) {
	VECMP_FREE(&inv->items); 
	VEC_FREE(&inv->escrow);
}


InvItem* Inv_GetItemP(Inventory* inv, econid_t id) {
	if(!inv) return NULL;
	
	VECMP_EACH(&inv->items, i, item) {
		if(item->item == id) return item;
	}
	return NULL;
}

EscrowItem* Inv_GetEscrowItemP(Inventory* inv, econid_t owner, econid_t id) {
	if(!inv) return NULL;
	
	VEC_EACHP(&inv->escrow, i, item) {
		if(item->item == id && item->owner == owner) return item;
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

EscrowItem* Inv_AssertEscrowItemP(Inventory* inv, econid_t owner, econid_t id) {
	VEC_EACHP(&inv->escrow, i, item) {
		if(item->item == id && item->owner == owner) return item;
	}
	
	VEC_PUSH(&inv->escrow, ((EscrowItem){owner, id, 0}));
	
	return &VEC_TAIL(&inv->escrow);
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


EscrowItem* Inv_AddEscrowItem(Inventory* inv, econid_t owner, econid_t id, long count) {
	EscrowItem* item = Inv_GetEscrowItemP(inv, owner, id);
	if(!item) {
		if(count < 0) count = 0;
		VEC_PUSH(&inv->escrow, ((EscrowItem){owner, id, count}));
	}
	else {
		item->count += count;
		if(item->count < 0) item->count = 0;
	}
	
	return &VEC_TAIL(&inv->escrow);
}

// returns number of items moved into escrow
long Inv_MoveToEscrow(Inventory* inv, econid_t newOwner, econid_t item, long count) {
	EscrowItem* es = Inv_AssertEscrowItemP(inv, newOwner, item);
	InvItem* it = Inv_GetItemP(inv, item);
	
	if(!it || it->count <= 0) return 0;
	
	long toMove = MIN(count, it->count);
	it->count -= toMove;
	es->count += toMove;
	
	return toMove;
}

// returns the number of items changed
long Inv_EscrowChangeOwner(Inventory* inv, econid_t oldOwner, econid_t newOwner, econid_t item, long count) {
	EscrowItem* o = Inv_AssertEscrowItemP(inv, oldOwner, item);
	if(o->count == 0) return 0;
	
	EscrowItem* n = Inv_AssertEscrowItemP(inv, newOwner, item);
	
	long toChange = MIN(o->count, count);
	o->count -= toChange;
	n->count += toChange;
	
	return toChange;
}



void Entity_FuseInventories(Entity* e_core, Entity* e_extra) {
	Inventory* invc = e_core->inv;
	Inventory* inve = e_extra->inv;
	
	// simple cases where one or both inv's are NULL
	if(!invc && !inve) {
		invc = Inv_New();
		e_core->inv = invc;
		e_extra->inv = invc;
		return;
	}
	
	if(!invc) {
		e_core->inv = inve;
		return;
	}
	if(!inve) {
		e_extra->inv = invc;
		return;
	}
	
	// combine contents
	VECMP_EACH(&inve->items, i, it) {
		Inv_AddItem(invc, it->item, it->count);
	}
	VEC_EACHP(&inve->escrow, i, eit) {
		Inv_AddEscrowItem(invc,  eit->owner, eit->item, eit->count);
	}
	
	Inv_Destroy(inve);
	free(inve);
	
	e_extra->inv = invc;
}


