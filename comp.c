#include <stdlib.h>
#include <stdio.h>


#include "econ.h"


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



CompDef* Economy_NewCompDef(Economy* ec) {
	CompDef* cd;
	econid_t id;
	
	VECMP_INC(&ec->compDefs); \
	id = VECMP_LAST_INS_INDEX(&ec->compDefs); \
	cd = &VECMP_ITEM(&ec->compDefs, id); \
	cd->id = id;
	
	return cd;
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


