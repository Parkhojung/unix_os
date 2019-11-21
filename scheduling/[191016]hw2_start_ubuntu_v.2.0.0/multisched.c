//
//  main.c
//  [191016]hw2_start_xcode_v.1.0.0
//
//  Created by PARKHOJUNG on 2019/10/17.
//  Copyright © 2019 PARKHOJUNG. All rights reserved.
//

/*
 * OS Assignment #2
 */

/* header file part */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>


/* #define part */
#define MSG(x...) fprintf (stderr, x)

#define PROCESS_MAX_NUMBER (26 * 10)
#define ID_MAX  2

#define ARRIVE_TIME_MIN 0
#define ARRIVE_TIME_MAX 30

#define SERVICE_TIME_MIN 1
#define SERVICE_TIME_MAX 30

#define PRIORITY_MIN 1
#define PRIORITY_MAX 10

#define SLOT_MAX ARRIVE_TIME_MAX + (SERVICE_TIME_MAX * PROCESS_MAX_NUMBER)

#define TRUE 1
#define FALSE 0

/* type declaration part */
typedef struct _Process Process;
struct _Process
{
//    int    idx;
//    int    queue_idx;
    
    /* input data */
    char   id[ID_MAX + 1];
    int    arrive_time; // arrive time
    char   process_type; // type
    int    service_time; // assign work time
    int    priority;
    
    /* using data while simulate */
    int    work_remain_time;   // remain work time 
    int    work_end_time; // complete time
    
    /* output data */
    char   work_chart[SLOT_MAX];
    int    ternaround_time;
    int    wait_time;
    
    Process* next;
};

typedef struct _Process_queue Process_queue;
struct _Process_queue{
    Process * head;
    Process * tail;
    Process * cur;
};



/* global variable */
Process process_array[PROCESS_MAX_NUMBER]; // all process is saved here
Process_queue high_process_queue;   // H type queue
Process_queue mid_process_queue;    // M type queue
Process_queue low_process_queue;    // L type queue
static int num_of_all_process;
static int last_assigned_process_order; // order number assigned last time( best recently)




/* prototype part */
static void assign_process_to_multi_level_queue(int);
static int add_multilevl_process_queue(char process_type, Process * add_process);
static void prn_multilevel_queue(void);
static void prn_process_array(void);
static void prn_result(int );



/* function definition part with compound statement */

/* print split line */
static void prn_split_line(){
    MSG("\n-----------------------------------\n");
}

/* print split line with string */
static void prn_split_str(char * s){
    MSG("\n-----------%s--------------\n", s);
}

/* print all queue*/
static void prn_multilevel_queue(){
    Process * pp;
    
    Process_queue pl[3];
    pl[0] = high_process_queue;
    pl[1] = mid_process_queue;
    pl[2] = low_process_queue;

    for(int i = 0 ; i < 3; i++){
        
        if(i ==0){
            MSG("H queue ) ");
        }else if(i ==1){
            MSG("M queue ) ");
        }else if(i ==2){
            MSG("L queue ) ");
        }
//
//        pp = pl[i].head->next;
        pp = pl[i].head;
        while(1){
            
            pp = pp->next;
            if(pp == pl[i].tail)
                break;
            else
                MSG("[%s]",pp->id);
            
            if(pp->next != pl[i].tail)
                MSG("-> ");
            
        }
        MSG("\n");
    }
}

/* */
static void prn_process_array(void){
    
    Process* pp;
    
    prn_split_str("[PROCESS ARRAY]");
    for(int i = 0; i < num_of_all_process; i++){
        pp = &process_array[i];
        MSG("%s %c %d %d %d\n",pp->id, pp->process_type, pp->arrive_time, pp->service_time, pp->priority);
    }
    
}

/* strip function */
static char *strstrip (char *str)
{
    char  *start;
    size_t len;
    
    len = strlen (str);
    
    while (len--)
    {
        if (!isspace (str[len]))
            break;
        str[len] = '\0';
    }
    
    for (start = str; *start && isspace (*start); start++)
        ;
    memmove (str, start, strlen (start) + 1);
    
    return str;
}

/* check id format */
static int check_valid_id (const char *str)
{
    size_t len;
    
    len = strlen (str);
    if (len != ID_MAX)
        return -1;
    
    if(!isupper(str[0]))
        return -1;
    
    if(!isdigit(str[1]))
        return -1;
//
//    for (i = 0; i < len; i++)
//        if (!(isupper (str[i]) || isdigit (str[i])))
//            return -1;
//
    return 1;
}

/* check duplicated id */
static Process *lookup_process (const char *id)
{
    int i;
    
    for (i = 0; i < num_of_all_process; i++){
        if (!strcmp (id, process_array[i].id))
            return &process_array[i];
    }
    
    return NULL;
}

