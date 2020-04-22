#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

int ** map ;
int * prefix_arr ;

int search_count = 0;
int size = 0;
int prefix_size = 0;
int process_bound_at_a_time ;
const int * SIZE = &size;
const int * PS = &prefix_size;
const int * PB = &process_bound_at_a_time ;

int * children_num;
int pipes[2] ;

void terminate_handler(int sig){
	printf("%d\n", *PS);
	exit(0);	
}

void child_handler(int sig){

	printf("gueeeeegueee\n");
	exit(0);
}

void swap(int * a, int * b){
	int temp = *a;
	*a = *b;
	*b= temp;
}

void heap_permutation(int * arr, int s, void(*routine)(int * _arr)){	
	if(s == 1){
		routine(arr);
		return; 
	} 
	
	for(int i= 0 ; i < s ; i++){
		heap_permutation(arr,s-1,routine);
		if(i< s -1){
			if(s%2 ==1)
				swap(&arr[0], &arr[s-1]);
		} else {
			swap(&arr[i], &arr[s-1]);
		}
	}
}


void parent_routine(int * arr){
	if(*children_num < *PB){
		*children_num += 1;
		search_count++;
		printf("%d, %d\n", *children_num, search_count);
		pid_t child ;
		child = fork();
		if(child == 0){
			key_t key = ftok("shmfile",65);
			int shmid = shmget(key,1024,0666|IPC_CREAT);
			children_num = (int*) shmat(shmid,(void*)0,0);

			signal(SIGINT, child_handler);		
			sleep(2);
			close(pipes[0]);
			ssize_t sent = 0;
			char * data = "If I die tomorrow" ;
			ssize_t s = strlen(data);
			//data[s+1] = 0x0;
			while(sent < s){
				sent += write(pipes[1], data+sent, s-sent);	
			}
			*children_num -= 1;
			shmdt(children_num);
			close(pipes[1]);
			exit(0);	
		}
  
	} else {
		while(*children_num >= *PB){
		}
		parent_routine(arr);
	} 
}

void children_routine(int * arr){
	search_count ++; 
	printf("%d| (", search_count); 
	for(int i =0 ; i< *PS ; i++){
		printf("%d ", arr[i]);
	}
	printf("%d)\n", arr[0]);
	return;
}

void (*PARENT_ROUTINE)(int * _arr) = parent_routine ;
void (*CHILDEREN_ROUTINE)(int * _arr) = children_routine;



void start(){
	prefix_arr = (int*) malloc(sizeof(int)*(*PS)) ;
	for(int i = 0 ; i < *PS ; i++)
		prefix_arr[i] = i;
	heap_permutation(prefix_arr, *PS, *PARENT_ROUTINE); 	
}


int main(int argc, char** argv){

	if(argc < 3) {
		fprintf(stderr,"please input file path and the number of processes\n");
		return -1;
	}
	process_bound_at_a_time = atoi(argv[2]) ;
	if(*PB < 1 || *PB > 12){
		fprintf(stderr,"please input the number of processes  from 1 to 12\n");
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
	
	prefix_size = size - 12 ; 
	fclose(fp);
	//until here, for knowing size
	map = (int **) malloc(sizeof(int*) * (*SIZE));
	for(int i = 0 ; i< *SIZE ; i++){
		map[i] = (int *) malloc(sizeof(int) * (*SIZE));
	}
	fp = fopen(argv[1], "r");
	for(int i = 0 ; i< *SIZE ; i++){
		for(int j = 0 ; j< *SIZE ; j++){
			fscanf(fp, "%d", &map[i][j]);
		}
	}
	fclose(fp);
	//until here, for receving input
	signal(SIGINT,terminate_handler);

	if(pipe(pipes) != 0){
		fprintf(stderr,"Pipe Create Error\n");
	}
	
	key_t key = ftok("shmfile",65);
	int shmid = shmget(key,1024,0666|IPC_CREAT);
	children_num = (int*) shmat(shmid,(void*)0,0);
	*children_num = 0;

	pid_t slave ;
	int exit_code;
	slave = fork();
	if(slave==0){
		start();
	} else /*master*/{
		char buf[32] ;
		ssize_t s;
		close(pipes[1]);
		while(1){	
			while((s = read(pipes[0], buf, 31))> 0){
				buf[s+1] = 0x0;
				printf("%s\n",buf);
			}
		}	
		close(pipes[0]);	
	}
	wait(&exit_code);
	
	return 0;
}
