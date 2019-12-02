#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "timer.h"

/* define some shortcut for struct */
#define City_count(tour) (tour->count)
#define Tour_cost(tour) (tour->cost)
#define Last_city(tour) (tour->cities[tour->count-1])
#define Tour_city(tour, i) (tour->cities[i])
#define Cost_a2b(a, b) (digraph[a*n+b])

const int NO_CITY = -1;
const int INFINITY = 1000000;
const int TRUE = 1;
const int FALSE = 0;


typedef int city_t;
typedef int cost_t;

/* tour cost struct */
typedef struct {
	city_t* cities;	// city
	int count;		// city number
	cost_t cost;	// cost
} tour_struct;
typedef tour_struct* tour_t;

/* stack struct */
typedef struct {
	tour_t* list;	// stack item
	int sz;			// number of stack item
} stack_struct;
typedef stack_struct* stack_t;

int n;				// city number
int* digraph;		// digraph
city_t home_town = 0;
tour_t best_tour;


/*-----------------------------------------------------------------------------
 * read digraph form file
 */
void Read_digraph(char* file_name) {
	FILE* file;
	// open file
	file = fopen(file_name, "r");
	if (file == NULL) {
		fprintf(stderr, "Can't find file!\n");
		exit(-1);
	}

	// read number of city
	fscanf(file, "%d", &n);
	if (n <= 0) {
		fprintf(stderr, "Number of city can't less than 1!\n");
		exit(-1);
	}

	digraph = malloc(n*n*sizeof(cost_t));	// alloc mem
	
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++){
			fscanf(file, "%d", &digraph[i*n + j]);	// read cost
			if (i == j && digraph[i*n + j] != 0) {
				fprintf(stderr, "The cost of city to itself is must be 0\n");
				exit(-1);
			} else if (i != j && digraph[i*n + j] <= 0) {
				fprintf(stderr, "The cost of %dth to %dth is must be greater than 0!\n", i, j);
				exit(-1);
			}
		}
	}
}	/* Read_digraph */


/*-----------------------------------------------------------------------------
 * print number and cost of city
 */
void Print_digraph() {
	printf("print digraph:\n");
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			printf("%d ", digraph[i*n + j]);
		}
		printf("\n");
	}
}	/* Print_digraph */

/*-----------------------------------------------------------------------------
 * print tour
 */
void Print_tour(tour_t tour) {
	printf("print tour: ");
    for (int i = 0; i < tour->count; i++) {
		printf("%d ", tour->cities[i]);
    }
	printf("\n");
}   /* Print_tour */


/*-----------------------------------------------------------------------------
 * print stack
 */
void Print_stack(stack_t stack) {
	if (stack == NULL || stack->sz == 0) {
		fprintf(stderr, "stack is NULL\n");
		return;
	}
	tour_t tmp;
	printf("print stack:\n");
	for (int i = stack->sz-1; i >= 0; i--) {
		tmp = stack->list[i];
		if (tmp->count == 0) {
			printf("tour is NULL\n");
			continue;
		}
		for (int j = 0; j < tmp->count; j++) {
			printf("%d ", tmp->cities[j]);\
		}
		printf("\n");
	}
}	/* Print_stack */


/*-----------------------------------------------------------------------------
 * init stack
 */
stack_t Init_stack() {
	stack_t stack = malloc(sizeof(stack_struct));
	stack->list = malloc(n*n*sizeof(tour_t));
	for(int i = 0; i < n*n; i++)
		stack->list[i] = NULL;
	stack->sz = 0;
	return stack;
}	/* Init_stack */


/*----------------------------------------------------------------------------
 * alloc mem for tour
 */
tour_t Alloc_tour() {
	tour_t tour = malloc(sizeof(tour_struct));
	tour->cities = malloc((n+1)*sizeof(city_t));
	return tour;
}	/* Alloc_tour */


/*-----------------------------------------------------------------------------
 * init tour
 */
tour_t Init_tour(cost_t cost) {
	tour_t tour = Alloc_tour();
	tour->count = 1;
	tour->cost = cost;
	tour->cities[0] = home_town;
	for(int i = 1; i < n+1; i++) {
		tour->cities[i] = NO_CITY;
	}
	return tour;
}	/* Init_tour */


/*----------------------------------------------------------------------------
 * determine if the stack is empty
 */
int Empty(stack_t stack) {
	if(stack == NULL || stack->sz == 0) {
		return TRUE;
	}
	return FALSE;
}	/* Empty */


/*----------------------------------------------------------------------------
 * create a duplicate tour
 */
void Copy_tour(tour_t dest_tour, tour_t source_tour) {
	memcpy(dest_tour->cities, source_tour->cities, (n+1)*sizeof(city_t));
	dest_tour->count = source_tour->count;
	dest_tour->cost = source_tour->cost;
}	/* Copy_tour */


