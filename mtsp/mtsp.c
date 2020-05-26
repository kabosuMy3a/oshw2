#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

int ** map ;
int * real_arr;

long long sci = 0;
int * minpath ;
int min = -1;
int current_thread_num = 0;

int size = 0;
int prefix_size = 0;
int process_bound_at_a_time ;

const int * SIZE = &size;
const int * PS = &prefix_size;
const int * PB = &process_bound_at_a_time ;

pthread_t threads[8] ;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER ;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER ;

typedef struct {
        sem_t filled ;
        sem_t empty ;
        pthread_mutex_t lock ;
        int ** elem ;
        int capacity ;
        int num ;
        int front ;
        int rear ;
} bounded_buffer ;

bounded_buffer * buf = 0x0 ;

void
bounded_buffer_init(bounded_buffer * buf, int capacity) {
        sem_init(&(buf->filled), 0, 0) ;
        sem_init(&(buf->empty), 0, capacity) ;
        pthread_mutex_init(&(buf->lock), 0x0) ;
        buf->capacity = capacity ;
        buf->elem = (int **) calloc(sizeof(int *), capacity) ;
        buf->num = 0 ;
        buf->front = 0 ;
        buf->rear = 0 ;
}

void
bounded_buffer_enqueue(bounded_buffer * buf, int * arr)
{
        sem_wait(&(buf->empty)) ; //dec or wait-and-then-dec
        //--Semaphore gurantee that there is empty spot
        pthread_mutex_lock(&(buf->lock)) ; 
                buf->elem[buf->rear] = arr ;
                buf->rear = (buf->rear + 1) % buf->capacity ;
                buf->num += 1 ;
        pthread_mutex_unlock(&(buf->lock)) ;
        sem_post(&(buf->filled)) ; //increase filled
}

int *
bounded_buffer_dequeue(bounded_buffer * buf)
{
        int * r = 0x0 ;
        sem_wait(&(buf->filled)) ; //dec or wait-and-then-dec
        //Semaphore gurantee that there is item in buffer
        pthread_mutex_lock(&(buf->lock)) ;
                r = buf->elem[buf->front] ;
                buf->front = (buf->front + 1) % buf->capacity ;
                buf->num -= 1 ;
        pthread_mutex_unlock(&(buf->lock)) ;
        sem_post(&(buf->empty)) ; //increase empty
        return r ;
}

void print_result(){
        printf("\nMIN %d (", min) ;
        for (int i = 0 ; i < *SIZE ; i++)
                printf("%d ", minpath[i]) ;
        printf("%d) - %lld checked\n", minpath[0],sci) ;

}


void terminate_handler(int sig){
	print_result();
	exit(0);
}


void swap(int * a, int * b){
	int temp = *a;
	*a = *b;
	*b= temp;
}

void producer_routine(int * arr);

void _permutation_recurr(int * arr, int depth){
	if(depth== *PS){
		producer_routine(arr);
	} else {
		int i;
		for(int i = depth ; i < *SIZE ; i++){
			swap(arr+i, arr+depth);
			_permutation_recurr(arr, depth+1);
			swap(arr+i, arr+depth);
		}
	}
}

void *permutation_starter(void * arr){
	_permutation_recurr((int*)arr, 0);
}


long * customer_tids ;
int * subtasks ;
int * searched_nums ;
int * current_item_from_buffer[8] ;

