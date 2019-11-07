#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <stdlib.h>
#include <sys/timeb.h>

#define TRUE  1
#define FALSE 0
#define P_CNT 4 
#define LOG_MAX 100

pthread_mutex_t mymut;
pthread_cond_t mycond;
sem_t mysem;
int count;
int num_of_phil;
int num_of_ms;
int num_of_cycle;
int num_of_us;
enum {DO,END}state;

char log[LOG_MAX];
int log_order;

void err(char * err_msg){
	printf("%s", err_msg);
	exit(1);
}

void* t_function(void *data){
	pthread_t id;
	int tmp;
	char * pid = (char *)data;
	id = pthread_self();
	printf("Thread %lu Created. \n", id);
	
	while(1)
	{
		
		sem_wait(&mysem);	
		pthread_mutex_lock(&mymut);
//		pthread_cond_wait(&mycond, &mymut);

//		tmp = count;
//		tmp = tmp +1;
		usleep(num_of_us);
		log[log_order] = pid[7];
		log_order ++; 
//		count = tmp;
//		printf("[id:%s] : %d\n",pid, count);
		
//		pthread_cond_signal(&mycond);
		pthread_mutex_unlock(&mymut);
		sem_post(&mysem);
		
		
		usleep(num_of_us);
		
		//if(state == END)
		//	pthread_exit((void *)pid);
	
	}
}

int main(int arg_cnt, char ** arg_arr){

	pthread_t p_thread[P_CNT];
	char *p[] = {"thread_1", "thread_2","thread_3","thread_4","thread_5"};
	int thr_id;
	int status;
	int i = 0;
	
	num_of_phil = atoi(arg_arr[1]);
	if( (3 <= num_of_phil && num_of_phil <=10)== FALSE)
		err("num_of_phil is invalid");
	
	
	num_of_ms = atoi(arg_arr[2]);
	if( (10 <= num_of_ms && num_of_ms <=1000 ) == FALSE )
                err("num_of_ms is invalid");
        num_of_us = num_of_ms*1000;
	
	num_of_cycle = atoi(arg_arr[3]);
	if( (1 <= num_of_phil && num_of_phil <=100) ==FALSE)
                err("num_of_cycle is invalid");
        

	pthread_mutex_init(&mymut, NULL);
	pthread_cond_init(&mycond, NULL);
	

	if(sem_init(&mysem, 0, (int)P_CNT/2) == -1)
		err("sem_init");
	
	pthread_mutex_lock(&mymut);
	for( i = 0; i< P_CNT; i++)
	{
		thr_id= pthread_create(&p_thread[i],NULL, t_function, (void *)p[i]);
		if(thr_id<0)
		{
			perror("thread create error : ");
			exit(0);
		}	
	}
	

	pthread_mutex_unlock(&mymut);
	struct timeb start, end;
	ftime(&start);
//	pthread_cond_signal(&mycond);
	usleep(num_of_cycle*num_of_us);
	
	ftime(&end);
	int diff = (end.millitm-start.millitm)+(int)(1000.0*(end.time-start.time));
	state = END;
	for( i = 0 ; i<P_CNT; i++)
		pthread_cancel(p_thread[i]);
		
	printf("elapsed time : %d\n", diff);

	for(i = 0 ; i< LOG_MAX; i++){
		if(log[i] != '\0')
			printf("log[%d]: %c\n",i,log[i]);
	}

	return 0;
}


