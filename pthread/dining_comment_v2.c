#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <stdlib.h>
#include <sys/timeb.h>
#include <time.h>

#define TRUE  1
#define FALSE 0
#define P_CNT 4 
#define LOG_TIMING_MAX 100
#define LOG_ORDER_MAX 5
#define PHIL_NUM_MAX 10
//variable
pthread_mutex_t write_mut;
pthread_mutex_t pickup_mut;
pthread_mutex_t fork_mut_arr[PHIL_NUM_MAX];
sem_t mysem;
//sem_t mysem[PHIL_NUM_MAX];
int count;
int num_of_phil, num_of_fork_set;
int num_of_ms, num_of_us;
int num_of_cycle;
// log variable
char log_arr[LOG_TIMING_MAX][LOG_ORDER_MAX];
int log_index;
int log_timing;

// error fuction
void err(char * err_msg){
	printf("%s", err_msg);
	exit(1);
}

// check function
void check(){
	int diff;
	for(int i = 0; i <num_of_cycle; i++){
		for(int j = 0; j< num_of_fork_set-1; j++){
			if(log_arr[i][j] == '\0')
				continue;
			for(int k =j+1; k< num_of_fork_set; k++){
				if(log_arr[i][k] == '\0')
					continue;
				diff = log_arr[i][j] - log_arr[i][k];
				if(diff == 1 || diff ==-1){
					printf("error in [%d][%d]:%c, [%d][%d]:%c, diff: %d \n"
						,i,j,log_arr[i][j],i,k,log_arr[i][k], diff );
					exit(1);
				}
			}
		}
	}
	printf("error doesn't occur\n");
}

// thread function
void* t_function(void *data){
	
	char * pid = (char *)data; // save parameter
	int int_pid = atoi(pid);
	int lock_res;

	//print id
	pthread_t id; 
	id = pthread_self(); 
	printf("[%d, id:%lu] Created.\n", int_pid,id);
	
	int next_int_pid = (int_pid+1)%num_of_phil;
	// loop eating and thinking
	while(1)
	{
		
		// check using forkset
		sem_wait(&mysem);
		pthread_mutex_lock(&pickup_mut);
	//	printf("[%d, t:%d] pick up try\n",int_pid,log_timing);
		pthread_mutex_lock(&fork_mut_arr[int_pid]);
		pthread_mutex_lock(&fork_mut_arr[next_int_pid]);
	//	printf("[%d, t:%d] pick up sucess\n",int_pid,log_timing);	
		pthread_mutex_unlock(&pickup_mut);		
		
		// write log
		pthread_mutex_lock(&write_mut);	
		log_arr[log_timing][log_index] = pid[0];
		log_index++; 
		pthread_mutex_unlock(&write_mut);
		
		// eating
	//	printf("[%d, t:%d] eat\n",int_pid,log_timing);
		usleep(num_of_us);
		
		// release semaphore(two fork)
	//	printf("[%d, t:%d] release\n",int_pid, log_timing);
		pthread_mutex_unlock(&fork_mut_arr[int_pid]);
		pthread_mutex_unlock(&fork_mut_arr[(int_pid+1)%num_of_phil]);
		sem_post(&mysem);		
		
		//thinking
		usleep(num_of_us);

	}
}

int main(int arg_cnt, char ** arg_arr){

	pthread_t p_thread[P_CNT];
	char *pid_list[] = {"0","1", "2","3","4","5","6", "7", "8","9"};
	
	int thr_id;
	int status;
	int i = 0;
	
	// number of philoscophers
	num_of_phil = atoi(arg_arr[1]);
	num_of_fork_set =(int)num_of_phil/2;
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
	if(pthread_mutex_init(&write_mut, NULL) !=0){	
		perror("mutex initialize error: ");
		exit(1);
	}
	
	if(pthread_mutex_init(&pickup_mut,NULL)!=0){
		perror("mutex initialize error: ");
		exit(1);
	}
	
	
	sem_init(&mysem,0, num_of_fork_set);
	
	for(i = 0 ; i< num_of_phil; i++){
		//if(sem_init(&mysem[i], 0, 1) == -1){
		if(pthread_mutex_init(&fork_mut_arr[i],NULL)!=0){
			perror("semaphore initialize error:");
			exit(1);
		}	
	}
	
	// lock mutex until thread_create is completed
	pthread_mutex_lock(&pickup_mut);
	
	// thread create 0,2,4,... 1,3,5,...
	
	for (int start_num= 0; start_num <= 1; start_num++){	
		for( i = start_num; i< num_of_phil; i=i+2)
		{		
			thr_id= pthread_create(&p_thread[i],NULL, t_function, (void *)pid_list[i]);
			// error check
			if(thr_id != 0)
			{
				perror("thread create error : ");
				exit(1);
			}	
		}
	}

	// unlock mutex
	pthread_mutex_unlock(&pickup_mut);
	
	// time calculate
	struct timeb start, end;
	ftime(&start);
	
	// sleep each cycle 
	for(log_timing= 0; log_timing < num_of_cycle; log_timing++){
		pthread_mutex_lock(&write_mut);
		log_index =0;
		pthread_mutex_unlock(&write_mut);
		usleep(num_of_us);
	}
	

	ftime(&end);
	for( i = 0 ; i<P_CNT; i++)
		pthread_cancel(p_thread[i]);
	
	// elapsed time
	int elapsed_time = (end.millitm-start.millitm)+(int)(1000.0*(end.time-start.time));
	printf("elapsed time : %d\n", elapsed_time);

	// print log [7, t:1] eat
	for(i = 0 ; i< num_of_cycle; i++){
		printf("log[%d]: ",i);	
		for(int j = 0; j <num_of_fork_set; j++){
			if(log_arr[i][j] != '\0')
				printf("%c/ ",log_arr[i][j]);	
			else 
				printf("x/ ");
		}
		printf("\n");
	}
	
	// check function	
	check();	

	return 0;
}




