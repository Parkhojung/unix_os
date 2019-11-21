/*
 * OS Assignment #2
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <sys/wait.h>

#define MSG(x...) fprintf (stderr, x)
#define STRERROR  strerror (errno)

#define PROCESS_MAX (52 * 10)
#define ID_MAX      2

#define ARRIVE_TIME_MIN 0
#define ARRIVE_TIME_MAX 30

#define SERVICE_TIME_MIN 1
#define SERVICE_TIME_MAX 30

#define PRIORITY_MIN 1
#define PRIORITY_MAX 10

#define SLOT_MAX (ARRIVE_TIME_MAX + SERVICE_TIME_MAX) * PROCESS_MAX



typedef struct _Process Process;
struct _Process
{
  int    idx;
  int    queue_idx;

  char   id[ID_MAX + 1];
  int    arrive_time;
  char   process_type;
  int    service_time;
  int    priority;

  int    remain_time;
  int    complete_time;
  int    wait_time;

  Process* next;
};


typedef struct _Process_list Process_list;
struct _Process_list{
  Process * head;
  Process * tail;
  Process * cur;
};
 
Process all_process_queue[PROCESS_MAX];

Process_list high_process_queue;
Process_list mid_process_queue;
Process_list low_process_queue;

static int      num_of_all_task;
static int      queue_len;
static char     schedule[PROCESS_MAX][SLOT_MAX];

static void assign_process_to_multi_level_queue(int);
static int add_multilevl_process_queue(char process_type, Process * add_process);
static void prn_multilevel_queue();

static void prn_split_line(){
  MSG("\n---------------------------------------\n");
}

static void prn_split_str(char * s){
  MSG("\n---------------%s--------------------\n", s);
}

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

static int check_valid_id (const char *str)
{
  size_t len;
  int    i;

  len = strlen (str);
  if (len != ID_MAX)
    return -1;

  for (i = 0; i < len; i++)
    if (!(isupper (str[i]) || isdigit (str[i])))
      return -1;

  return 0;
}

static Process *lookup_process (const char *id)
{
  int i;

  for (i = 0; i < num_of_all_task; i++){
      if (!strcmp (id, all_process_queue[i].id))
          return &all_process_queue[i];
  }

  return NULL;
}

static void append_process (Process *process)
{
  all_process_queue[num_of_all_task] = *process;
  all_process_queue[num_of_all_task].idx = num_of_all_task;
  num_of_all_task++;
}

static int read_config (const char *filename)
{

  FILE *fp;
  char  line[256];
  int   line_nr;

  fp = fopen (filename, "r");
  if (!fp)
    return -1;

  num_of_all_task = 0;

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
      if (check_valid_id (s)){
	        MSG ("invalid process id '%s' in line %d, ignored\n", s, line_nr);
	        continue;
	    }

      if (lookup_process (s)){
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
      if(strlen(s)>1)
          MSG ("invalid process type '%s' in line %d, ignored\n", s, line_nr);
      process.process_type = s[0];
      


      /* arrive time */
      s = p + 1;
      p = strchr (s, ' ');
      if (!p)
	        goto invalid_line;
      *p = '\0';
      strstrip (s);

      process.arrive_time = strtol (s, NULL, 10);
        
      if (process.arrive_time < ARRIVE_TIME_MIN 
            ||  process.arrive_time > ARRIVE_TIME_MAX
	          || (num_of_all_task > 0 
            && all_process_queue[num_of_all_task - 1].arrive_time > process.arrive_time))
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
      process.service_time = strtol (s, NULL, 10);
      if (process.service_time < SERVICE_TIME_MIN
	        ||  process.service_time > SERVICE_TIME_MAX)
	    {
	        MSG ("invalid service-time '%s' in line %d, ignored\n", s, line_nr);
	        continue;
	    }

      /* priority */
      s = p + 1;
      strstrip (s);
      process.priority = strtol (s, NULL, 10);
      if (process.priority < PRIORITY_MIN
	        || process.priority > PRIORITY_MAX)
	    {
	        MSG ("invalid priority '%s' in line %d, ignored\n", s, line_nr);
	        continue;
	    }

      append_process (&process);
      continue;

      invalid_line:
          MSG ("invalid format in line %d, ignored\n", line_nr);
    }
  
  fclose (fp);

  prn_split_line();
  for(int i = 0 ; i < num_of_all_task; i++ ){
    printf("[%s] -> ", all_process_queue[i].id);
  }
  
  prn_split_line();

  return 0;
}



