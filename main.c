#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <unistd.h>
#include <ncurses.h>

#ifndef CTRL_KEY
	#define CTRL_KEY(c) ((c) & 037)
#endif


#include "econ.h"

static void print_comp_val(Economy* ec, Comp* c);
static void print_entity(Economy* ec, Entity* e, int width, int offset, char** cols);
static void print_entities_type(
	Economy* ec, int type, int voffset,
	int width, char** cols, int hoffset
);




int main(int argc, char* argv[]) {
	int ch;

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
	
	char* cols[] = {
		"name",
		"cash",
		"owner",
		"owns",
	};
	
	for(int n = 1; n <= 1000; ) {
		clear();
		
		mvprintw(0, 5, "Tick %d\n", n);
		
		
		print_entities_type(&ec, tab, scrollv, 20, cols, scrollh);
		
		
		while(1) {
			ch = getch();
			if(ch == -1) {
				usleep(10000);
				continue;
			}
			
			break;
		}
		
		if(ch == CTRL_KEY('c')) {
			break;
		}
		
		mvprintw(18, 2, "char %d, %d", ch, tab);
		if(ch == KEY_LEFT) {
			tab = ((tab + 6) % 7) + 1;
		}
		else if(ch == KEY_RIGHT) {
			tab = (tab + 1) % 7;
		}
		
		if(ch == ' ') {
			Economy_tick(&ec);
			n++;
		}
	}
	
	
	
	endwin();
	
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
		case CT_itemrate: printw("%ld:%f", c->itemRate.item, c->itemRate.rate); break;
	}
	
}


static void print_entity(Economy* ec, Entity* e, int width, int offset, char** cols) {
	int x, y;
////	int rows, cols;
//	getmaxyx(stdscr, rows, cols);
	getyx(stdscr, y, x);
		
	
	for(int i = offset, n = 0; cols[i] && n < 3; i++, n++) {
		move(y, x + (width * n));
		Comp* c = Entity_GetCompName(e, cols[i]);
		
		print_comp_val(ec, c);
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
		
		move(n++ + 2, 2);		
		
		print_entity(ec, e, width, hoffset, cols); 
		
		if(n > 20) break;
	}
	
}

 
