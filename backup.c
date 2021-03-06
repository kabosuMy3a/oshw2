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

int ** map ;
int * real_arr;
int * prefix_arr ;

int * minpath ;
int min = -1;
int search_count = 0;

int size = 0;
int prefix_size = 0;
int process_bound_at_a_time ;
int * children_num; //shared memory

const int * SIZE = &size;
const int * PS = &prefix_size;
const int * PB = &process_bound_at_a_time ;

int pipes[2] ;
int pipe_offset = 0 ;
pid_t master ;

void send_result(){
	close(pipes[0]);
	char data[256];
	char temp[24]; 	
	sprintf(data, "m%d (", min );
	for(int i = 0; i <*SIZE ; i++){
		sprintf(temp,"%d ", minpath[i]);
		strcat(data, temp); 
	}
	sprintf(temp, ")- c%d ", search_count);
	strcat(data,temp);
	write(pipes[1], data, strlen(data));
	close(pipes[1]);
	
} 

void print_result(){
	printf("\nDUDUDUNGA! %d (", min) ;
        for (int i = 0 ; i < *SIZE ; i++)
                printf("%d ", minpath[i]) ;
        printf("%d) - %d checked\n", minpath[0],search_count) ;

}

void receive_result(){
	char buf[257] ;
	ssize_t s;
	close(pipes[1]);
	if((s = read(pipes[0], buf, 256))> 0){
		buf[s+1] = 0x0;
	}
	printf("%s-cut\n",buf);
}

void receive_for_termination(){
	char buf[12][257] ;
	ssize_t s;
	close(pipes[1]);
	int i = 0;
	while((s = read(pipes[0], buf[i], 256))> 0){
		buf[i][s+1] = 0x0;
		i++;	
	}
	for(int j = 0; j <i ; j++){
		printf("%s-cut\n",buf[j]);
	}
}

void sigusr_handler(int sig){
	if(sig == SIGUSR1){	
		receive_result();
	}
}

void terminate_handler(int sig){
	exit(0);	
}

void child_handler(int sig){
	send_result();
	//print_result();
	exit(0);
}

void swap(int * a, int * b){
	int temp = *a;
	*a = *b;
	*b= temp;
}

void _permutation_recurr(int * arr, int depth, void(*routine)(int *_arr)){
	if(depth== *PS){
		routine(arr);
	} else {
		int i;
		for(int i = depth ; i < *SIZE ; i++){
			swap(arr+i, arr+depth);
			_permutation_recurr(arr, depth+1,routine);
			swap(arr+i, arr+depth);
		}
	}
}

void permutation_starter(int * arr, void(*routine)(int *_arr)){
		_permutation_recurr(arr, 0, routine);
}


void just_print_routine(int *arr){
	search_count++;
	printf("%d (",search_count);
	for(int i= 0; i< *SIZE ; i++){
		printf("%d ",real_arr[i]);
	}
	printf(")\n");
}

void (*JUST_PRINT_ROUTINE)(int *arr) = just_print_routine;

int path[12] ;
int used[12] ;
int length = 0 ;

void _travel(int idx){

	int i ;
	
	if (idx == 12){
		search_count ++;
		length += map[path[11]][path[0]];
		for (i = 0 ; i< *PS ; i++){
			if(i != *PS -1)
				length += map[prefix_arr[i]][prefix_arr[i+1]];
			else 
				length += map[prefix_arr[i]][prefix_arr[0]];
		}
		if (min == -1 || min > length){
			min = length ;
				
			for(i = 0; i < *PS ; i++){
				minpath[i] = prefix_arr[i];
			}
			for(i = 0; i < 12 ; i++){
				minpath[i+*PS] = path[i] ; 
			}	
		}
		if (search_count % 50000000 == 0){	
			printf("%d| %d (", search_count, length);
			for(i =0 ; i < *PS ; i++){
				printf("%d ", prefix_arr[i]);
			}
			for( i = 0; i< 12 ; i++)
				printf("%d ", path[i]);
			printf("%d)\n", prefix_arr[0]);
		}
	
		length -= map[path[11]][path[0]] ;
		for (i = 0 ; i< *PS ; i++){
			if(i != *PS -1)
				length -= map[prefix_arr[i]][prefix_arr[i+1]];
			else 
				length -= map[prefix_arr[i]][prefix_arr[0]];
		}

	} else {
		for( i = 0; i< 12 ; i++){
			if(used[i] == 0){
				path[idx] = real_arr[i+*PS];
				used[i] = 1;
				length += map[path[idx-1]][i]; 
				_travel(idx+1);
				length -= map[path[idx-1]][i];
				used[i] =0;
			}		
		}
	}
}