static int last_assigned_process_order; // order number assigned last time( best recently)

/* process in all_process_list is assigned to multilevel queue by process type  */
static void assign_process_to_multi_level_queue(int cpu_time){

    prn_split_str("assign_process_to_multi_level_queue start");

    // add_multilevl_process_queue('H', &all_process_queue[0]);
    // add_multilevl_process_queue('M', &all_process_queue[1]);
    // add_multilevl_process_queue('L', &all_process_queue[2]);
    // add_multilevl_process_queue('H', &all_process_queue[3]);
    int i = 0;
    for(i = last_assigned_process_order; i < num_of_all_task; i++){
      if(all_process_queue[i].arrive_time < 100)
          add_multilevl_process_queue(all_process_queue[i].process_type, &all_process_queue[i]);
    }

    last_assigned_process_order = i;

    prn_split_str("assign_process_to_multi_level_queue end");
    prn_multilevel_queue();
}


/* add process in multi level queue by process type */
static int add_multilevl_process_queue(char process_type, Process * add_process){
    
    prn_split_str("add multi level queue fun start ");
    
    MSG("add_process->id: %s\n",add_process->id);
    MSG("add_process: %u\n", add_process);
    Process * cur_process;
    Process * prev_process;

    /* append in high_process_queue */
    if(process_type == 'H'){
        MSG("high_process_queue.head->next : %u\n", high_process_queue.head->next);
        MSG("high_process_queue.tail : %u\n", high_process_queue.tail);
        /* empty queue case */
        if(high_process_queue.head->next == high_process_queue.tail){
            // MSG("1.high_process_queue.head->next : %u\n", high_process_queue.head->next);

            high_process_queue.head->next = add_process;
            add_process->next = high_process_queue.tail;

            // MSG("2. high_process_queue.head->next: %u\n",high_process_queue.head->next);    
            // MSG("2. add_process->next : %u\n", add_process->next);
            // MSG("2.add_process->id : %s\n", add_process->id);
            // MSG("2.high_process_queue.head->next->id : %s\n",high_process_queue.head->next->id);

        }else{
          cur_process = high_process_queue.head->next;
          prev_process = high_process_queue.head;
          
          MSG("add_process addr: %u , id: %s, priority: %d\n", &add_process, add_process->id, add_process->priority );
          while(cur_process != high_process_queue.tail){

              MSG("cur_process addr: %u , id: %s, priority: %d\n", &cur_process, cur_process->id, cur_process->priority );
              

              if(cur_process->priority <= add_process->priority ){
                  MSG("cur p: %d\n" ,cur_process->priority);
                  MSG("add p: %d\n",add_process->priority);
                  prev_process = cur_process;
                  cur_process = cur_process->next;
              }else{
                  prev_process->next = add_process;
                  add_process->next = cur_process;

                  MSG("prev_process->next->id :%s", prev_process->next->id);
                  MSG("add_process->next->id: %s", add_process->next->id);
                  break;
              } 


          }
          if(cur_process == high_process_queue.tail){
            prev_process->next = add_process;
            add_process->next = high_process_queue.tail;
          }

        }

    }
    MSG("FINISH****************************");
    prn_multilevel_queue();
    prn_split_str("add multi level queue fun end");
}

/* print all queue*/
static void prn_multilevel_queue(){
  Process * pp;

  Process_list pl[3];
  pl[0] = high_process_queue;
  pl[1] = mid_process_queue;
  pl[2] = low_process_queue;
  
  prn_split_str("prn_multilevel_queue start");
  MSG("%s\n",high_process_queue.head->next->id);

  for(int i = 0 ; i < 3; i++){

      pp = pl[i].head->next;
      while(1){
        
        if(pp != pl[i].tail){
            MSG("[id: %s]",pp->id);
            pp = pp->next;
        }
        else{
            MSG("\n");
            break;

        }
        MSG("->");
      }
 
  }

  prn_split_str("prn_multilevel_queue end");

}

