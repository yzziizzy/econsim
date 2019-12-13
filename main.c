#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>



#include "econ.h"



static void print_cash(Economy* ec, int count);




int main(int argc, char* argv[]) {
	
	Economy ec;
	
	Economy_init(&ec);
	
	
	
	/*
	Economy_AddActor(&ec, "Nobody", 100);
	Economy_AddActor(&ec, "A", 100);
	Economy_AddActor(&ec, "B", 100);
	Economy_AddActor(&ec, "C", 100);
	Economy_AddActor(&ec, "D", 100);
	Economy_AddActor(&ec, "E", 100);
	
	Economy_AddCashflow(&ec, 30, 0, 1, 1, "Pay me");
	
	for(int i = 0; g_assets[i].id != UINT32_MAX; i++) {
		Economy_AddAsset(&ec, &g_assets[i]);
	}
	
	*/
	
	printf("\n-- tick 0 -------------\n");
	print_cash(&ec, 5);
	
	for(int n = 1; n <= 10; n++) {
		Economy_tick(&ec);
		
		printf("\n-- tick %d -------------\n", n);
		
		print_cash(&ec, 5);
	}
	
	
	
	return 0;
}







static void print_cash(Economy* ec, int count) {
// 	for(int i = 0; i < VECMP_LEN(&ec->cash); i++) {
// 		printf("%d: $%ld '%s'\n",
// 			i,
// 			VECMP_ITEM(&ec->cash, i), 
// 			VECMP_ITEM(&ec->actors, i).name 
// 		);
// 	}
}






// 

// static void farmer_fn(Ec) {
// 	
// }
// 