/* append process in process_array */
static void add_process (Process *process)
{
    process_array[num_of_all_process] = *process;
//    process_array[num_of_all_process].idx = num_of_all_process;
    num_of_all_process++;
}

/* read data file */
static int read_config (const char *filename)
{
    
    prn_split_str("[READ CONFIG]");
    FILE *fp;
    char  line[256];
    int   line_nr;
    
    fp = fopen (filename, "r");
    if (!fp)
        return -1;
    
    num_of_all_process = 0;
    
    line_nr = 0;
    while (fgets (line, sizeof (line), fp)){
        
        Process process;
        char  *p;
        char  *s;
        size_t len;
        
        line_nr++;
        memset (&process, 0x00, sizeof (process));
        
        /* preprocessing */
        len = strlen (line);
        if (line[len - 1] == '\n')
            line[len - 1] = '\0';
        if (0)
            MSG ("config[%3d] %s\n", line_nr, line);
        strstrip (line);
        
        
        /* comment or empty line */
        if (line[0] == '#' || line[0] == '\0')
            continue;
        
        
        /* id */
        s = line;
        p = strchr (s, ' ');
        if (!p)
            goto invalid_line;
        *p = '\0';
        strstrip (s);
        
        // format check
     
        
        if (check_valid_id (s) == -1 ){
            MSG ("invalid process id '%s' in line %d, ignored\n", s, line_nr);
            continue;
        }
        
        // duplication check
        if (lookup_process (s) != NULL){
            MSG ("duplicate process id '%s' in line %d, ignored\n", s, line_nr);
            continue;
        }
        strcpy (process.id, s);
        
        
        /* process type */
        s = p + 1;
        p = strchr(s, ' ');
        if(!p)
            goto invalid_line;
        *p ='\0';
        strstrip(s);
        // length check
        if(strlen(s)>1 ){
            MSG ("invalid process type '%s' in line %d, ignored\n", s, line_nr);
            continue;
        }
        if(!(s[0] == 'H' || s[0] == 'M' || s[0] == 'L')){
            MSG ("invalid process type '%s' in line %d, ignored\n", s, line_nr);
            continue;
        }
        process.process_type = s[0];
        
        
        
        /* arrive time */
        s = p + 1;
        p = strchr (s, ' ');
        if (!p)
            goto invalid_line;
        *p = '\0';
        strstrip (s);
        
        process.arrive_time = (int)strtol (s, NULL, 10);
        
        // time check
        if (process.arrive_time < ARRIVE_TIME_MIN ||  process.arrive_time > ARRIVE_TIME_MAX
            || (num_of_all_process > 0 && process_array[num_of_all_process - 1].arrive_time > process.arrive_time)
            )
        {
            
            MSG ("invalid arrive-time '%s' in line %d, ignored\n", s, line_nr);
            continue;
        }
        
        /* service time */
        s = p + 1;
        p = strchr (s, ' ');
        if (!p)
            goto invalid_line;
        *p = '\0';
        strstrip (s);
        process.service_time = (int)strtol (s, NULL, 10);
        //time check
        if (process.service_time < SERVICE_TIME_MIN
            ||  process.service_time > SERVICE_TIME_MAX)
        {
            MSG ("invalid service-time '%s' in line %d, ignored\n", s, line_nr);
            continue;
        }
        // remain time set
        process.work_remain_time = process.service_time;
        
        
        /* priority */
        s = p + 1;
        strstrip (s);
        process.priority = (int)strtol (s, NULL, 10);
        if (process.priority < PRIORITY_MIN
            || process.priority > PRIORITY_MAX)
        {
            MSG ("invalid priority '%s' in line %d, ignored\n", s, line_nr);
            continue;
        }
        
        add_process (&process);
        continue;
        
        /* invalid label */
        invalid_line:
            MSG ("invalid format in line %d, ignored\n", line_nr);
    }
    
    fclose (fp);
    
    prn_process_array();
    prn_split_line();
    
    return 1;
}



/* process in all_process_list is assigned to multilevel queue by process type  */
static void assign_process_to_multi_level_queue(int cpu_time){
    
    int i;
    for(i = last_assigned_process_order; i < num_of_all_process; i++){
        
        if(process_array[i].arrive_time <= cpu_time){
            prn_split_str("ASSIGN TO QUEUE");
            MSG("cpu time : %d\n", cpu_time);
            MSG("[%s]'s arrive_time : %d\n", process_array[i].id, process_array[i].arrive_time);
            add_multilevl_process_queue(process_array[i].process_type, &process_array[i]);
            prn_multilevel_queue();
            prn_split_line();
        }
        else{
            break;
        }
    }
    
    last_assigned_process_order = i;
    
}

