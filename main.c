#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <unistd.h>
#include <ncurses.h>

#ifndef CTRL_KEY
	#define CTRL_KEY(c) ((c) & 037)
#endif


#include "econ.h"



static void print_companies(Economy* ec);
static void print_mines(Economy* ec);
static void print_parcels(Economy* ec);
static void print_people(Economy* ec);
static void print_factories(Economy* ec);
static void print_stores(Economy* ec);
static void print_map(Economy* ec);



int main(int argc, char* argv[]) {
	int ch;
	
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
	
	int tab = 0;
	
	
// 	entityid_t companyid = Econ_CreateCompany(&ec, "Initech");
	Econ_CreatePerson(&ec, "Adam");
	Econ_CreatePerson(&ec, "Bob");
	Econ_CreatePerson(&ec, "Chuck");
	Econ_CreatePerson(&ec, "Don");
	Econ_CreatePerson(&ec, "Earl");
	Econ_CreatePerson(&ec, "Frank");
	Econ_CreatePerson(&ec, "Garret");
	Econ_CreatePerson(&ec, "Hank");
	Econ_CreatePerson(&ec, "Irving");
	Econ_CreatePerson(&ec, "John");
	Econ_CreatePerson(&ec, "Karl");
	Econ_CreatePerson(&ec, "Lenny");
	Econ_CreatePerson(&ec, "Matt");
	Econ_CreatePerson(&ec, "Nigel");
	Econ_CreatePerson(&ec, "Oscar");
	Econ_CreatePerson(&ec, "Peter");
	Econ_CreatePerson(&ec, "Quentin");
	Econ_CreatePerson(&ec, "Randy");
// 	entityid_t mineID = Econ_CreateMine(&ec, 4, "Big Hole");
// 	Mine* mine = Econ_GetEntity(&ec, mineID)->userData;
// 	mine->owner = companyid;
	
	
	for(int n = 1; n <= 1000; ) {
		clear();
		
		mvprintw(0, 5, "Tick %d\n", n);
		
		switch(tab) {
			case 0: print_companies(&ec); break;
			case 1: print_mines(&ec); break;
			case 2: print_factories(&ec); break;
			case 3: print_stores(&ec); break;
			case 4: print_parcels(&ec); break;
			case 5: print_people(&ec); break;
			case 6: print_map(&ec); break;
		}
		
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
			tab = (tab + 6) % 7;
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







static void print_parcels(Economy* ec) {
	int n = 0;
	int rows, cols;
	getmaxyx(stdscr, rows, cols);
	
	mvprintw(0, 20, "Parcels");
	
	VECMP_EACH(&ec->Parcel, mi, m) {
		mvprintw(n + 1, 2, "%ld| minerals: %d, owner: %d\n", mi, m->minerals, m->owner);
		if(++n >= rows - 2) return;
	}
	
}

static void print_mines(Economy* ec) {
	int n = 0;
	int rows, cols;
	getmaxyx(stdscr, rows, cols);
	
	mvprintw(0, 20, "Mines");
	
	VECMP_EACH(&ec->Mine, mi, m) {
		mvprintw(n + 1, 2, "%ld| metals: %d\n", m->id, m->metals);
		
		if(++n >= rows - 2) return;
	}
	
}


static void print_people(Economy* ec) {
	int n = 0;
	int rows, cols;
	getmaxyx(stdscr, rows, cols);
	
	mvprintw(0, 20, "People");
	
	VECMP_EACH(&ec->Person, pi, p) {
		Entity* e = Econ_GetEntity(ec, p->id);
		Money* m = Econ_GetCompMoney(ec, e->money);
		
		mvprintw(n + 1, 2, " %d| %s,\t cash: %d\n", p->id, p->name, m->cash);
		
		if(++n >= rows - 2) return;
	}
	
}

static void print_companies(Economy* ec) {
	int n = 0;
	int rows, cols;
	getmaxyx(stdscr, rows, cols);
	
	mvprintw(0, 20, "Companies");
	
	VECMP_EACH(&ec->Company, pi, c) {
		Entity* e = Econ_GetEntity(ec, c->id);
		Money* m = Econ_GetCompMoney(ec, e->money);
		
		mvprintw(n + 1, 2, " %d| cash: %d, MIT: %d, par: %d, mine: %d, fact: %d, store: %d \n", 
				 c->id, m->cash, c->metalsInTransit, VEC_LEN(&c->parcels), 
				 VEC_LEN(&c->mines), VEC_LEN(&c->factories), VEC_LEN(&c->stores));
		
		if(++n >= rows - 2) return;
	}
	
}


static void print_stores(Economy* ec) {
	int n = 0;
	int rows, cols;
	getmaxyx(stdscr, rows, cols);
	
	mvprintw(0, 20, "Stores");
	
	VECMP_EACH(&ec->Store, pi, p) {
		Entity* e = Econ_GetEntity(ec, p->id);
		Money* m = Econ_GetCompMoney(ec, e->money);
		Store* s = e->userData;
		
		mvprintw(n + 1, 2, " %d| widgets: %d, sales %d\n", p->id, s->widgets, s->sales);
		
		if(++n >= rows - 2) return;
	}
	
}


static void print_factories(Economy* ec) {
	int n = 0;
	int rows, cols;
	getmaxyx(stdscr, rows, cols);
	
	mvprintw(0, 20, "Factories");
	
	VECMP_EACH(&ec->Factory, pi, p) {
		Entity* e = Econ_GetEntity(ec, p->id);
		Factory* f = e->userData;
		
		mvprintw(n + 1, 2, " %d| metals: %d, widgets: %d\n", p->id, f->metals, f->widgets);
		
		if(++n >= rows - 2) return;
	}
	
}



static void print_map(Economy* ec) {
	
	int rows, cols;
	getmaxyx(stdscr, rows, cols);

	
	
	for(int y = 0; y < 64; y++) {
		for(int x = 0; x < 64; x++) {
						
			attron(COLOR_PAIR(2));
			mvaddch(y, x*2,   ' ');
			mvaddch(y, x*2+1, ' ');
			attroff(COLOR_PAIR(2));
		}
	}
	
	
}

// 

// static void farmer_fn(Ec) {
// 	
// }
// 