void travel(int start){ 
	path[0] = real_arr[*PS] ;
	used[start] = 1;
	_travel(1);
	used[start] = 0;

}

void children_routine(int *arr){
	for(int i = 0 ; i < 12 ; i++){
		travel(i);
	}	
}

void (*CHILDREN_ROUTINE)(int * _arr) = children_routine;

void parent_routine(int * arr){
	if(*children_num < *PB){
		for(int i = 0 ; i < *PS ; i++){
			prefix_arr[i] = real_arr[i] ;
		}
		*children_num += 1;
		printf("%d\n", *children_num);
		pid_t child ;
		child = fork();
		if(child == -1){
			fprintf(stderr, "fork failed\n");
		}
		if(child == 0){
			
			key_t key = ftok("shmfile",65);
			int shmid = shmget(key,1024,0666|IPC_CREAT);
			children_num = (int*) shmat(shmid,(void*)0,0);
			signal(SIGINT, child_handler);		
			
			children_routine(arr);		
			send_result();	
			//print_result();
			kill(master,SIGUSR1);	
			*children_num -= 1;
			shmdt(children_num);
			exit(0);
		} else{
			waitpid(-1, NULL,WNOHANG);
		}
  
	} else {
		while(*children_num >= *PB){
		}
		parent_routine(arr);
	} 
}

void (*PARENT_ROUTINE)(int * _arr) = parent_routine ;

void start(){
	real_arr = (int*) malloc(sizeof(int)*(*SIZE));
	for(int i =0 ; i < *SIZE ; i++){
		real_arr[i] = i;	
	}
	prefix_arr = (int*) malloc(sizeof(int)*(*PS)) ;
	permutation_starter(real_arr, *PARENT_ROUTINE); 
	free(real_arr);
	free(prefix_arr);
}


int main(int argc, char** argv){

	if(argc < 3) {
		fprintf(stderr,"please input file path and the number of processes\n");
		return -1;
	}
	process_bound_at_a_time = atoi(argv[2]) ;
	if(*PB < 1 || *PB > 12){
		fprintf(stderr,"please input the number of processes  from 1 to 12\n");
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
	
	minpath = (int*) malloc(sizeof(int) * (*SIZE+1));

	fclose(fp);
	//until here, for receving input

	if(pipe(pipes) != 0){
		fprintf(stderr,"Pipe Create Error\n");
	}
	
	key_t key = ftok("shmfile",65);
	int shmid = shmget(key,1024,0666|IPC_CREAT);
	children_num = (int*) shmat(shmid,(void*)0,0);
	*children_num = 0;

	pid_t slave ;
	int exit_code;
	master = getpid();
	printf("%d\n", master);
	slave = fork();
	if(slave==0){
		start();
		exit(0);
	} else /*master*/{
		signal(SIGUSR1, sigusr_handler);
		signal(SIGINT,terminate_handler);
		wait(&exit_code);
			
		close(pipes[0]);	
	}
	wait(&exit_code);
	free(minpath);
	for(int i= 0 ; i< *SIZE ; i++){
		free(map[i]);
	}
	free(map);

	shmdt(children_num);
	shmctl(shmid,IPC_RMID,NULL);	

	return 0;
}