/* check level queue */
static int isEmptyQueue(Process_queue *pq){
    if(pq->head->next == pq->tail)
        return TRUE;
    else
        return FALSE;
}

/* add process in multi level queue by process type */
static int add_multilevl_process_queue(char process_type, Process * add_process){

    Process * cur_process;
    Process * prev_process;
    Process_queue* pq_p;
    
    /* append in high_process_queue */
    if(process_type == 'H'){
        
        pq_p = &high_process_queue;
        
        /* empty queue case */
        if(pq_p->head->next == pq_p->tail){
            
            pq_p->head->next = add_process;
            add_process->next = pq_p->tail;
            
        }/* non empty queue case */
        else{
            cur_process = pq_p->head->next;
            prev_process = pq_p->head;
            
            while(cur_process != pq_p->tail){
                
                /* criteria : priority */
                if(cur_process->priority <= add_process->priority ){

                    prev_process = cur_process;
                    cur_process = cur_process->next;
                }
                else{
                    prev_process->next = add_process;
                    add_process->next = cur_process;
                    
                    break;
                }
            }
            
            // when cur_process ends to tail
            if(cur_process == pq_p->tail){
                prev_process->next = add_process;
                add_process->next = pq_p->tail;
            }
            
        }
        
    } /* Mid process */
    else if(process_type == 'M'){
        pq_p = &mid_process_queue;
        
        /* empty queue case */
        if(pq_p->head->next == pq_p->tail){
            
            pq_p->head->next = add_process;
            add_process->next = pq_p->tail;
            
        }/* non empty queue case */
        else{
            cur_process = pq_p->head->next;
            prev_process = pq_p->head;
            
            while(cur_process != pq_p->tail){
                
                /* criteria : service time */
                if(cur_process->service_time <= add_process->service_time ){
                    
                    prev_process = cur_process;
                    cur_process = cur_process->next;
                }else{
                    prev_process->next = add_process;
                    add_process->next = cur_process;
                    
                    break;
                }
            }
            
            // when cur_process ends to tail
            if(cur_process == pq_p->tail){
                prev_process->next = add_process;
                add_process->next = pq_p->tail;
            }
            
        }
    }
    else if(process_type == 'L'){
        
        pq_p = &low_process_queue;
        
        /* empty queue case */
        if(pq_p->head->next == pq_p->tail){
            
            pq_p->head->next = add_process;
            add_process->next = pq_p->tail;
            
        }/* non empty queue case */
        else{
            cur_process = pq_p->head->next;
            prev_process = pq_p->head;
            
            while(cur_process != pq_p->tail){
                
                if(cur_process->arrive_time <= add_process->arrive_time ){
                    
                    prev_process = cur_process;
                    cur_process = cur_process->next;
                }else{
                    prev_process->next = add_process;
                    add_process->next = cur_process;
                    
                    break;
                }
            }
            
            // when cur_process ends to tail
            if(cur_process == pq_p->tail){
                prev_process->next = add_process;
                add_process->next = pq_p->tail;
            }
        }
    }

    return 1;
}


/* simulate by cpu time , criteria : 1msec */
static int simulate (){
    
//    int last_complete_time;
    int remain_process_num = num_of_all_process;
    Process * work_process;
    Process_queue* work_queue = NULL;
    
    int cpu_time = 0 ;

    while ( remain_process_num > 0 && cpu_time <= SLOT_MAX){
        
        
        //
        assign_process_to_multi_level_queue(cpu_time);
         
         // 60% part - 0~5
        if(cpu_time % 10 <= 5){
             
             /* high process exist */
            if(isEmptyQueue(&high_process_queue) != TRUE){
                 
                work_process = high_process_queue.head->next;
                work_queue = &high_process_queue;
                
            }/* high process does not exist && mid process exist */
            else if(isEmptyQueue(&mid_process_queue) != TRUE){
                 
                work_process = mid_process_queue.head->next;
                work_queue = &mid_process_queue;
             
            }/* high process does not exist && mid process does not exist && low process exist */
            else if(isEmptyQueue(&low_process_queue) != TRUE){
                 
                work_process = low_process_queue.head->next;
                work_queue = &low_process_queue;
             
            } /* all queue is empty*/
            else{
                work_process = NULL;
            }
        }
         // 40 % part - 6~9
        else{
            if(isEmptyQueue(&mid_process_queue) != TRUE){
                 
                work_process = mid_process_queue.head->next;
                work_queue = &mid_process_queue;
                 
            }
            else if(isEmptyQueue(&high_process_queue) != TRUE){
                
                work_process = high_process_queue.head->next;
                work_queue = &high_process_queue;
             
            }
            else if(isEmptyQueue(&low_process_queue) != TRUE){
            
                work_process = low_process_queue.head->next;
                work_queue = &low_process_queue;
            
            }
            else{
                work_process = NULL;
            }
         }
    
        /* empty queue */
        if(work_process == NULL)
            ;
        else{
         
            work_process->work_remain_time -= 1;
            work_process->work_chart[cpu_time] = '*';
             
            if(work_process->work_remain_time == 0 ){
                work_process->work_end_time = cpu_time;
                work_queue->head->next = work_queue->head->next->next;
                remain_process_num -=1;
                 
            }
        }
        cpu_time += 1;
     }
    
    
    return cpu_time;
}

