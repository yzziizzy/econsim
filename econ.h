
#include <stdint.h>


#include "c3dlas/c3dlas.h"

#include "mempool.h"
#include "ds.h"


typedef int64_t money_t;
typedef uint32_t entityid_t;
typedef uint32_t compid_t;
typedef uint32_t econid_t;
typedef uint32_t tick_t; // 4 billion tick limit to world sim time


#define ECON_MAGIC (INT64_MAX - 1)
#define ECON_CASHMAX (INT64_MAX - 10)

enum EcAssetType {
	ECASST_MISC = 0,
	ECASST_LAND = 1,
	ECASST_MINE = 2,
};









typedef struct EcCashflow {
	econid_t from, to;
	money_t amount;
	tick_t frequency;
} EcCashflow;


typedef struct EcAsset {
	econid_t id;
	econid_t owner;
	econid_t type;
	econid_t location;
	money_t purchasePrice; // last purchase price
	money_t curEstValue;
	money_t buyNowPrice;
	char* name;
} EcAsset;


typedef struct EcWarehouse {

} EcWarehouse;

#define ECORDERTYPE_ASK 0x00
#define ECORDERTYPE_BID 0x01
#define ECORDERTYPE_MARKET 0x00
#define ECORDERTYPE_LIMIT 0x02
#define ECORDERTYPE_M_ASKBID 0x01
#define ECORDERTYPE_M_MARLIM 0x02


typedef struct EcMarketOrder {
	unsigned char type; 
	money_t price;
	uint32_t qty; // 0 for unlimited
	econid_t who;
} EcMarketOrder;


typedef struct EcMarket {
	econid_t commodity;
	VEC(EcMarketOrder) asks;
	VEC(EcMarketOrder) bids;
} EcMarket;

typedef struct EcActor {
	econid_t id;
	char* name;
	// type, ie, person or corporation
	// occupation
	// age
} EcActor;



#define ENTITY_TYPE_LIST \
	X(Parcel) \
	X(Mine) \
	X(Factory) \
	X(Company) \
	X(Store) \
	X(Person)

#define COMP_TYPE_LIST \
	X(Position) \
	X(Movement) \
	X(Money) \
	X(PathFollow) \


enum EntityType {
	ET_None = 0,
	
#define X(x) ET_##x,
	ENTITY_TYPE_LIST
#undef X
	
	ET_MAXVALUE,
};


/*




*/
typedef struct Entity {
	unsigned int dead : 1;
	unsigned int _pad : 23;
	unsigned int uniqueCounter : 8;
	
	tick_t created;
	
	enum EntityType type;
	
	
	econid_t money;
	econid_t position;
	econid_t movement;
	econid_t pathFollow;
	
	void* userData;
	
	// for debugging:
	char* name;
} Entity;




typedef struct Money {
	money_t cash;
	money_t totalDebt;
	money_t totalAssets;
} Money;

typedef struct Position {
	Vector p;
} Position;

typedef struct Movement {
	Vector direction;
	float vel;
	float acc;
	float maxVel;
	float maxAcc;
} Movement;


typedef struct Path {
	VEC(Vector) nodes;
	float totalLength;
} Path;

typedef struct PathFollow {
	Path* p;
	int a, b; // moving from a to b
	float segmentProgress;
	float totalProgress;
} PathFollow;


// TODO: get rid of this in favor of CPosition
typedef struct Location {
	int x, y;
} Location;

typedef struct Parcel {
	entityid_t id;
	
	Location loc;
	money_t price;
	int minerals;
	
	entityid_t owner;
	entityid_t structure;
} Parcel;


typedef struct Company {
	entityid_t id;
	
	char* name;
	VEC(entityid_t) parcels;
	VEC(entityid_t) mines;
	VEC(entityid_t) factories;
	VEC(entityid_t) stores;
	
	int metalsInTransit;
	int widgetsInTransit;
	
} Company;


typedef struct Mine {
	entityid_t id;
	char* name;
	
	Location loc;
	entityid_t parcel;
	entityid_t owner;
	
	
	int metals;
	
} Mine;


typedef struct Factory {
	entityid_t id;
	char* name;
	
	Location loc;
	entityid_t owner;
	entityid_t parcel;
	
	int metals;
	int widgets;
	
} Factory;


typedef struct Person {
	entityid_t id;
	char* name;
	
} Person;

typedef struct Store {
	entityid_t id;
	char* name;
	
	entityid_t owner;
	entityid_t parcel;
	int widgets;
	
} Store;



typedef struct Economy {
	// processed every tick
	VECMP(EcCashflow) cashflow;
	
// 	VECMP(EcAsset) assets;
	
	// TODO: debt
	
// 	EcMarket* markets;
// 	char** comNames;
	
// 	VECMP(EcActor) actors;
// 	VECMP(char*) cfDescriptions;
	
	size_t entityCounts[ET_MAXVALUE];
	
	
	VECMP(Entity) Entity;
	
	
	#define X(type) VECMP(type) type;
		ENTITY_TYPE_LIST
	#undef X
	
	#define X(type) VECMP(type) type;
		COMP_TYPE_LIST
	#undef X
	
	
	// temporary junk:
	
	entityid_t parcelGrid[64][64];
	
} Economy;


// tick loop functions
#define X(type) void tick##type(Economy* ec);
	ENTITY_TYPE_LIST
#undef X

#define X(type) entityid_t Econ_NewEnt##type(Economy* ec, type** out);
	ENTITY_TYPE_LIST
#undef X

#define X(type) compid_t Econ_NewComp##type(Economy* ec, type** out);
	COMP_TYPE_LIST
#undef X


#define X(type) type* Econ_GetComp##type(Economy* ec, compid_t id);
	COMP_TYPE_LIST
#undef X

void Economy_tick(Economy* ec);

econid_t Economy_AddActor(Economy* ec, char* name, money_t cash);
econid_t Economy_AddCashflow(Economy* ec, money_t amount, econid_t from, econid_t to, uint32_t freq, char* desc);
econid_t Economy_AddAsset(Economy* ec, EcAsset* ass);

void Economy_init(Economy* ec);
Entity* Econ_GetEntity(Economy* ec, econid_t eid);

entityid_t Econ_CreateCompany(Economy* ec, char* name);
entityid_t Econ_CreatePerson(Economy* ec, char* name);
entityid_t Econ_CreateMine(Economy* ec, entityid_t parcel_id, char* name);
entityid_t Econ_CreateFactory(Economy* ec, entityid_t parcel_id, char* name);
entityid_t Econ_CreateStore(Economy* ec, entityid_t parcel_id, char* name);
