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











