
#include <stdint.h>


#include "mempool.h"
#include "ds.h"


typedef int64_t money_t;
typedef uint32_t econid_t;


#define ECON_MAGIC (INT64_MAX - 1)
#define ECON_CASHMAX (INT64_MAX - 10)




typedef struct EcCashflow {
	econid_t from, to;
	money_t amount;
	uint32_t frequency; // ticks, which are days for now
} EcCashflow;


typedef struct EcAsset {
	econid_t owner;
	money_t purchasePrice; // last purchase price
	money_t curEstValue;
	money_t buyNowPrice;
	char* name;
} EcAsset;


typedef struct EcMarketOrder {
	money_t price;
	uint32_t qty; // 0 for unlimited
} EcMarketOrder;


typedef struct EcMarket {
	econcom_t commodity;
	VEC(EcMarketOrder) asks;
	VEC(EcMarketOrder) bids;
} EcMarket;

typedef struct EcActor {
	char* name;
	// type, ie, person or corporation
	// occupation
	// age
} EcActor;


typedef struct Economy {
	// processed every tick
	VECMP(money_t) cash; // this is the primary array.
	VECMP(EcCashflow) cashflow;
	
	// debt
	EcMarket* markets;
	char** comNames;
	
	VECMP(EcActor) actors;
	VECMP(char*) cfDescriptions;
} Economy;



void Economy_tick(Economy* ec);

econid_t Economy_AddActor(Economy* ec, char* name, money_t cash);
econid_t Economy_AddCashflow(Economy* ec, money_t amount, econid_t from, econid_t to, uint32_t freq, char* desc);
void Economy_init(Economy* ec);


