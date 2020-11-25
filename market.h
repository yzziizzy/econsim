


typedef struct MarketOrder {
	Entity* seller;
	econid_t item;
	long qtyAvail;
	long minQty;
	money_t price; // FOB
} MarketOrder;


// endlessly buys items
typedef struct MarketSink {
	econid_t id;
	char* name;
	
//	long maxBuysPerOrder;
	long maxBuysPerTick;
	money_t maxBuyPrice;
	econid_t item;
} MarketSink;




typedef struct Market {

	VEC(MarketOrder) sells;

	VECMP(MarketSink) sinks;
	Entity* sinkEntity;
	
} Market;




Market* Market_New();
void Market_Init(Market* m);
void Market_Destroy(Market* m);
void Market_Free(Market* m);


void Market_AddSellOrder(Market* m, Entity* seller, econid_t item, long qty, money_t price);
void Market_BuyNow(Market* m, Entity* buyer, econid_t item, long* qty, money_t* price);

MarketSink* Market_AddSink(Market* m, econid_t item, money_t maxBuyPrice);
void Market_RunSinks(Market* m);
	



