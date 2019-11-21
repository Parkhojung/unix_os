#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <stdlib.h>
#include <sys/timeb.h>

#define TRUE  1
#define FALSE 0
#define P_CNT 4 
#define LOG_TIMING_MAX 100
#define LOG_ORDER_MAX 5

pthread_mutex_t mymut;
pthread_cond_t mycond;
sem_t mysem;
int count;
int num_of_phil;
int num_of_ms, num_of_us;
int num_of_cycle;

char log[LOG_TIMING_MAX][LOG_ORDER_MAX];
int log_index;
int log_timing;

// error fuction
void err(char * err_msg){
	printf("%s", err_msg);
	exit(1);
}

// thread function
void* t_function(void *data){
	
	char * pid = (char *)data; // save parameter
	
	//print id
	pthread_t id; 
	id = pthread_self(); 
	printf("Thread %lu Created. \n", id);
	
	// loop eating and thinking
	while(1)
	{
		
		// get semaphore(two fork)	
		sem_wait(&mysem);	
		// eating
		usleep(num_of_us);
		
		// write log
		pthread_mutex_lock(&mymut);	
		log[log_timing][log_index] = pid[7];
		log_index++; 
		pthread_mutex_unlock(&mymut);
		
		// release semaphore(two fork)
		sem_post(&mysem);
		
		//thinking
		usleep(num_of_us);
		
	}
}

int main(int arg_cnt, char ** arg_arr){

	pthread_t p_thread[P_CNT];
	char *p[] = {"thread_1", "thread_2","thread_3","thread_4","thread_5"
	 	     ,"thread_6", "thread_7", "thread_8","thread_9", "thread_0"};
	int thr_id;
	int status;
	int i = 0;
	
	// number of philoscophers
	num_of_phil = atoi(arg_arr[1]);
	if( (3 <= num_of_phil && num_of_phil <=10)== FALSE)
		err("num_of_phil is invalid");
	
	// eating or thinking time
	num_of_ms = atoi(arg_arr[2]);
	if( (10 <= num_of_ms && num_of_ms <=1000 ) == FALSE )
                err("num_of_ms is invalid");
        num_of_us = num_of_ms*1000;
	
	// number of cycle 
	num_of_cycle = atoi(arg_arr[3]);
	if( (1 <= num_of_phil && num_of_phil <=100) ==FALSE)
                err("num_of_cycle is invalid");
        
	// initialize mutex variable
	if(pthread_mutex_init(&mymut, NULL) !=0){	
		perror("mutex initialize error: ");
		exit(1);
	}
	
	// initialize semaphore variable
	// value is (int)(# philosopher /2) for prevention
	if(sem_init(&mysem, 0, (int)num_of_phil/2) == -1){
		perror("semaphore initialize error:");
		exit(1);
	}
	
	// lock mutex until thread_create is completed
	pthread_mutex_lock(&mymut);
	
	// thread create
	for( i = 0; i< P_CNT; i++)
	{		
		thr_id= pthread_create(&p_thread[i],NULL, t_function, (void *)p[i]);
		
		// error check
		if(thr_id != 0)
		{
			perror("thread create error : ");
			exit(1);
		}	
	}
	// unlock mutex
	pthread_mutex_unlock(&mymut);
	
	// time calculate
	struct timeb start, end;
	ftime(&start);
	
	// sleep each cycle 
	for(log_timing= 0; log_timing < num_of_cycle; log_timing++){
		log_index =0;
		usleep(num_of_us);
	}
	

	ftime(&end);
	for( i = 0 ; i<P_CNT; i++)
		pthread_cancel(p_thread[i]);
	
	// elapsed time
	int elapsed_time = (end.millitm-start.millitm)+(int)(1000.0*(end.time-start.time));
	printf("elapsed time : %d\n", elapsed_time);

	// print log 
	for(i = 0 ; i< LOG_TIMING_MAX; i++){	
		for(int j = 0; j <LOG_ORDER_MAX; j++){
			if(log[i][j] != '\0')
				printf("log[%d][%d]: %c\n",i,j,log[i][j]);	
		}
	}
	
	return 0;
}


