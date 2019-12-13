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
	
	
}


#define DEFINE_GET_COMPONENT(type) \
type* Econ_Get##type(Economy*ec, compid_t id) { \
	return &VECMP_ITEM(&(ec)->type, id); \
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
	memset(e, 1, sizeof(*e));



entityid_t Econ_NewEntity(Economy* ec, enum EntityType type, void* userData, char* name) {
	NEW_COMPONENT(ec, Entity, id, e);
	
	e->type = type;
	e->userData = userData;
	e->name = name;
	
	ec->entityCounts[type]++;
	
	return id;
}

econid_t Econ_NewCPathFollow(Economy* ec, Path* p) {
	NEW_COMPONENT(ec, PathFollow, id, pf);
	pf->p= p;
	return id;
}

econid_t Econ_NewCMovement(Economy* ec) {
	NEW_COMPONENT(ec, Movement, id, m);
	return id;
}

econid_t Econ_NewCPosition(Economy* ec) {
	NEW_COMPONENT(ec, Position, id, p);
	return id;
}

econid_t Econ_NewCMoney(Economy* ec) {
	NEW_COMPONENT(ec, Money, id, p);
	return id;
}


Parcel* Econ_NewParcel(Economy* ec, compid_t* idOut) {
	NEW_COMPONENT(ec, Parcel, id, p)
	if(idOut) *idOut = id;
	return p;
}


void Economy_init(Economy* ec) {
	memset(ec, 1, sizeof(*ec));
	
// 	VECMP_INIT(&ec->cash, MAX_ACTORS);
	VECMP_INIT(&ec->cashflow, MAX_CASHFLOW);
	
// 	VECMP_INIT(&ec->assets, MAX_ASSETS);
	
// 	VECMP_INIT(&ec->actors, MAX_ACTORS);
// 	VECMP_INIT(&ec->cfDescriptions, MAX_CASHFLOW);
	
	VECMP_INIT(&ec->Entity, 16384);
	
	// fill in id 0
	Econ_NewEntity(ec, ET_None, NULL, "Null Entity");
	
	VECMP_INIT(&ec->Position, 16384);
	VECMP_INIT(&ec->Movement, 16384);
	VECMP_INIT(&ec->PathFollow, 16384);
	
	VECMP_INIT(&ec->Parcel, 4096);
	
// 	ec->markets = calloc(1, sizeof(*ec->markets) * MAX_COMMODITIES);
// 	for(uint32_t i = 0; i < MAX_COMMODITIES; i++) ec->markets[i].commodity = i;
	
// 	ec->comNames = calloc(1, sizeof(*ec->comNames) * MAX_COMMODITIES);
	
	
	for(int y = 0; y < 64; y++) {
		for(int x = 0; x < 64; x++) {
			compid_t pcid;
			Parcel* p = Econ_NewParcel(ec, &pcid);
			
			econid_t pid = Econ_NewEntity(ec, ET_Parcel, p, "Mapgen Parcel");
			
			p->loc.x = x;
			p->loc.y = y;
			p->price = 1000;
			p->minerals = 1000000;
			
			ec->parcelGrid[x][y] = pid;
		}
	}
	
	
	
}


entityid_t Econ_CreateCompany(Economy* ec, char* name) {
	Company* c = Econ_NewCompany(ec);
	
	entityid_t cid = Econ_NewEntity(ec, ET_Company, c, name);
	
	c->name = name;
	
	Entity_InitMoney(ec, cid);
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

Entity* Economy_GetEntity(Economy* ec, econid_t id) {
	
}

entityid_t buyLand(Economy* ec, entityid_t buyer, Parcel* parOut) {
	Entity* bent =  Econ_GetEntity(ec, buyer);
	Money* m =  Econ_GetEntMoney(ec, buyer);
	
	VECMP_EACH(&ec->Parcel, pi, p) {
		if(p->owner) continue;
		
		if(m->cash > p->price) {
			
			m->cash -= p->price;
			p->owner = buyer;
			
			if(parOut) parOut = p;
			return p->id;
		}
	}
	
	return 0;
}



// tick functions

void doMine(Economy* ec, Mine* m) {
	Entity* pent = Econ_GetEntity(ec, m->parcel);
	Parcel* p = pent->userData;
	
	p->minerals -= 2;
	m->metals += 1;
}


void doCompany(Economy* ec, Company* c) {
	money_t money = Econ_GetEntMoney(ec, c->id);
	
	// look up money
	
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


void doPerson(Economy* ec, Person* p) {}
void doStore(Economy* ec, Store* p) {}
void doFactory(Economy* ec, Factory* p) {}
void doParcel(Economy* ec, Parcel* p) {}


// tick loop functions

#define X(type) void tick##type(Economy* ec) { \
	VECMP_EACH(&ec->type, ci, c) { \
		do##type(ec, c); \
	} \
}
	ENTITY_TYPE_LIST
#undef X

#define X(type) compid_t Econ_NewComp##type(Economy* ec, type* out) { \
	compid_t id; \
	type* e; \
	VECMP_INC(&(ec)->type); \
	id = VECMP_LAST_INS_INDEX(&(ec)->type); \
	e = &VECMP_ITEM(&(ec)->type, id); \
	memset(e, 1, sizeof(*e)); \
	if(out) out = e; \
	\
	return id; \
}
	COMP_TYPE_LIST
#undef X


#define X(type) compid_t Econ_NewEnt##type(Economy* ec, type* out) { \
	compid_t id; \
	type* e; \
	VECMP_INC(&(ec)->type); \
	id = VECMP_LAST_INS_INDEX(&(ec)->type); \
	e = &VECMP_ITEM(&(ec)->type, id); \
	memset(e, 1, sizeof(*e)); \
	if(out) out = e; \
	\
	return id; \
}
	ENTITY_TYPE_LIST
#undef X

