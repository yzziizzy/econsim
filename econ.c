#include <stdlib.h>
#include <stdio.h>


#include "econ.h"



#define MAX_ACTORS       1024*1024
#define MAX_ITEMS        1024*1024
#define MAX_CASHFLOW     1024*1024
#define MAX_ASSETS       1024*1024
#define MAX_COMMODITIES  1024


#define CLAMP(v, min, max) (v < min ? v : (v > max ? max : v))



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
	
	tickMine(ec);
	tickFactory(ec);
	tickStore(ec);
	tickCompany(ec);
	tickPerson(ec);
	
	// external interaction
	
	if(frandNorm() > .9) {
		entityid_t companyid = Econ_CreateCompany(ec, "Initech");
	}
	
}


#define DEFINE_GET_COMPONENT(type) \
type* Econ_Get##type(Economy*ec, compid_t id) { \
	return &VECMP_ITEM(&(ec->type), id); \
} 

DEFINE_GET_COMPONENT(Entity)
DEFINE_GET_COMPONENT(Movement)
DEFINE_GET_COMPONENT(Position)
DEFINE_GET_COMPONENT(Money)
DEFINE_GET_COMPONENT(PathFollow)



#define NEW_COMPONENT(ec, type, id, e) \
	econid_t id; \
	type* e; \
	VECMP_INC(&(ec)->type); \
	id = VECMP_LAST_INS_INDEX(&(ec)->type); \
	e = &VECMP_ITEM(&(ec)->type, id); \
	memset(e, 0, sizeof(*e));


	

entityid_t Econ_NewEntity(Economy* ec, enum EntityType type, void* userData, char* name) {
	NEW_COMPONENT(ec, Entity, id, e);
	
	e->type = type;
	e->userData = userData;
	e->name = name;
	
	ec->entityCounts[type]++;
	
	return id;
}




void Economy_init(Economy* ec) {
	memset(ec, 0, sizeof(*ec));
	
// 	VECMP_INIT(&ec->cash, MAX_ACTORS);
	VECMP_INIT(&ec->cashflow, MAX_CASHFLOW);
	
// 	VECMP_INIT(&ec->assets, MAX_ASSETS);
	
// 	VECMP_INIT(&ec->actors, MAX_ACTORS);
// 	VECMP_INIT(&ec->cfDescriptions, MAX_CASHFLOW);
	
	VECMP_INIT(&ec->Entity, 16384);
	
	// fill in id 0
	Econ_NewEntity(ec, ET_None, NULL, "Null Entity");
	
	#define X(type, a) VECMP_INIT(&ec->type, 8192);
		ENTITY_TYPE_LIST
	#undef X
	#define X(type, a) VECMP_INIT(&ec->type, 8192);
		COMP_TYPE_LIST
	#undef X

// 	ec->markets = calloc(1, sizeof(*ec->markets) * MAX_COMMODITIES);
// 	for(uint32_t i = 0; i < MAX_COMMODITIES; i++) ec->markets[i].commodity = i;
	
// 	ec->comNames = calloc(1, sizeof(*ec->comNames) * MAX_COMMODITIES);
	
	
	for(int y = 0; y < 64; y++) {
		for(int x = 0; x < 64; x++) {
			entityid_t pcid; // wrong
			Parcel* p;
			pcid = Econ_NewEntParcel(ec, &p);
			
			entityid_t pid = Econ_NewEntity(ec, ET_Parcel, p, "Mapgen Parcel");
			
			p->id = pid; 
			p->loc.x = x;
			p->loc.y = y;
			p->price = 1000;
			p->minerals = 1000000;
			
			ec->parcelGrid[x][y] = pid;
		}
	}
	
	
	
}

void Entity_InitMoney(Economy* ec, entityid_t eid) {
	Entity* e = Econ_GetEntity(ec, eid);
	
	e->money = Econ_NewCompMoney(ec, NULL);
}


entityid_t Econ_CreatePerson(Economy* ec, char* name) {
	Person* p;
	compid_t perid = Econ_NewEntPerson(ec, &p);
	
	entityid_t pid = Econ_NewEntity(ec, ET_Person, p, name);
	
	p->name = name;
	p->id = pid;
	
	Entity_InitMoney(ec, pid);
	
	return pid;
}

entityid_t Econ_CreateCompany(Economy* ec, char* name) {
	Company* c;
	compid_t compid = Econ_NewEntCompany(ec, &c);
	
	entityid_t cid = Econ_NewEntity(ec, ET_Company, c, name);
	
	c->name = name;
	c->id = cid;
	
	Entity_InitMoney(ec, cid);
	
	return cid;
}


entityid_t Econ_CreateMine(Economy* ec, entityid_t parcel_id, char* name) {
	Mine* c;
	compid_t compid = Econ_NewEntMine(ec, &c);
	
	entityid_t cid = Econ_NewEntity(ec, ET_Mine, c, name);
	
	c->parcel = parcel_id;
	c->name = name;
	c->id = cid;
	c->metals = 0;
	
	return cid;
}


entityid_t Econ_CreateFactory(Economy* ec, entityid_t parcel_id, char* name) {
	Factory* c;
	compid_t compid = Econ_NewEntFactory(ec, &c);
	
	entityid_t cid = Econ_NewEntity(ec, ET_Factory, c, name);
	
	c->parcel = parcel_id;
	c->name = name;
	c->id = cid;
	c->widgets = 0;
	
	return cid;
}


entityid_t Econ_CreateStore(Economy* ec, entityid_t parcel_id, char* name) {
	Store* c;
	compid_t compid = Econ_NewEntStore(ec, &c);
	
	entityid_t cid = Econ_NewEntity(ec, ET_Store, c, name);
	
	c->parcel = parcel_id;
	c->name = name;
	c->id = cid;
	c->widgets = 0;
	
	return cid;
}




/*
econid_t Economy_AddActor(Economy* ec, char* name, money_t cash) {
	econid_t id;
	EcActor* ac;
	
	VECMP_INSERT(&ec->cash, cash);
	id = VECMP_LAST_INS_INDEX(&ec->cash);
	char* n = strdup(name);
	
	ac = &VECMP_ITEM(&ec->actors, id);
	ac->id = id;
	ac->name = n;
	
	
	return id;
}
*/


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



// tick functions

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