/* multilevel queue intialize */
static int inititalize_multilevel_queue(){
    
    /* high_process_queue initialize */
    high_process_queue.head = (Process*)malloc(sizeof(Process));
    high_process_queue.tail = (Process*)malloc(sizeof(Process));
    if(high_process_queue.head == NULL || high_process_queue.tail == NULL)
        return -1;
    
    high_process_queue.head->next = high_process_queue.tail;
    high_process_queue.tail->next = NULL;

    /* mid_process_queue initialize */
    mid_process_queue.head = (Process*)malloc(sizeof(Process));
    mid_process_queue.tail = (Process*)malloc(sizeof(Process));
    if(mid_process_queue.head == NULL || mid_process_queue.tail == NULL)
        return -1;
    
    mid_process_queue.head->next = mid_process_queue.tail;
    mid_process_queue.tail->next = NULL;
    
    
    /* low_process_queue initialize */
    low_process_queue.head = (Process*)malloc(sizeof(Process));
    low_process_queue.tail = (Process*)malloc(sizeof(Process));
    if(low_process_queue.head == NULL || low_process_queue.tail == NULL)
        return -1;
    
    low_process_queue.head->next = low_process_queue.tail;
    low_process_queue.tail->next = NULL;
    
    return 1;
}


/* main function */
int main (int argc, char **argv)
{
    
    
    
    /* argument check */
    if (argc <= 1)
    {
        MSG ("input file must specified\n");
        return -1;
    }
    
    /* multi level queue initialize */
    if(inititalize_multilevel_queue() == -1){
        MSG("multi level queue initialize failed \n");
        return -1;
    }
    
    /* correct config check */
    if ( read_config (argv[1]) == -1)
    {
        MSG ("failed to load input file '%s'\n", argv[1]);
        return -1;
    }
    
    
    
    int cpu_time;
    
    /* simulate */
    if( (cpu_time = simulate ()) == -1){
        MSG("simulate failed\n");
        return -1;
    }
    
    prn_result(cpu_time);
    MSG("file name : %s", argv[1]);
    
    return 0;
}

static void prn_result(int cpu_time){
    
    int last_complete_time = cpu_time;
    
    /* calculate times and print cpu slot */
    int sum_turnaround_time = 0; //
    int sum_waiting_time = 0;
    Process *pp;
    
    prn_process_array();
    MSG("\n");
    
    prn_split_str("[RESULT]");
    /* print gantt chart */
    for(int i = 0 ; i < num_of_all_process; i++){
        
        pp = &process_array[i];
        
        /* many process test time for simulation
         int weight = 1;
         
         if(pp->process_type == 'H')
         weight = 5;
         else if( pp->process_type =='M')
         weight = 3;
         else{
         weight = 1;
         }
         
         
         pp->ternaround_time = ((pp->work_end_time+1) - pp->arrive_time) *weight ;
         pp->wait_time = (((pp->work_end_time+1) - pp->arrive_time) - pp->service_time) *weight;
         
         
         
         ------------------------------------------------------*/
        
        
        /* sumation of turnaround time and wait time */
        pp->ternaround_time = ((pp->work_end_time+1) - pp->arrive_time) ;
        pp->wait_time = (((pp->work_end_time+1) - pp->arrive_time) - pp->service_time) ;
        
        sum_turnaround_time += pp->ternaround_time;
        sum_waiting_time += pp->wait_time;

        MSG("%s ", pp->id);
        
        for(int j = 0 ; j <= last_complete_time; j++){
            if(pp->work_chart[j] == '*'){
                MSG("*");
            }else{
                MSG(" ");
            }
            
        }
        MSG("\n");
    }
    
    //    MSG("\n");
    //    /* print ternaround time, wait time */
    //    for(int i = 0; i< num_of_all_process; i++){
    //        MSG("%s ternaround: %d, wait: %d\n",process_array[i].id, process_array[i].ternaround_time, process_array[i].wait_time);
    //    }
    
    MSG("\n");
    MSG ("CPU TIME: %d\n", cpu_time);
    MSG ("AVERAGE TURNAROUND TIME: %.2lf\n", ((double)sum_turnaround_time)/ num_of_all_process);
    MSG ("AVERAGE WAITING TIME: %.2lf\n", ((double)sum_waiting_time)/num_of_all_process);
    
}
