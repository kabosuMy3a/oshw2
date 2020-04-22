#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

int m[12][12] ;

int path[12] ;
int used[12] ;
int length = 0 ;
int min = -1 ;
int minpath[12] ;
int count = 0;

void _travel(int idx) {
	int i ;

	if (idx == 12) {
		count ++;
		length += m[path[11]][path[0]] ;
		if (min == -1 || min > length ) {
			min = length ;
			for(i = 0 ; i< 12 ; i++){
				minpath[i]= path[i];
			}
		}
		printf("%d| %d (", count, length) ;
		for (i = 0 ; i < 12 ; i++) 
			printf("%d ", path[i]) ;
		printf("%d)\n", path[0]) ;	
		length -= m[path[11]][path[0]] ;
	}
	else {
		for (i = 0 ; i < 12 ; i++) {
			if (used[i] == 0) {
				path[idx] = i ;
				used[i] = 1 ;
				length += m[path[idx-1]][i] ;
				_travel(idx+1) ;
				length -= m[path[idx-1]][i] ;
				used[i] = 0 ;
			}
		}
	}
}

void handler(int isg){
	printf("\nDUDUDUNGA!\n %d (", min) ;
	for (int i = 0 ; i < 12 ; i++) 
		printf("%d ", minpath[i]) ;
	printf("%d) - %d checked\n", minpath[0],count) ;
	exit(0);
	
}

void travel(int start) {
	path[0] = start ;
	used[start] = 1 ;
	_travel(1) ;
	used[start] = 0 ;
}

int main() {
	int i, j, t ;

	FILE * fp = fopen("gr17.tsp", "r") ;

	for (i = 0 ; i < 12 ; i++) {
		for (j = 0 ; j < 12 ; j++) {
			fscanf(fp, "%d", &t) ;
			m[i][j] = t ;
		}
	}
	fclose(fp) ;
	signal(SIGINT, handler);

	for (i = 0  ; i < 12 ; i++) 
		travel(i) ;
	printf("\nDUDUDUNGA!\n %d (", min) ;
	for (int i = 0 ; i < 12 ; i++) 
		printf("%d ", minpath[i]) ;
	printf("%d) - %d checked\n", minpath[0],count) ;

}