/*----------------------------------------------------------------------------
 * push tour to stack(copy)
 */
void Push_copy(stack_t stack, tour_t tour) {
	// determine if the stack is overflow
	if(stack->sz >= n*n) {
		fprintf(stderr, "stack overflow!\n");
		exit(-1);
	}

	tour_t tmp_tour;
	tmp_tour = Alloc_tour();
	Copy_tour(tmp_tour, tour);
	// push tour to stack
	stack->list[(stack->sz)++] = tmp_tour;
}	/* Push_copy */


/*----------------------------------------------------------------------------
 * pop
 */
tour_t Pop(stack_t stack) {
	// determine if the stack is empty
	if(Empty(stack)) {
		fprintf(stderr, "stack is empty!\n");
		exit(-1);
	}
	// pop
	tour_t tmp_tour = stack->list[--stack->sz];
	stack->list[stack->sz] = NULL;
	return tmp_tour;
}	/* Pop */


/*----------------------------------------------------------------------------
 * add city to tour 
 */
void Add_city(tour_t tour, city_t city) {
	city_t old_last_city = Last_city(tour);
	tour->cities[tour->count++] = city;
	tour->cost += Cost_a2b(old_last_city, city);
}	/* Add_city */


/*----------------------------------------------------------------------------
 * update best tour
 */
void Update_best_tour(tour_t tour) {
	Copy_tour(best_tour, tour);
	Add_city(best_tour, home_town);
}	/* Update_best_tour */


/*----------------------------------------------------------------------------
 * determine if the city isn't visited
 */
int Unvisit(tour_t tour, city_t city) {
	for(int i = 0; i < tour->count; i++) {
		if(tour->cities[i] == city) {
			return FALSE;
		}
	}
	return TRUE;
}	/* Unvisit */


/*----------------------------------------------------------------------------
 * detemine a nbr city is feasible(the cost is less than the best tour when add this city or this city have visited)
 */
int Feasible(tour_t tour, city_t city) {
	city_t last_city = Last_city(tour);
	if(Unvisit(tour, city) && Cost_a2b(last_city, city) + tour->cost < best_tour->cost) {
		return TRUE;
	}
	return FALSE;
}	/* Feasible */


/*----------------------------------------------------------------------------
 * remove the last city of tour
 */
void Remove_last_city(tour_t tour) {
	city_t old_last_city = Last_city(tour);

	tour->cities[--(tour->count)] = NO_CITY;
	city_t new_last_city = Last_city(tour);
	tour->cost -= Cost_a2b(new_last_city, old_last_city);
}	/* Remove_last_city */


/*----------------------------------------------------------------------------
 * release mem
 */
void Free_stack(stack_t stack) {
	free(stack->list);
	free(stack);
}	/* Free_stack */


/*----------------------------------------------------------------------------
 * release mem of tour
 */
void Free_tour(tour_t tour) {
	free(tour->cities);
	free(tour);
}	/* Free_tour */


/*----------------------------------------------------------------------------
 * kernal
 */
void Find_best_tour() {
	tour_t tmp_tour;
	tour_t init_tour;
	stack_t stack;
	
	stack = Init_stack();
	init_tour = Init_tour(0);
	best_tour = Init_tour(INFINITY);
	
	Push_copy(stack, init_tour);
	while(!Empty(stack)) {
#	ifdef DEBUG
		Print_stack(stack);
		Print_tour(best_tour);
		printf("\n");
#	endif
		tmp_tour = Pop(stack);
		if(tmp_tour->count == n && tmp_tour->cost + Cost_a2b(Last_city(tmp_tour), home_town) < best_tour->cost) {
			// update best tour if number of this tour is equal to n and cost is less than the best tour
			Update_best_tour(tmp_tour);
		} else {
			for(city_t i = n-1; i >= 1; i--) {
				if (Feasible(tmp_tour, i)) {
					Add_city(tmp_tour, i);
					Push_copy(stack, tmp_tour);
					Remove_last_city(tmp_tour);
				}
			}
		}
		Free_tour(tmp_tour);
	}

	Free_stack(stack);
}	/* Find_best_tour */


/*--------------------------------------------------------------------------*/
int main(int argc, char* argv[]) {
	// argc is number of argv, argv[0] is name of this proc(tsp)
	if (argc < 1) {
		fprintf(stderr, "Please use 'tsp filename'!\n");
		exit(-1);
	}

	// read file
	Read_digraph(argv[1]);

#	ifdef DEBUG
	Print_digraph();
#	endif

	Find_best_tour();
	Print_tour(best_tour);

	return 0;
}	/* main */