void _travel(int idx, int * arr, int* prefix_arr, int* offset, int *path, int *used, int length){

	int i ;

	if (idx == 11){

		for (i = 0 ; i< *PS ; i++){
			if(i != *PS -1)
				length += map[prefix_arr[i]][prefix_arr[i+1]];
			else 
				length += map[prefix_arr[i]][path[0]];
		}

		for(i=0 ; i< 10 ; i++){
                        length += map[path[i]][path[i+1]];
                }

		length += map[path[10]][prefix_arr[0]];
		pthread_mutex_lock(&m);
			sci++;
			searched_nums[*offset] += 1; 
			if (min == -1 || min > length){
				min = length ;
					
				for(i = 0; i < *PS ; i++){
					minpath[i] = prefix_arr[i];
				}
				for(i = 0; i < 11 ; i++){
					minpath[i+*PS] = path[i] ; 
				}	
			}
		pthread_mutex_unlock(&m);

		length -= map[path[10]][prefix_arr[0]] ;

		for(i=0 ; i< 10 ; i++){
                        length -= map[path[i]][path[i+1]];
                }

		for (i = 0 ; i< *PS ; i++){
			if(i != *PS -1)
				length -= map[prefix_arr[i]][prefix_arr[i+1]];
			else 
				length -= map[prefix_arr[i]][path[0]];
		}

	} else {
		for( i = 0; i< 11 ; i++){
			if(used[i] == 0){
				path[idx] = arr[i+*PS];
				used[i] = 1;
				_travel(idx+1,arr,prefix_arr, offset, path, used, length);
				used[i] =0;
			}		
		}
	}
}

void travel(int start, int *arr, int * prefix_arr, int* offset){ 
	int path[11] ;
	int used[11] ;
	int length = 0 ;
	path[0] = arr[*PS+start] ;
	used[start] = 1;
	_travel(1,arr,prefix_arr, offset, path, used, length);
	used[start] = 0;

}

void* customer_routine(void * OFFSET){
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,0x0);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,0x0);
	int * offset = (int*) OFFSET ; 
	int * arr = 0x0;
	customer_tids[*offset] = pthread_self();
	arr = bounded_buffer_dequeue(buf);
	searched_nums[*offset] = 0;

	current_item_from_buffer[*offset] = arr; 

	int * prefix_arr = (int*) malloc(sizeof(int) * *PS);
	for(int i = 0 ; i < *PS ; i++){
		prefix_arr[i] = arr[i] ;
	}
	for(int i = 0 ; i < 11 ; i++){
		travel(i, arr,prefix_arr, offset);
	}
	free(prefix_arr);
	subtasks[*offset] += 1;
	customer_routine(offset);
}

void producer_routine(int * arr){
	int * item = (int*)malloc(sizeof(int) * *SIZE);
	for(int i = 0 ; i < *SIZE ; i++) item[i] = arr[i];
	bounded_buffer_enqueue(buf, item);
}

void stat_result(){
        printf("\nMIN %d (", min) ;
        for (int i = 0 ; i < *SIZE ; i++)
                printf("%d ", minpath[i]) ;
        printf("%d) - %lld checked\n", minpath[0], sci) ;

}

void stat_fun(){
	stat_result();	
}

void threads_fun(){
	for(int i =0 ; i < current_thread_num ; i++)
		printf("Tid: %ld, processed subtasks: %d, current route: %d\n"
			,customer_tids[i], subtasks[i], searched_nums[i] ); 
}

void num_fun(int num){
	//current_thread_num //*PB
	if (num > current_thread_num){
		for(int i = current_thread_num; i < num ; i++){
			int * offset = (int*) malloc(sizeof(int)) ;
			*offset = i ;
			current_thread_num = i+1;
			subtasks[i] = 0;
			searched_nums[i] = 0;
			current_item_from_buffer[i] = 0x0;
			pthread_create(&threads[i], NULL, customer_routine, (void *)offset);	
		}
		current_thread_num = num;
	} else if (num < current_thread_num){
		for(int i = num ; i < current_thread_num ; i++){
			pthread_cancel(threads[i]);
			threads[i] = 0x0;
//	The fault of this program.	
//			pthread_mutex_lock(&m);
//				sci - searched_nums[i]; 
//			pthread_mutex_lock(&m);
//			if(current_item_from_buffer[i] != 0x0){ 
//				int * item = (int*)malloc(sizeof(int) * *SIZE);
//				for(int j = 0 ; j < *SIZE ; j++) 
//					item[j] = current_item_from_buffer[i][j];
//				current_item_from_buffer[i] = 0x0;
//				bounded_buffer_enqueue(buf, item);
//			} 
// 		
		}
		current_thread_num = num;
	} 
}

void _help(){
	printf("---Help---\n");
	printf("stat: \n");
	printf("threads: \n");
	printf("num N: \n");	
}

