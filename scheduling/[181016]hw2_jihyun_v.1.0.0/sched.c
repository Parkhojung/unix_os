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

#define PROCESS_MAX (26 * 10) // 지정할 수 있는 프로세스 개수는 최대 260개 알파벳(26개:A~Z) * 숫자(10개:0~9)
#define ID_MAX      2					// 알파벳 대문자, 숫자로 구성 (중복 허용 X)

/* 프로세스 도착시간 0 이상 30 이하 정수*/
#define ARRIVE_TIME_MIN 0
#define ARRIVE_TIME_MAX 30
/* 프로세스 서비스시간 1 이상 30 이하 정수*/
#define SERVICE_TIME_MIN 1
#define SERVICE_TIME_MAX 30
/* 프로세스 우선순위 1 이상 10 이하 정수 (작을 수록 우선순위 높음)*/
#define PRIORITY_MIN 1
#define PRIORITY_MAX 10


#define SLOT_MAX ((ARRIVE_TIME_MAX + SERVICE_TIME_MAX) * PROCESS_MAX) // ((30+30) * 260)

enum
{
  SCHED_SJF = 0,	// SCHED_SJF (Shortest Job First)의 값 = 0
  SCHED_SRT,			// SCHED_SRT (Shortest Remaining Time First)의 값 = 1
  SCHED_RR,				// SCHED_RR	 (Round Robin)의 값 = 2
  SCHED_PR,				// SCHED_PR	 (Priority)의 값 = 3 		*단, preemptive 이다.
  SCHED_MAX				// SCHED_MAX 의 값 = 4
};    

typedef struct _Process Process; // 구조체명 _Process을 Process로 재정의
struct _Process
{
  int    idx;					// id번호
  int    queue_idx;		        // 기다리는 값

  char   id[ID_MAX + 1];	// id값 char형 원소 3개 갖음
  int    arrive_time;			// 도착시간
  int    service_time;		// 서비스 수행 시간
  int    priority;				// 우선순위

  int    remain_time;			// 완료까지 남은 시간
  int    complete_time;		// 종료 시각
  int    turnaround_time;	// 완료 시간
  int    wait_time;				// 대기 시간
};

static Process  processes[PROCESS_MAX]; // Process 구조체 배열로, 원소 갯수는 260개임. *전역 스태틱 변수
static int      process_total;					// 기본값 0. 프로세스가 추가될 경우 ++1되는 현재 프로세스들 갯수 *전역 스태틱 변수

static Process *queue[PROCESS_MAX];			// Process 구조체를 가리킬 수 있는 Process 포인터 변수의 모음 queue 배열. 원소 갯수 260개 * 전역 스태틱 변수
static int      queue_len;							// queue 배열의 갯수

/*프로세스 시뮬레이션 시 시각화 해줄 배열 : 프로세스 최대 갯수만큼 행, 각 프로세스가 가질 수 있는 최대 소요시간 만큼 열(슬롯) 할당*/
static char     schedule[PROCESS_MAX][SLOT_MAX]; // 2차원 char형 배열 schedule.


