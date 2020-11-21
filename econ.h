#ifndef __econsim_econ_h__
#define __econsim_econ_h__


#include <stdint.h>
#include <stdarg.h>


#include "c3dlas/c3dlas.h"
#include "sti/sti.h"
#include "c_json/json.h"


typedef  int64_t money_t;
typedef uint32_t entityid_t;
typedef uint32_t compid_t;
typedef uint32_t econid_t;
typedef uint32_t commodityid_t;
typedef  int32_t qty_t;
typedef uint32_t tick_t; // 4 billion tick limit to world sim time


#define ECON_MAGIC (INT64_MAX - 1)
#define ECON_CASHMAX (INT64_MAX - 10)









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



#define COMMODITY_LIST \
	X(Ore) \
	X(Metal) \
	X(Widget) \
	X(Food)



enum CommodityType {
	COMT_None = 0,
	
	#define X(x) COMT_##x,
		COMMODITY_LIST
	#undef X
	
	COMT_MAXVALUE,
};
typedef struct CommodityBucket {
	commodityid_t comm;
	qty_t qty;
} CommodityBucket;



typedef struct CommoditySet {
	int length;
	int fill;
	CommodityBucket buckets[0];
} CommoditySet;










// jsonNam,    isPtr, typeName,    propName
#define COMP_TYPE_LIST \
	X(int,         0, int64_t,     n) \
	X(float,       0, double,      d) \
	X(id,          0, econid_t,    id) \
	X(str,         0, char*,       str) \
	X(itemRate,    1, ItemRate,    itemRate) \
	X(conversion,  1, econid_t,    conversion) \



enum CompType {
	CT_NULL = 0,
	
#define X(x, a, b, c) CT_##x,
	COMP_TYPE_LIST
#undef X
	
	CT_MAXVALUE,
};



typedef struct ItemRate {
	econid_t item;
	float rate;
} ItemRate;

typedef struct ConvertRate {
	econid_t fromItem;
	econid_t toItem;
	
	float rate;
} ConvertRate;



typedef struct CompDef {
	int id;
	char* name;
	enum CompType type;
	char isArray;
	char isPtr;
} CompDef;

#define IF_PTR_1 *
#define IF_PTR_0
#define IF_PTR(a) IF_PTR_ ## a

typedef struct Comp {
	int type;
	unsigned short length;
	unsigned short alloc;
	union {
		void* vp;
		
		#define X(x, a, b, c) b IF_PTR(a) c;
			COMP_TYPE_LIST
		#undef X
	};
} Comp;



typedef struct InvItem {
	econid_t item;
	long count;
} InvItem;


typedef struct Inventory {
	VECMP(InvItem) items;
} Inventory;


typedef struct EntityDef {
	int id;
	char* name;
		
	VEC(struct {
		int defID;
		char hasLocalDefault;
		Comp localDefault;
	}) defaultComps;
	
} EntityDef;


typedef struct Entity {
	econid_t id;
	unsigned int dead : 1;
	unsigned int _pad : 23;
	unsigned int uniqueCounter : 8;
	unsigned int type;
	tick_t born, died;
	
	VEC(Comp) comps;
	Inventory inv;
	 
	// for debugging:
	char* name;
} Entity;


typedef struct Conversion {
	econid_t id;
	char* name;
	
	int inputCnt, outputCnt;
	struct {
		econid_t id;
		long count;
	} *inputs, *outputs;
	
} Conversion;

typedef struct Economy {
	tick_t tick;

	// processed every tick
	VECMP(EcCashflow) cashflow;
	
//	size_t entityCounts[ET_MAXVALUE];
	
	VECMP(EntityDef) entityDefs;
	VECMP(CompDef) compDefs;
	
	VECMP(Entity) entities;
	VECMP(Conversion) conversions;
	
	VEC(econid_t) convertors;
	
// 	// total amount of each commodity in the world
//	int64_t commodityTotals[CT_MAXVALUE];
	
//	entityid_t parcelGrid[64][64];
	
} Economy;



Entity* Econ_NewEntity(Economy* ec, int type, char* name);
Entity* Economy_NewEntityName(Economy* ec, char* typeName, char* name);
int Economy_EntityType(Economy* ec, char* typeName);
EntityDef* Economy_NewEntityDef(Economy* ec);

