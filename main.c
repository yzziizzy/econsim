#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <unistd.h>
#include <ncurses.h>

#ifndef CTRL_KEY
	#define CTRL_KEY(c) ((c) & 037)
#endif


#include "econ.h"

FILE* _log;



static void print_comp_val(Economy* ec, Comp* c);
static void print_entity(Economy* ec, Entity* e, int width, int offset, char** cols);
static void print_entities_type(
	Economy* ec, int type, int voffset,
	int width, char** cols, int hoffset
);





int main(int argc, char* argv[]) {
	int ch;
	
	_log = fopen("/tmp/econsim.log", "w");

	Economy ec;
	
	Economy_init(&ec);

	
	// ncurses stuff
	initscr();
	raw();
	timeout(0); // nonblocking i/o
	keypad(stdscr, TRUE);
	noecho();
// 	curs_set(0); // hide the cursor
	
	start_color();
// 	init_pair(1, COLOR_WHITE, COLOR_BLACK | A_BOLD);
    assume_default_colors( COLOR_WHITE, COLOR_BLACK | A_BOLD);
	
	
	
	
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
	
	int tab = 1;
	int scrollh = 0;
	int scrollv = 0;
	
	
	typedef struct View {
		char* name;
		char* dispName;
		char** cols;
		
	} View;
	
	View views[] = {
		{.name = "Forest", .dispName = "Forests", .cols = (char*[]){"name", "!Tree", "!Log", "!Board", "!Sawdust", NULL} },
		{.name = "Person", .dispName = "People", .cols = (char*[]){"name", "cash", NULL} },
		{.name = "Mine", .dispName = "Mines", .cols = (char*[]){"name", "!Iron Ore", NULL} },
		
		{.name = NULL},
	};
	
	
	for(int n = 1; n <= 1000; ) {
		
		
		erase();
		
		mvprintw(0, 1, "Tick %d\n", n);
		mvprintw(0, 20, "%s:\n", views[tab].dispName);
		
		mvprintw(1, 2, "char %d, %d, %d", scrollh, scrollv, tab);
		
		
		int entType = Economy_EntityType(&ec, views[tab].name);
		
		print_entities_type(&ec, entType, scrollv, 20, views[tab].cols, scrollh);
		
		
		
		
		while(1) {
			ch = getch();
			if(ch == -1) {
				usleep(10000);
				continue;
			}
			
			break;
		}
		
		
		if(ch == CTRL_KEY('c') || ch == 'q') {
			break;
		}
		
		if(ch == '\t') {
			scrollh = 0;
			scrollv = 0;
			
			tab++;
			
			if(views[tab].name == NULL) tab = 0;
		}
		else if(ch == KEY_LEFT) {
			scrollh = MAX(0, scrollh - 1);
		}
		else if(ch == KEY_RIGHT) {
			scrollh = MAX(0, scrollh + 1);
		}
		else if(ch == KEY_UP) {
			scrollv = MAX(0, scrollv - 1);
		}
		else if(ch == KEY_DOWN) {
			scrollv = MAX(0, scrollv + 1);
		}
		
		if(ch == ' ') {
			Economy_tick(&ec);
			n++;
		}
	}
	
	
	
	endwin();
	
	fclose(_log);
	
	return 0;
}




static void print_comp_val(Economy* ec, Comp* c) {
	if(!c) return;
	
	// TODO: support arrays
	CompDef* cd = Econ_GetCompDef(ec, c->type);
	switch(cd->type) {
		default:
			fprintf(stderr, "unknown component type: %d\n", cd->type);
			exit(1);
			
		case CT_int: printw("%ld", c->n); break;
		case CT_float: printw("%f", c->d); break;
		case CT_str: printw("%s", c->str); break;
		case CT_id: printw("%ld", c->id); break;
		case CT_itemRate: printw("%ld:%f", c->itemRate->item, c->itemRate->rate); break;
	}
	
}


static void print_entity(Economy* ec, Entity* e, int width, int offset, char** cols) {
	int x, y;
////	int rows, cols;
//	getmaxyx(stdscr, rows, cols);
	getyx(stdscr, y, x);
		
	
	for(int i = 0, n = 0; cols[i] && n < 6; i++) {
		if(i < offset) continue;
		
		move(y, x + (width * n));
		
		if(cols[i][0] == '!') {
//			LOG("inv");
			int itemid = Econ_FindItem(ec, cols[i] + 1);
//			LOG(" - %d", itemid);
			InvItem* item = Inv_GetItemP(e->inv, itemid);
			EscrowItem* eitem = Inv_GetEscrowItemP(e->inv, e->id, itemid);
//			LOG(" -- %p", item);
			if(eitem) 
				printw("%ld/%ld", item->count, eitem->count);
			else if(item)
				printw("%ld", item->count);
		}
		else {
			Comp* c = Entity_GetCompName(ec, e, cols[i]);
			print_comp_val(ec, c);
		}
		
		n++;
	}
}






static void print_entities_type(
	Economy* ec, int type, int voffset,
	int width, char** cols, int hoffset
) {
	
	int n = 0;
	VECMP_EACH(&ec->entities, i, e) {
		if(e->type != type) continue;
		if(i < voffset) continue;
		
		move(n++ + 3, 2);		
		
		print_entity(ec, e, width, hoffset, cols); 
		
		if(n > 20) break;
	}
	
}

 