static char* strstrip (char* str) {
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

static int check_valid_id (const char* str)
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

static Process* lookup_process (const char* id)
{
  int i;

  for (i = 0; i < process_total; i++)
    if (!strcmp (id, processes[i].id))
      return &processes[i];

  return NULL;
}

static void append_process (Process *process) // 프로세스 추가
{
  processes[process_total] = *process;
  // process 배열인 processes의 마지막 원소, 즉 맨 마지막 원소에 actual param으로 받은 process*가 가리키는 객체의 값 복사
  processes[process_total].idx = process_total;
  // processes 배열 맨 마지막 process 객체의 멤버 변수 idx에 process_total 값 대입
  process_total++;
  // process_total 값 1 추가 (스태틱이라 계속 반영됨.)
}

static int read_config (const char *filename)
{
  FILE *fp;
  char  line[256];
  int   line_nr;

  fp = fopen (filename, "r");
  if (!fp)
    return -1;

  process_total = 0;

  line_nr = 0;
  while (fgets (line, sizeof (line), fp))
    {
      Process process;
      char  *p;
      char  *s;
      size_t len;

      line_nr++;
      memset (&process, 0x00, sizeof (process));

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
      if (check_valid_id (s))
	{
	  MSG ("invalid process id '%s' in line %d, ignored\n", s, line_nr);
	  continue;
	}
      if (lookup_process (s))
	{
	  MSG ("duplicate process id '%s' in line %d, ignored\n", s, line_nr);
	  continue;
	}
      strcpy (process.id, s);

      /* arrive time */
      s = p + 1;
      p = strchr (s, ' ');
      if (!p)
	goto invalid_line;
      *p = '\0';
      strstrip (s);

      process.arrive_time = strtol (s, NULL, 10);
      if (process.arrive_time < ARRIVE_TIME_MIN
	  || ARRIVE_TIME_MAX < process.arrive_time
	  || (process_total > 0 &&
	      processes[process_total - 1].arrive_time > process.arrive_time))
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
	  || SERVICE_TIME_MAX < process.service_time)
	{
	  MSG ("invalid service-time '%s' in line %d, ignored\n", s, line_nr);
	  continue;
	}

      /* priority */
      s = p + 1;
      strstrip (s);
      process.priority = strtol (s, NULL, 10); // 현재 process의 priority를 s부터 NULL문자 까지 읽고, 10진수로 바꾼다. 
      if (process.priority < PRIORITY_MIN 		 // 유효한 priority 값이 아닌경우. 1~10사이의 정수가 아닌경우
	  || PRIORITY_MAX < process.priority)
	{
	  MSG ("invalid priority '%s' in line %d, ignored\n", s, line_nr);
	  continue; // 올라가서 다음 줄 읽기
	}
	
      append_process (&process); // 프로세스 추가.
      continue;

    invalid_line:
      MSG ("invalid format in line %d, ignored\n", line_nr);
    }
  
  fclose (fp);

  return 0;
}

