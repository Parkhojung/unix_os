#include <pthread.h>
#include <stdio.h>

void *t_main(void *);

pthread_mutex_t sync_mutex;
pthread_cond_t sync_cond;

char 

typedef struct _Phil{
	struct _Phil * link;
} Phil;

void err(char * err_msg){
	printf("%s", err_msg);
	exit(1);
}

int main(int arg_cnt, char ** arg_arr){

	int num_of_phil = atoi(arg_arr[1]);
	if( 3 <= num_of_phil && num_of_phil <=10){
		printf("number of philoscophers : %d\n", num_of_phil);
	}else{
		err("num_of_phil is invalid");
	}
	
	int num_of_ms = atoi(arg_arr[2]);
	if( 10 <= num_of_ms && num_of_ms <=1000){
                printf("number of msec : %d\n", num_of_ms);
        }else{
                err("num_of_ms is invalid");
        }
	
	int num_of_cycle = atoi(arg_arr[3]);
	if( 1 <= num_of_phil && num_of_phil <=100){
                printf("number of cycle : %d\n", num_of_cycle);
        }else{
                err("num_of_cycle is invalid");
        }

	pthread_mutex_init(&sync_mutex, NULL);
	pthread_cond_init(&sync, NULL);
	
	for(int i = 0; i < 
		
	
	return 0;
}


