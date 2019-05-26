#include <stdlib.h>
#include <stdio.h>


#include "econ.h"



#define MAX_ITEMS        1024*1024
#define MAX_CASHFLOW     1024*1024
#define MAX_COMMODITIES  1024


#define CLAMP(v, min, max) (v < min ? v : (v > max ? max : v))



void Economy_tick(Economy* ec) {
	
	
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
	
	
	
	
}








void Economy_init(Economy* ec) {
	VECMP_INIT(&ec->cash, MAX_ITEMS);
	VECMP_INIT(&ec->cashflow, MAX_CASHFLOW);
	
	VECMP_INIT(&ec->actors, MAX_ITEMS);
	VECMP_INIT(&ec->cfDescriptions, MAX_CASHFLOW);
	
	ec->markets = calloc(1, sizeof(*ec->markets) * MAX_COMMODITIES);
	for(uint32_t i = 0; i < MAX_COMMODITIES; i++) ec->markets[i] = i;
	
	ec->comNames = calloc(1, sizeof(*ec->comNames) * MAX_COMMODITIES);
	
}


econid_t Economy_AddActor(Economy* ec, char* name, money_t cash) {
	econid_t id;
	
	VECMP_INSERT(&ec->cash, cash);
	id = VECMP_LAST_INS_INDEX(&ec->cash);
	char* n = strdup(name);
	
	VECMP_ITEM(&ec->names, id) = n;
	
	
	return id;
}



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
	
	VECMP_ITEM(&ec->cfDescriptions, id) = strdup(desc);
	
	
	return id;
}