static void simulate (int sched)	// process scheduling algorithm 시뮬레이션 하는 함수
{
  Process* process;		// Process* 형 변수 process (현재 실행되는 프로세스 나타냄)
  int      p;					// int 형 변수 p (프로세스 갯수)
  int      p_done;		// int 형 변수 p (완료된 프로세스 갯수)
  int      cpu_time;	// int 형 변수 cpu_time (cpu_time 나타냄)
  int      sum_turnaround_time; // int 형 변수 sum_turnaround_time (소요시간(대기시간+서비스시간) 합계)
  int      sum_waiting_time;		// int 형 변수 sum_waiting_time (대기시간 합계)
  float    avg_turnaround_time; // float 형 변수 avg_turnaround_time (평균 소요시간(대기시간+서비스시간))
  float    avg_waiting_time;		// float 형 변수 avg_waiting_time (평균 대기시간)

  for (p = 0; p < PROCESS_MAX; p++) // 프로세스 최대갯수(260) 까지 반복
    {
      int slot;

      for (slot = 0; slot < SLOT_MAX; slot++)
    
	          schedule[p][slot] = 0; // 반복문으로 char 배열 schedule의 모든 원소 0으로 초기화
            queue[p] = NULL; 	 // 반복문으로 Process* 배열 queue의 모든 원소가 아무것도 가리키지 않도록 초기화 
    }

  p = 0;
  p_done = 0;
  queue_len = 0;
  process = NULL;				// Process* 형 변수도 아무것도 가리키지 않도록 초기화

  for (cpu_time = 0; p_done < process_total; cpu_time++)
    {
      /* Insert arrived process into the queue. */
      for (; p < process_total; p++)
			{
	  		Process *pp;
	  		pp = &processes[p];
	  		if (pp->arrive_time != cpu_time) break;
	  		pp->remain_time = pp->service_time;
	  		pp->queue_idx = queue_len;
	  		
	  		queue[queue_len] = pp;
	  		queue_len++;
			}  	
    	  /* Pick a process according to scheduling algorithm. */
      switch (sched) 
	{
	case SCHED_SJF: // shortest job first scheduling algorithm
	  if (!process)
	    {
	      int i;
	      int shortest;

	      shortest = SERVICE_TIME_MAX + 1; 					// shortest 값을 최대+1로 설정
	      for (i = 0; i < queue_len; i++)
					if (queue[i]->service_time < shortest)	// queue[i]의 service_time이 shortest보다 작은 경우(더 짧은 service)
		  		{
		    		process = queue[i];										// 현재 process가 queue[i]가 가리키는 것과 같은 Process 가리키도록 함
		    		shortest = process->service_time;			// shortest에 현재 process의 멤버 service_time 값 복사
		 	 		}
	    }
	  break;
	case SCHED_SRT: // shortest remaining time first scheduling algorithm 
	  {
	    int i;
	    int shortest;

	    shortest = SERVICE_TIME_MAX + 1; // shortest 값을 최대+1로 설정
	    for (i = 0; i < queue_len; i++)
	      if (queue[i]->remain_time < shortest) // queue[i]의 service_time이 shortest보다 작은 경우(더 짧은 service)
			{
		  	process = queue[i];										// 현재 process가 queue[i]가 가리키는 것과 같은 Process(구조체) 가리키도록 함 
		  	shortest = process->remain_time;			// shortest에 현재 process의 멤버 service_time 값 복사
			}
	  }
	  break;
	case SCHED_RR: // round-robin scheduling algorithm
	  {
	    int i;
	    
	    process = queue[0];		// process는 queue의 첫번째 원소가 가리키는 것과 같은 Process(구조체) 가리키도록 함
	    for (i = 0; i < (queue_len - 1); i++) // queue가 2개 이상일 때
	    {
					queue[i] = queue[i + 1]; // queue[i] 원소가 queue[i+1]번째 원소가 가리키는 것과 같은 Process(구조체) 가리키도록 함
					queue[i]->queue_idx = i; // queue[i] 원소의 멤버 queue_idx를 i 값으로 바꿈.
	    }
	    queue[i] = process;					 // queue[i] 원소에 현재 process가 가리키는 것과 같은 Process(구조체) 가리키도록 함
	    queue[i]->queue_idx = i;		 // queue[i] 원소의 멤버 queue_idx를 i 값으로 바꿈.
	  }
	  break;
	case SCHED_PR: // priority scheduling algorithm (preemptive)	
	  /*
	  SCHED_PR 구동 원리 :
	  1. priority 정렬을 위한 임시변수 설정
	  	 : 블럭 내의 int형 변수 tmp_PR을 설정하고 PRIORITY_MAX+1, 즉 priority 최대값(최하위) + 1 값 주었음
	  2. priority가 같을 경우의 임의 규칙 설정 (service_time이 짧은 것을 먼저 실행하는 SJF scheduling algorithm 적용)
	     : 블럭 내의 int형 변수 tmp_service을 설정하고 SERVICE_TIME_MAX+1, 즉 service_time 최대값(30) + 1 값 주었음
	  3. queue 를 순회
	  	 : queue[i]가 가리키는 Process 구조체의 멤버 priority와 tmp_PR을 비교
	  	 * priority가 tmp_PR보다 작은 경우
	  	 		 - 현재 process가 queue[i]가 가리키는 것과 같은 Process 구조체 가리키도록 함
	  	 		 - tmp_PR에 process가 가리키는 Process 구조체의 멤버 priority 값 복사
	  	 		 - tmp_service에 process가 가리키는 Process 구조체의 멤버 service_time 값 복사  	 		 
	  	 * priority가 tmp_PR과 같은 경우
	 				 - queue[i]가 가리키는 Process 구조체의 멤버 service_time이 tmp_service보다 작은 경우,
	 				 	 현재 process가 queue[i]가 가리키는 것과 같은 Process 구조체 가리키도록 함
	 				 - tmp_service에 process가 가리키는 Process 구조체의 멤버 service_time 값 복사
	  */
	  
	  {
	  	int i;																// index 접근을 위한 int형 변수 i
	  	int tmp_PR = PRIORITY_MAX+1;					// priority 정렬을 위한 임시변수 tmp_PR 선언 및 초기화
	  	int tmp_service = SERVICE_TIME_MAX+1; // priority가 같을 경우 SJF algorithm 정렬을 위한 임시변수 tmp_service 선언 및 초기화
	  	for (i=0;i<queue_len;++i)
	  	{
	  		if(queue[i]->priority < tmp_PR) // queue[i]의 priority가 tmp_PR보다 작을 경우
	  		{
	  			process = queue[i];						// 현재 process가 queue[i]가 가리키는 것과 같은 Process 구조체 가리키도록 함
	  			tmp_PR = process->priority;		// tmp_PR에 process가 가리키는 Process 구조체의 멤버 priority 값 복사
	  		}
	  		else if (queue[i]->priority == tmp_PR) // queue[i]의 priority가 tmp_PR과 같은 경우
	  		{
	  			if (queue[i]->service_time < tmp_service) // queue[i]의 service_time이 tmp_service보다 작은 경우
	  			{
	  				process = queue[i];											// 현재 process가 queue[i]가 가리키는 것과 같은 Process 구조체 가리키도록 함
	  				tmp_service = process->service_time;		// tmp_service에 process가 가리키는 Process 구조체의 멤버 service_time 값 복사
	  			}
	  		}
	  	}
	  }
	  
	  break;
	default:
	  MSG ("invalid scheduing algorithm '%d', ignored\n", sched); // sched 값에 SJF, SRT, RR, PR이외의 값이 들어온 경우
	  return;
	}

      if (1)
	MSG ("[%02d] %s[%d:%d] %d/%d\n",
	     cpu_time,
	     process->id,
	     process->idx,
	     process->queue_idx,
	     process->remain_time,
	     process->service_time);
      
      /* no process to schedule. */
      if (!process)
	        continue;

      schedule[process->idx][cpu_time] = 1;
      process->remain_time--; // 현재 프로세스 남은 시간 1씩 줄임.
      if (process->remain_time <= 0)
	{
	  int i;

	  for (i = process->queue_idx; i < (queue_len - 1); i++)
	    {
	      queue[i] = queue[i + 1];
	      queue[i]->queue_idx = i;
	    }
	  queue_len--;

	  process->complete_time = cpu_time + 1;
	  // 프로세스의 완료 시각은 cpu_time + 1 (완료를 정하는 시간까지 +1)
	  process->turnaround_time = process->complete_time - process->arrive_time;
	  // 프로세스가 완료되는데 걸린 시간은 프로세스의 완료시각(complete_time) - 프로세스의 도착시각(arrive_time)
	  process->wait_time = process->turnaround_time - process->service_time;
		// 프로세스가 기다린 시간은 프로세스가 완료되는데 걸린시간(turnaround_time) - 프로세스가 실제 실행된 시간(serviced_time)
	  process = NULL;
	  p_done++; // 완료된 프로세스 갯수 +1 증가
	}
    }
    /* for end*/
  
  printf ("\n[%s]\n",
	  sched == SCHED_SJF ? "SJF" :
	  sched == SCHED_SRT ? "SRT" :
	  sched == SCHED_RR  ? "RR" :
	  sched == SCHED_PR  ? "PR" : "UNKNOWN");

  sum_turnaround_time = 0; // sum_turnaround_time 값 0 으로 초기화
  sum_waiting_time = 0; 	 // sum_waiting_time 값 0 으로 초기화
  for (p = 0; p < process_total; p++) // Gantt Chart 출력
    {
      int slot;

      printf ("%s ", processes[p].id);						// processes 원소에 해당하는 id 값 출력
      for (slot = 0; slot <= cpu_time; slot++)	
				putchar (schedule[p][slot] ? '*' : ' ');	// process별로 schedule에 잡힌 시간 *로 표시
      printf ("\n");

      sum_turnaround_time += processes[p].turnaround_time; // sum_turnaround_timeprocesses(소요시간 합)에 원소의 turnaround_time(소요시간) 계속 더해감
      sum_waiting_time += processes[p].wait_time;					 // sum_waiting_time(대기시간 합)에 processes 원소의 waiting_time(대기시간) 계속 더해감
    }

  avg_turnaround_time = (float) sum_turnaround_time / (float) process_total; // avg_turnaround_time에 소요시간 합 / 추가된 프로세스 갯수 연산한 값 복사
  avg_waiting_time = (float) sum_waiting_time / (float) process_total;			 // avg_waiting_time에 대기시간 합 / 추가된 프로세스 갯수 연산한 값 복사

  printf ("CPU TIME: %d\n", cpu_time);
  printf ("AVERAGE TURNAROUND TIME: %.2f\n", avg_turnaround_time);
  printf ("AVERAGE WAITING TIME: %.2f\n", avg_waiting_time);
}

int main (int    argc, char **argv)
{
  int sched;

  if (argc <= 1) // terminal에 실행시키는 문자열만 입력한 경우
    {
      MSG ("usage: %s input-file\n", argv[0]);
      return -1;
    }

  if (read_config (argv[1])) // 두번째 문자열(읽을 file) 읽었을 때 읽기 실패할 경우
    {
      MSG ("failed to load config file '%s': %s\n", argv[1], STRERROR);
      return -1;
    }

  for (sched = 0; sched < 1; sched++) // sched이 4보다 작을 때까지 반복
    simulate (sched);	// simulate 함수 실행
  
  return 0;
}