CompDef* Economy_NewCompDef(Economy* ec);
CompDef* Econ_GetCompDef(Economy* ec, int compType);
CompDef* Econ_GetCompDefName(Economy* ec, char* compName);

int Economy_CompTypeFromName(Economy* ec, char* compName);
int CompInternalTypeFromName(char* t);
int Economy_LoadConfig(Economy* ec, char* path);
int Economy_LoadConfigJSON(Economy* ec, json_value_t* root);
Comp* Entity_GetCompName(Economy* ec, Entity* e, char* compName);
Comp* Entity_GetComp(Entity* e, int compType);
Comp* Entity_AddComp(Entity* e, int compType);
Comp* Entity_AssertComp(Entity* e, int compType);
Comp* Entity_SetCompName(Economy* ec, Entity* e, char* compName, ...);
Comp* Entity_SetComp(Economy* ec, Entity* e, int compType, ...);
Comp* Entity_SetComp_va(Economy* ec, Entity* e, int ctype, va_list va);
int Econ_CompTypeFromName(Economy* ec, char* compName);

char* CompInternalType_GetName(int compInternalType);
size_t InternalCompTypeSize(int internalType);

econid_t Econ_FindItem(Economy* ec, char* name);

void Inv_Init(Inventory* inv);
InvItem* Inv_GetItemP(Inventory* inv, econid_t id);
InvItem* Inv_AddItem(Inventory* inv, econid_t id, long count);
InvItem* Inv_AssertItemP(Inventory* inv, econid_t id);

Conversion* Econ_NewConversion(Economy* ec);


/*
// tick loop functions
#define X(type, a) void tick##type(Economy* ec);
	ENTITY_TYPE_LIST
#undef X

#define X(type, a) entityid_t Econ_NewEnt##type(Economy* ec, type** out);
	ENTITY_TYPE_LIST
#undef X

#define X(type, a) compid_t Econ_NewComp##type(Economy* ec, type** out);
	COMP_TYPE_LIST
#undef X


#define X(type, a) type* Econ_GetComp##type(Economy* ec, compid_t id);
	COMP_TYPE_LIST
#undef X
*/
void Economy_tick(Economy* ec);
/*
econid_t Economy_AddActor(Economy* ec, char* name, money_t cash);
econid_t Economy_AddCashflow(Economy* ec, money_t amount, econid_t from, econid_t to, uint32_t freq, char* desc);
econid_t Economy_AddAsset(Economy* ec, EcAsset* ass);
*/
void Economy_init(Economy* ec);
Entity* Econ_GetEntity(Economy* ec, econid_t eid);

/*
entityid_t Econ_CreateCompany(Economy* ec, char* name);
entityid_t Econ_CreatePerson(Economy* ec, char* name);
entityid_t Econ_CreateMine(Economy* ec, entityid_t parcel_id, char* name);
entityid_t Econ_CreateFactory(Economy* ec, entityid_t parcel_id, char* name);
entityid_t Econ_CreateStore(Economy* ec, entityid_t parcel_id, char* name);

void Econ_CommodityExNihilo(Economy* ec, commodityid_t comm, qty_t qty);
*/






/*
CommoditySet* CommoditySet_New(int size);
int CommoditySet_Add(CommoditySet* cs, commodityid_t comm, qty_t qty, int compact);
qty_t CommoditySet_Get(CommoditySet* cs, commodityid_t comm);
*/


/*

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


typedef struct House {
	entityid_t id;
	
	money_t price;
	money_t rent;
	
	entityid_t owner;
	entityid_t occupants[4];
} House;


typedef struct Farm {
	entityid_t id;
	
	money_t price;
	int food;
	
	entityid_t owner;
} Farm;


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
	
	entityid_t employees[16];
	int employeeCnt; 
	
} Factory;


typedef struct Person {
	entityid_t id;
	char* name;
	
	entityid_t employer;
	entityid_t residence;
	
} Person;

typedef struct Store {
	entityid_t id;
	char* name;
	
	entityid_t owner;
	entityid_t parcel;
	int widgets;
	
	money_t sales;
	
	entityid_t employees[16];
	int employeeCnt; 
	
} Store;

*/


#endif // __econsim_econ_h__