/* simulate */
static void simulate (){

  prn_split_str("simulate start");
  int cpu_time = 0 ;
  int remain_process_num = num_of_all_task;

  assign_process_to_multi_level_queue(100);

  // while ( remain_process_num > 0){
  //     cpu_time += 1;

  //     assign_process_to_multi_level_queue(cpu_time);

  //     // 60% part
  //     if(cpu_time %10 + 1 <= 6){
  //         ;
  //     }
  //     // 40 % part
  //     else{
  //         ;
  //     }

  //     if(cpu_time == 30)
  //         return;
  // }
    
  

  /* calculate times and print cpu slot */
  int sum_turnaround_time = 0;
  int sum_waiting_time = 0;


  // for (int i = 0; i < num_of_all_task; i++)
  //   {
  //     int slot;
  //                                               //간트 챠트 형식의 출력
  //     printf ("%s ", all_process_queue[i].id);
  //     for (slot = 0; slot <= cpu_time; slot++)
	//         putchar (schedule[i][slot] ? '*' : ' ');
      
  //     printf ("\n");

  //     sum_waiting_time += all_process_queue[i].wait_time;
  //   }

  int avg_turnaround_time = (float) sum_turnaround_time / (float) num_of_all_task;
  int avg_waiting_time = (float) sum_waiting_time / (float) num_of_all_task;

  printf ("CPU TIME: %d\n", cpu_time);
  printf ("AVERAGE TURNAROUND TIME: %.2f\n", avg_turnaround_time);
  printf ("AVERAGE WAITING TIME: %.2f\n", avg_waiting_time);

  prn_split_str("simulate end");
}

static int inititalize_multilevel_queue(){
  
  prn_split_str("multilevel queue init start");
  /* high_process_queue initialize */
  high_process_queue.head = (Process*)malloc(sizeof(Process));
  high_process_queue.tail = (Process*)malloc(sizeof(Process));
  if(high_process_queue.head == NULL || high_process_queue.tail == NULL)
    return -1;

  high_process_queue.head->next = high_process_queue.tail;
  high_process_queue.tail->next = NULL;

  // MSG("high_process_queue address : %d\n", &high_process_queue);
  // MSG("high_process_queue.head address : %d\n", high_process_queue.head);
  // MSG("high_process_queue.head->next address : %d\n", high_process_queue.head->next);
  // MSG("high_process_queue.tail address : %d\n", high_process_queue.tail);

  /* mid_process_queue initialize */
  mid_process_queue.head = (Process*)malloc(sizeof(Process));
  mid_process_queue.tail = (Process*)malloc(sizeof(Process));
  if(mid_process_queue.head == NULL || mid_process_queue.tail == NULL)
    return -1;
  
  mid_process_queue.head->next = mid_process_queue.tail;
  mid_process_queue.tail = NULL;


  /* low_process_queue initialize */
  low_process_queue.head = (Process*)malloc(sizeof(Process));
  low_process_queue.tail = (Process*)malloc(sizeof(Process));
  if(low_process_queue.head == NULL || low_process_queue.tail == NULL)
    return -1;

  low_process_queue.head->next = low_process_queue.tail;
  low_process_queue.tail = NULL;


  prn_split_str("multilevel queue init end");

  return 1;
}
/* main function */
int main (int    argc, char **argv)
{
  
  /* main start */
  prn_split_str("main start");
  
  int sched;
  
  /* multi level queue initialize */
  if(inititalize_multilevel_queue() == -1){
      MSG("multi level queue initialize failed \n");
      return -1;
  }  
    
  /* argument check */
  if (argc <= 1)
    {
      MSG ("input file must specified\n");
      return -1;
    }

  /* correct config check */
  if (read_config (argv[1]))
    {
      MSG ("failed to load config file '%s': %s\n", argv[1], STRERROR);
      return -1;
    }

  
  MSG("<ID LIST>\n");
  for(int i = 0 ; i < num_of_all_task; i++){
    MSG("[%d]: id - %s , prt - %d , type- %c\n", i, all_process_queue[i].id, all_process_queue[i].priority, all_process_queue[i].process_type);
  }
  prn_split_line();

  /* simulate */
  simulate ();
  
  /* main complete */
  prn_split_str("main end");

  return 0;
}
