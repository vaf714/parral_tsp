#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "timer.h"

/* define some shortcut for struct */
#define City_count(tour) (tour->count)
#define Tour_cost(tour) (tour->cost)
#define Last_city(tour) (tour->cities[tour->count-1])
#define Tour_city(tour, i) (tour->cities[i])
#define Cost_a2b(a, b) (digraph[a*n+b])
#define Queue_elt(queue, i) (queue->list[(queue->head + (i)) % queue->list_alloc])

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
typedef stack_struct* my_stack_t;

typedef struct {
	tour_t* list;
	int list_alloc;
	int head;
	int tail;
	int full;
} queue_struct;
typedef queue_struct* queue_t;

typedef struct {
	int curr_tc;
	int max_tc;
	pthread_mutex_t mutex;
	pthread_cond_t ok_to_go;
} barrier_struct;
typedef barrier_struct* my_barrier_t;

typedef struct {
	my_stack_t new_stack;
	int wait_threads_count;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
} term_struct;
typedef term_struct* term_t;


int n;				// city number
int* digraph;		// digraph
city_t home_town = 0;
tour_t best_tour;
int thread_count;
int min_split_sz;
my_barrier_t barrier;
term_t term;
pthread_mutex_t best_tour_mutex;
pthread_mutex_t print_mutex;
int queue_size;
queue_t queue;
int init_tour_count;

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
void Print_stack(int rank, my_stack_t stack) {
	pthread_mutex_lock(&print_mutex);
	if (stack == NULL || stack->sz == 0) {
		fprintf(stderr, "stack is NULL\n");
		return;
	}
	tour_t tmp;
	printf("print stack(rank%d):\n", rank);
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
	pthread_mutex_unlock(&print_mutex);
}	/* Print_stack */


/*-----------------------------------------------------------------------------
 * init stack
 */
