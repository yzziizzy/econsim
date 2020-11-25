#include <stdlib.h>
#include <stdio.h>


#include "econ.h"



Conversion* Econ_NewConversion(Economy* ec) {
	econid_t id;
	Conversion* c;
	
	VECMP_INC(&ec->conversions);
	id = VECMP_LAST_INS_INDEX(&ec->conversions);
	c = &VECMP_ITEM(&ec->conversions, id);
	memset(c, 0, sizeof(*c));
	
	c->id = id;

	return c;	
}


long Conv_MaxAvail(Conversion* conv, Inventory* inv) {
	if(!inv || !conv) return 0;
	long max = -1;
	
	for(int i = 0; i < conv->inputCnt; i++) {
		InvItem* item = Inv_GetItemP(inv, conv->inputs[i].item);
		if(!item || item->count < conv->inputs[i].count) return 0;
		long amt = item->count / conv->inputs[i].count;
		
		max = (max == -1) ? amt : MIN(amt, max);
	}
	
	return max;
}



// does not check if it can be done
long Conv_DoConversion(Conversion* conv, Inventory* inv, long count) {
	if(!inv || !conv || count <= 0) return 1;
	
	// subtract inputs
	for(int i = 0; i < conv->inputCnt; i++) {
		InvItem* item = Inv_GetItemP(inv, conv->inputs[i].item);
		if(item) {
			long n = conv->inputs[i].count * count;
			item->count = MAX(0, item->count - n);
		}
	}
	
	// add outputs
	for(int i = 0; i < conv->outputCnt; i++) {
		Inv_AddItem(inv, conv->outputs[i].item, conv->outputs[i].count * count);
	}
	
	return 0;
}