void* console(){
	while(1){
		char a[100] ;
		fgets(a,100,stdin);
		if(a[0]=='s'){stat_fun();continue;}//debug
		if(strcmp(a,"stat\n")==0){
			stat_fun();
			continue;
		} else if(strcmp(a, "threads\n")==0){
			threads_fun();
			continue;
		} 
		char b[4];
		strncpy(b, a, 3);
		b[3] = 0x0;
		if(strcmp(b,"num")==0){
			int c = a[4]-'0';
			if(c>0 && c<=8) num_fun(c); 
			else printf("input N, 1<= N <=8\n");
			continue;
		} 
		else _help();
	}
}

int produce_complete_flag = 0;
void sigusr_handler(int sig){
	produce_complete_flag = 1;
}

void start(){
	real_arr = (int*) malloc(sizeof(int)*(*SIZE));
	for(int i =0 ; i < *SIZE ; i++){
		real_arr[i] = i;	
	}
	
	buf = (bounded_buffer *) malloc(sizeof(bounded_buffer));
	bounded_buffer_init(buf,16);
	pthread_t producer_thread ;

	pthread_t command_thread ;
	pthread_create(&producer_thread, NULL, permutation_starter, (void*)real_arr);
	pthread_create(&command_thread, NULL, console, 0x0);

	signal(SIGUSR1, sigusr_handler);//for termination

	customer_tids = (long*) malloc(sizeof(long)* 8) ;
	subtasks = (int*) malloc(sizeof(int) * 8);
	searched_nums = (int*) malloc(sizeof(int)* 8);

	for(int i = 0 ; i < *PB ; i++){
		int * offset = (int*) malloc(sizeof(int)) ;
		*offset = i ;
		current_thread_num = i+1;
		subtasks[i] = 0;
		searched_nums[i] = 0;
		current_item_from_buffer[i] =0x0;
		pthread_create(&threads[i], NULL, customer_routine, (void *)offset);	
	}
	for(int i = 0 ; i< *PB ; i++){
		pthread_join(threads[i], 0x0);
	}
	free(real_arr);
}


int main(int argc, char** argv){

	if(argc < 3) {
		fprintf(stderr,"please input file path and the number of threads\n");
		return -1;
	}
	process_bound_at_a_time = atoi(argv[2]) ;
	if(*PB < 1 || *PB > 8){
		fprintf(stderr,"please input the number of threads from 1 to 8\n");
		return -1;
	}
	//for knowing size
	FILE * fp ;
	fp = fopen(argv[1], "r");
	if(fp == 0x0){
		fprintf(stderr,"Input right file path\n");
		return -1;
	}
	char * lines = 0x0;
	size_t buf = 0;
	getline(&lines, &buf, fp) ;
	for(int i =0 ; i<strlen(lines) ; i++){
		if(lines[i] == ' ' || (lines[i] == '\n' && lines[i-1] != ' ')) size++;
	}
	if(*SIZE <13 || *SIZE > 50){
		fprintf(stderr,"Cities size n should be 13 <= n <= 50\n");
		return -1;
	}
	free(lines);
	prefix_size = size - 11 ; 
	fclose(fp);
	
	//until here, for knowing size
	map = (int **) malloc(sizeof(int*) * (*SIZE));
	for(int i = 0 ; i< *SIZE ; i++){
		map[i] = (int *) malloc(sizeof(int) * (*SIZE));
	}
	FILE * fc;
	fc = fopen(argv[1], "r");
	if(fc == 0x0){
		fprintf(stderr,"Input right file path\n");
		return -1;
	}
	for(int i = 0 ; i< *SIZE ; i++){
		for(int j = 0 ; j< *SIZE ; j++){
			fscanf(fc, "%d", &map[i][j]);
		}
	}	
	minpath = (int*) malloc(sizeof(int) * (*SIZE+1));
	fclose(fc);
	//LOGIC START
	signal(SIGINT, terminate_handler);
	start();
	print_result();
	//LOGIC END
	free(minpath);
	for(int i= 0 ; i< *SIZE ; i++){
		free(map[i]);
	}
	free(map);
	
	return 0;
}
