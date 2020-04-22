#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

int ** map ;
int * prefix_arr ;
int count = 0;
int size = 0;
int prefix_size = 0;
int process_bound_at_a_time ;
const int * SIZE = &size;
const int * PS = &prefix_size;
const int * PB = &process_bound_at_a_time ;


void terminate_handler(int isg){
	printf("%d\n", *PS);
	exit(0);	
}

void swap(int * a, int * b){
	int temp = *a;
	*a = *b;
	*b= temp;
}

void parent_routine(int * arr){
	count ++; 
	printf("%d| (", count); 
	for(int i =0 ; i< *PS ; i++){
		printf("%d ", arr[i]);
	}
	printf("%d)\n", arr[0]);
	return;
}

void children_routine(){
	
}

void (*PR)(int * _arr) = parent_routine ;
void (*CR)(int * _arr) = children_routine;

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

void prefix_permutation(){
	prefix_arr = (int*) malloc(sizeof(int)*(*PS)) ;
	for(int i = 0 ; i < *PS ; i++)
		prefix_arr[i] = i;
	heap_permutation(prefix_arr, *PS, *PR); 	
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

	prefix_permutation();

	return 0;
}
