#include <stdio.h>
#include <stdint.h>
#include <limits.h>


#include "econ.h"







Market* Market_New() {
	Market* m = calloc(1, sizeof(*m));
	Market_Init(m);
	return m;
}

void Market_Init(Market* m) {
	VEC_INIT(&m->sells);
	VECMP_INIT(&m->sinks, 4096);
}

void Market_Destroy(Market* m) {
	VEC_FREE(&m->sells);
	VECMP_FREE(&m->sinks);
}

void Market_Free(Market* m) {
	Market_Destroy(m);
	free(m);
}


void Market_AddSellOrder(Market* m, Entity* seller, econid_t item, long qty, money_t price) {
	if(price < 1) price = 1; // HACK
	
	long moved = Inv_MoveToEscrow(seller->inv, seller->id, item, qty);
	
	VEC_PUSH(&m->sells, ((MarketOrder){
		.seller = seller,
		.item = item,
		.qtyAvail = moved,
		.minQty = 0,
		.price = price,
	}));
}

void Market_BuyNow(Market* m, Entity* buyer, econid_t item, long* qty, money_t* price) {
	long maxQ = *qty;
	money_t maxP = *price;
	
	long bought = 0;
	money_t spent = 0;	

	
	for(intptr_t i = 0; maxP && maxQ && i < VEC_LEN(&m->sells); i++) {
		MarketOrder* o = &VEC_ITEM(&m->sells, i);
		
		if(o->item != item) continue;
		
		// careful of overfow
		long maxSpend = MIN(o->qtyAvail * o->price, maxP);
		
		// careful of free items
		long maxBuy = maxSpend / o->price;
		long toBuy = MIN(maxBuy, maxQ);
		
		if(toBuy == 0) continue;
		
		long changed = Inv_EscrowChangeOwner(o->seller->inv, o->seller->id, buyer->id, item, toBuy);
		
		o->qtyAvail -= changed;
		maxQ -= changed;
		bought += changed;
		spent += changed * o->price;
		
		// delete empty orders
		if(o->qtyAvail == 0) {
			*o = VEC_TAIL(&m->sells);
			VEC_LEN(&m->sells)--;
			i--; // retry this index
		}
	}

	*qty = bought;
	*price = spent; 
	
	
}



MarketSink* Market_AddSink(Market* m, econid_t item, money_t maxBuyPrice) {
	econid_t id;
	MarketSink* s;
	
	VECMP_INC(&m->sinks);
	id = VECMP_LAST_INS_INDEX(&m->sinks);
	s = &VECMP_ITEM(&m->sinks, id);
	memset(s, 0, sizeof(*s));
	
	s->id = id;
	s->item = item;
	s->maxBuyPrice = maxBuyPrice;
	
	return s;
}


void Market_RunSinks(Market* m) {
	
	VECMP_EACH(&m->sinks, i, sink) {
		long maxP = sink->maxBuysPerTick * sink->maxBuyPrice;
		long maxQ = sink->maxBuysPerTick;
		if(maxQ < 0) maxQ = LONG_MAX;
		
		if(maxQ > 0)
			Market_BuyNow(m, m->sinkEntity, sink->item, &maxQ, &maxP);
		
	}

}