my_stack_t Init_stack() {
	my_stack_t stack = malloc(sizeof(stack_struct));
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
int Empty(my_stack_t stack) {
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
void Push_copy(my_stack_t stack, tour_t tour) {
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
tour_t Pop(my_stack_t stack) {
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


int Best_tour(tour_t tour) {
	if (Tour_cost(tour) + Cost_a2b(Last_city(tour), home_town) < Tour_cost(best_tour))
		return TRUE;
	else 
		return FALSE;
}


/*----------------------------------------------------------------------------
 * update best tour
 */
void Update_best_tour(tour_t tour) {
	pthread_mutex_lock(&best_tour_mutex);
	if (Best_tour(tour)) {
		Copy_tour(best_tour, tour);
		Add_city(best_tour, home_town);
	}
	pthread_mutex_unlock(&best_tour_mutex);
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
void Free_stack(my_stack_t stack) {
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


queue_t Init_queue(int size) {
	queue_t new_queue = malloc(sizeof(queue_struct));
	new_queue->list = malloc(size*sizeof(tour_t));
	new_queue->list_alloc = size;
	new_queue->head = new_queue->tail = new_queue->full = 0;
	return new_queue;
}

void Enqueue(queue_t queue, tour_t tour) {
	tour_t tmp;

	if (queue->full == TRUE) {
		fprintf(stderr, "queue is full, can't enqueue!\n");
		exit(-1);
	}
	tmp = Alloc_tour();
	Copy_tour(tmp, tour);
	queue->list[queue->tail] = tmp;
	queue->tail = (queue->tail + 1) % queue->list_alloc;
	if (queue->tail == queue->head) {
		queue->full = TRUE;
	}
}

int Empty_queue(queue_t queue) {
	if (queue->full == TRUE) return FALSE;
	else if (queue->head != queue->tail) return FALSE;
	else return TRUE;
}


tour_t Dequeue(queue_t queue) {
	tour_t tmp;
	if (Empty_queue(queue)) {
		fprintf(stderr, "queue is empty, can't dequeue!\n");
		exit(-1);
	}
	tmp = queue->list[queue->head];
	queue->head = (queue->head + 1) % queue->list_alloc;
	return tmp;
}


void Build_initial_queue() {
	int curr_sz = 0;
	city_t nbr;
	tour_t tour =  Init_tour(0);

	queue = Init_queue(2*queue_size);

	Enqueue(queue, tour);

	Free_tour(tour);
	curr_sz++;
	while (curr_sz < thread_count) {
		tour = Dequeue(queue);
		curr_sz--;
		for(nbr = 1; nbr < n; nbr++) {
			if (Unvisit(tour, nbr)) {
				Add_city(tour, nbr);
				Enqueue(queue, tour);
				curr_sz++;
				Remove_last_city(tour);
			}
		}
		Free_tour(tour);
	}
	init_tour_count = curr_sz;
}


long long Fact(int k) {
	long long tmp = 1;
	int i;

	for (i = 2; i <= k; i++) {
		tmp *= i;
	}
	return tmp;
}


int Get_upper_bd_queue_sz() {
	int fact = n - 1;
	int size = n - 1;

	while (size < thread_count) {
		fact++;
		size *= fact;
	}

	if (size > Fact(n-1)) {
		fprintf(stderr, "threads are too many!\n");
		size = 0;
	}
	return size;
}


void Set_init_tours(long rank, int* first_tour, int* last_tour) {
	int quotient, remainder, my_count;

	quotient = init_tour_count/thread_count;
	remainder = init_tour_count%thread_count;
	if (rank < remainder) {
		my_count = quotient + 1;
		*first_tour = rank * my_count;
	} else {
		my_count = quotient;
		*first_tour = rank * my_count + remainder;
	}
	*last_tour = *first_tour + my_count - 1;
}


void My_barrier(my_barrier_t bar) {
	pthread_mutex_lock(&bar->mutex);
	bar->curr_tc++;
	if (bar->curr_tc == bar->max_tc) {
		bar->curr_tc = 0;
		pthread_cond_broadcast(&bar->ok_to_go);
	} else {
		while (pthread_cond_wait(&bar->ok_to_go, &bar->mutex) != 0);
	}
	pthread_mutex_unlock(&bar->mutex);
}


void Partition_tree(long rank, my_stack_t stack) {
	int my_first_tour, my_last_tour, i;

	if (rank == 0) {
		queue_size = Get_upper_bd_queue_sz();
	}
	
	My_barrier(barrier);
	if(queue_size == 0) {
		pthread_exit(NULL);
	}
	if (rank == 0) {
		Build_initial_queue();
	}
	My_barrier(barrier);

	Set_init_tours(rank, &my_first_tour, &my_last_tour);
	
	for (i = my_last_tour; i >= my_first_tour; i--) {
		Push_copy(stack, Queue_elt(queue, i));
	}

#	ifdef DEBUG
	Print_stack(rank, stack);
#	endif
} 




my_stack_t Split_stack(my_stack_t stack, long my_rank) {
	int old_source, new_dest, old_dest;
	my_stack_t new_stack = Init_stack();
	new_dest = 0;
	old_dest = 1;
	for(int new_source = 1; new_source < stack->sz; new_source += 2){
		old_source = new_source + 1;
		new_stack->list[new_dest++] = stack->list[new_source];
		if (old_source < stack->sz) {
			stack->list[old_dest++] = stack->list[old_source];
		}
		stack->sz = old_dest;
		new_stack->sz = new_dest;
	}
	return new_stack;
}


int Terminated(my_stack_t* stack_p, long my_rank) {
	my_stack_t stack = *stack_p;
	int got_lock;

	if (stack->sz >= min_split_sz && term->wait_threads_count > 0 && term->new_stack == NULL) {
		got_lock = pthread_mutex_trylock(&term->mutex);
		if (got_lock == 0) {
			if (term->wait_threads_count > 0 && term->new_stack == NULL) {
				term->new_stack = Split_stack(stack, my_rank);
				pthread_cond_signal(&term->cond);
			}
			pthread_mutex_unlock(&term->mutex);
		}
		return FALSE;
	} else if (!Empty(stack)) {
		return FALSE;
	} else {
		pthread_mutex_lock(&term->mutex);
		if (term->wait_threads_count == thread_count-1) {
			term->wait_threads_count++;
			pthread_cond_broadcast(&term->cond);
			pthread_mutex_unlock(&term->mutex);
			Free_stack(stack);
			return TRUE;
		} else {
			Free_stack(stack);
			term->wait_threads_count++;
			while(pthread_cond_wait(&term->cond, &term->mutex) != 0);
			if (term->wait_threads_count < thread_count) {
				if (term->new_stack != NULL) {
					*stack_p = stack = term->new_stack;
					term->new_stack = NULL;
					term->wait_threads_count--;
					pthread_mutex_unlock(&term->mutex);
					return FALSE;
				} else {
					term->wait_threads_count--;
					pthread_mutex_unlock(&term->mutex);
					fprintf(stderr, "Th %ld > Awakened with no work avail!\n", my_rank);
					pthread_exit(NULL);
				}
			} else {
				pthread_mutex_unlock(&term->mutex);
				return TRUE;
			}
		} 
	}
}


void Free_queue(queue_t queue) {
	free(queue->list);
	free(queue);
}


/*----------------------------------------------------------------------------
 * kernal
 */
void* Find_best_tour(void* rank) {
	long my_rank = (long) rank;
	my_stack_t my_stack;
	tour_t tmp_tour;
	
	my_stack = Init_stack();
	Partition_tree(my_rank, my_stack);

	while(!Terminated(&my_stack, my_rank)) {
		tmp_tour = Pop(my_stack);
		if(tmp_tour->count == n) {
			if(Best_tour(tmp_tour)) {
				// update best tour if number of this tour is equal to n and cost is less than the best tour
				Update_best_tour(tmp_tour);
			}
		} else {
			for(city_t i = n-1; i >= 1; i--) {
				if (Feasible(tmp_tour, i)) {
					Add_city(tmp_tour, i);
					Push_copy(my_stack, tmp_tour);
					Remove_last_city(tmp_tour);
				}
			}
		}
		Free_tour(tmp_tour);
	}

	// Free_stack(my_stack);
	if (my_rank == 0) Free_queue(queue);
	return NULL;
}	/* Find_best_tour */


my_barrier_t Barrier_init(int thread_count) {
	my_barrier_t bar = malloc(sizeof(barrier_struct));
	bar->curr_tc = 0;
	bar->max_tc = thread_count;
	pthread_mutex_init(&bar->mutex, NULL);
	pthread_cond_init(&bar->ok_to_go, NULL);
	return bar;
}


void Init_term() {
	term = malloc(sizeof(term_struct));
	term->new_stack = NULL;
	term->wait_threads_count = 0;
	pthread_cond_init(&term->cond, NULL);
	pthread_mutex_init(&term->mutex, NULL);
}


void My_barrier_destroy(my_barrier_t bar) {
	pthread_mutex_destroy(&bar->mutex);
	pthread_cond_destroy(&bar->ok_to_go);
	free(bar);
}


void Free_term() {
	pthread_cond_destroy(&term->cond);
	pthread_mutex_destroy(&term->mutex);
	free(term);
}

/*--------------------------------------------------------------------------*/
int main(int argc, char* argv[]) {
	double start_time, end_time;
	long thread;
	pthread_t* thread_handles;

	// argc is number of argv, argv[0] is name of this proc(tsp)
	if (argc < 4) {
		fprintf(stderr, "Please usage: %s <thread_num> <digraph file> <min split size>\n", argv[0]);
		exit(-1);
	}
	thread_count = strtol(argv[1], NULL, 10);
	min_split_sz = strtol(argv[3], NULL, 10);
	
	// read file
	Read_digraph(argv[2]);
#	ifdef DEBUG
	Print_digraph();
#	endif

	thread_handles = malloc(thread_count*sizeof(pthread_t));
	barrier = Barrier_init(thread_count);
	pthread_mutex_init(&best_tour_mutex, NULL);
	pthread_mutex_init(&print_mutex, NULL);
	Init_term();

	
	best_tour = Init_tour(INFINITY);
	
	GET_TIME(start_time);
	for (thread = 0; thread < thread_count; thread++) {
		pthread_create(&thread_handles[thread], NULL, Find_best_tour, (void*) thread);
	}
	for (thread = 0; thread < thread_count; thread++) {
		pthread_join(thread_handles[thread], NULL);
	}
	GET_TIME(end_time);

	Print_tour(best_tour);
	printf("Elapsed time is: %e s\n", end_time - start_time);
	
	// release mem
	Free_tour(best_tour);
	free(thread_handles);
	free(digraph);
	My_barrier_destroy(barrier);
	pthread_mutex_destroy(&best_tour_mutex);
	pthread_mutex_destroy(&print_mutex);
	Free_term();	
	return 0;
}
