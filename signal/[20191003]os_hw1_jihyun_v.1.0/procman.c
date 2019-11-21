/*
 * OS Assignment #1
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/signalfd.h> // signalfd();

#define MSG(x...) fprintf (stderr, x)
#define STRERROR  strerror (errno)

#define ID_MIN 2
#define ID_MAX 8
#define COMMAND_LEN 256

typedef enum
{
  ACTION_ONCE,
  ACTION_RESPAWN,

} Action;

typedef struct _Task Task;
struct _Task
{
  Task          *next; // 다음 Task 가리킬 pointer

  volatile pid_t pid;
  int            piped;
  int            pipe_a[2];
  int            pipe_b[2];
	
  char           id[ID_MAX + 1]; 			// 1st item
  char           pipe_id[ID_MAX + 1]; // 2nd item
  int						 order;								// 3rd item (new)
  Action         action;							// 4th item
  char           command[COMMAND_LEN];// 5th item
};

static Task *tasks;
static volatile int pid_num; // pid_num은 pid 값을 유효하지 않은 order에 넣어주기 위함.
static volatile int running;

static char * strstrip (char *str)
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
  if (len < ID_MIN || ID_MAX < len)
    return -1;

  for (i = 0; i < len; i++)
    if (!(islower (str[i]) || isdigit (str[i])))
      return -1;

  return 0;
}

static Task * lookup_task (const char *id)
{
  Task *task;

  for (task = tasks; task != NULL; task = task->next)
    if (!strcmp (task->id, id))
      return task;

  return NULL;
}

static Task * lookup_task_by_pid (pid_t pid)
{
  Task *task;

  for (task = tasks; task != NULL; task = task->next)
    if (task->pid == pid)
      return task;

  return NULL;
}

static void print_tasks_in_order (const Task * t) // order 번호 순서대로 task가 이어지는 지 출력하는 함수 
{
	MSG("[appended_task_print] arranged in ascending order\n");
  while(t)
  {
    MSG("[ task_id: %s, orderNo : %d ]", t->id, t->order);
    if(t->next != NULL)
    	MSG("  ->  ");
    t = t->next;
  }
  MSG("\n");
}

static void append_task (Task *task)
{
  Task *new_task;
  Task *iter; // tasks를 돌아다니면서 order에 맞게 노드를 연결 시켜줄 반복자 노드.
  Task *t;		// 연결된 tasks를 가리켜줄 노드
  new_task = malloc (sizeof (Task)); // new_task 동적할당
  if (new_task==NULL) // new_task 동적할당 실패한 경우
    {
      MSG ("failed to allocate a task: %s\n", STRERROR);
      return;
    }
 
  *new_task = *task;
 
  new_task->next = NULL; // 일단 다음 task에 아무것도 안가리키도록 함.
  if(!tasks)   // task가 하나도 없는 경우
      tasks = new_task;
 
  //when there is more then one task in tasks
  else
  {
   iter = malloc(sizeof(Task)); // 하나 이상의 task가 있는 경우 iter 동적할당
     if (iter==NULL) // iter 동적할당 실패한 경우
    {
      MSG ("failed to allocate a task: %s\n", STRERROR);
      return;
    }
   iter -> next = NULL; // 일단 다음 task에 아무것도 안가리키도록 함.
   t = tasks;
   
   //order 값이 작을 수록 우선순위 올라감
   if(new_task-> order < t->order) // 새로 들어온 task의 order 즉, 우선순위가 현재 tasks의 모음 중 하나보다 높을 때
   {
        new_task-> next = t;
        tasks = new_task;
   }
   else{ // 새로운 task의 order가 작지 않을 때.
        while(t) // t가 NULL이 아닐 때 까지, 즉 현재 링크드리스트의 마지막(==NULL)까지 반복
        {
            if(t->order > new_task->order) // 새로 들어온 task의 order 즉, 우선순위가 현재 tasks의 모음 중 하나보다 높을 때
            {
                new_task->next =t;
                iter->next = new_task;
                break; // 연결 완료 했으므로 break;
            }
 
            iter = t;
            t = t->next;     
        }
        // new_task order의 값이 가장 클 때.
        for(t = tasks;t->next != NULL ;t =t->next); // 일단 맨 뒤로 task * t를 보낸다. 그래야 맨마지막거와 비교하기 때문.
        if(t->order <= new_task->order) // 작거나 같은경우만, 무조건 뒤로 보낸다.
            t->next = new_task;
   }
  }
  
  // order대로 제대로 연결 되었는지 확인 
  t = tasks;
  print_tasks_in_order (t);
}



// 추가 시킨 함수
static int check_valid_order (const char* str, int size) // valid order : return 0, invalid order : return -1
{
	int i;
	int isZero = 0;
	
	for (i=0;i<size;i++){
		isZero = 0;
		if(!('0'<=str[i]&&str[i]<='9')) return -1; // invalid order (NULL or 올바른 숫자가 아닌 경우)
		else if (str[i]=='\0')
		{
			isZero = 1;
		}
	}
	return isZero;
}

static int read_config (const char *filename) // read config txt and make an order.
{
  FILE *fp;
  char  line[COMMAND_LEN * 2];
  int   line_nr;

  fp = fopen (filename, "r");
  if (!fp)
    return -1;

  tasks = NULL;

  line_nr = 0;
  while (fgets (line, sizeof (line), fp))
    {
      Task   task;
      char  *p;
      char  *s;
      char  *tmp; // order 값이 valid 또는 invalid한지 알아보기 위한 char형 포인터
      int		 tmp_size=0;   // check_valid_order에서 order의 문자 하나하나를 검사하기 위한 크기 지정자
      int		 order_valid_res; // valid한지, invalid면 공백인지, 공백이아니라 진짜 숫자가 아닌게 섞인지에 따라 나오는 결과를 받는 변수
      size_t len;

      line_nr++;
      memset (&task, 0x00, sizeof (task));

      len = strlen (line);
      if (line[len - 1] == '\n')
        line[len - 1] = '\0';

      if (0)
        MSG ("config[%3d] %s\n", line_nr, line);

      strstrip (line);

      /* comment or empty line */
      if (line[0] == '#' || line[0] == '\0')
        continue;

      /* 1st item: id */
      s = line;
      p = strchr (s, ':');
      if (!p)
        goto invalid_line;
      *p = '\0';
      strstrip (s);
      if (check_valid_id (s))
        {
          MSG ("invalid id '%s' in line %d, ignored\n", s, line_nr);
          continue;
        }
      if (lookup_task (s))
        {
          MSG ("duplicate id '%s' in line %d, ignored\n", s, line_nr);
          continue;
        }
      strcpy (task.id, s);

      /* 2nd item: action */
      s = p + 1;
      p = strchr (s, ':');
      if (!p)
        goto invalid_line;
      *p = '\0';
      strstrip (s);
      if (!strcasecmp (s, "once"))
        task.action = ACTION_ONCE;
      else if (!strcasecmp (s, "respawn"))
        task.action = ACTION_RESPAWN;
      else
        {
          MSG ("invalid action '%s' in line %d, ignored\n", s, line_nr);
          continue;
        }
			/* 3rd item: order */
			s = p + 1;
      p = strchr (s, ':');
      if (!p)
        goto invalid_line;
      *p = '\0'; 
    tmp=strstrip(s); // 문자열 자르기 성공한 부분을 넘겨줌.
    while(tmp[tmp_size]!='\0')
    {
    	++tmp_size; // 문자열 크기를 알기위한 연산 -> check_valid_order 확인용
    }
    task.order = atoi(s);
    order_valid_res = check_valid_order(s,--tmp_size);
    if(order_valid_res==0) // order 항목이 유효하지 않은 경우 중 0인경우에 한해 현재 pid 값이 저장된 pid_num을 대입(후위증가연산)
    	{
      	task.order = pid_num++;
      } else if (order_valid_res == -1){ // order 항목에 숫자가 아닌 것이 끼어있는 경우
      	 MSG ("invalid order '%s' in line %d, ignored\n", s, line_nr);
              continue;
      } 
      else {
      	task.order = atoi(s); // 유효할 경우는 atoi()함수로 문자열->숫자로 바꿔줌.
      }
      
       MSG("// current line : %d //\n ",line_nr);
       MSG(" task_id: %s, order: %d \n\n",task.id, task.order); // 

      /* 4th item: pipe-id */
      s = p + 1;
      p = strchr (s, ':');
      if (!p)
        goto invalid_line;
      *p = '\0';
      strstrip (s);
      if (s[0] != '\0')
        {
          Task *t;

          if (check_valid_id (s))
            {
              MSG ("invalid pipe-id '%s' in line %d, ignored\n", s, line_nr);
              continue;
            }

          t = lookup_task (s);
          if (!t)
            {
              MSG ("unknown pipe-id '%s' in line %d, ignored\n", s, line_nr);
              continue;
            }
          if (task.action == ACTION_RESPAWN || t->action == ACTION_RESPAWN)
            {
              MSG ("pipe not allowed for 'respawn' tasks in line %d, ignored\n", line_nr);
              continue;
            }
          if (t->piped)
            {
              MSG ("pipe not allowed for already piped tasks in line %d, ignored\n", line_nr);
              continue;
            }

          strcpy (task.pipe_id, s);
          task.piped = 1;
          t->piped = 1;
        }

      /* 5th item: command */
      s = p + 1;
      strstrip (s);
      if (s[0] == '\0')
        {
          MSG ("empty command in line %d, ignored\n", line_nr);
          continue;
        }
      strncpy (task.command, s, sizeof (task.command) - 1);
      task.command[sizeof (task.command) - 1] = '\0';

      if (0)
        MSG ("id:%s pipe-id:%s action:%d command:%s\n",
             task.id, task.pipe_id, task.action, task.command);

      append_task (&task);
      continue;

    invalid_line:
      MSG ("invalid format in line %d, ignored\n", line_nr);
    }
  
  fclose (fp);

  return 0;
}

static char ** make_command_argv (const char *str)
{
  char      **argv;
  const char *p;
  int         n;

  for (n = 0, p = str; p != NULL; n++)
    {
      char *s;

      s = strchr (p, ' ');
      if (!s)
        break;
      p = s + 1;
    }
  n++;

  argv = calloc (sizeof (char *), n + 1);
  if (!argv)
    {
      MSG ("failed to allocate a command vector: %s\n", STRERROR);
      return NULL;
    }

  for (n = 0, p = str; p != NULL; n++)
    {
      char *s;

      s = strchr (p, ' ');
      if (!s)
        break;
      argv[n] = strndup (p, s - p);
      p = s + 1;
    }
  argv[n] = strdup (p);

  if (0)
    {

      MSG ("command:%s\n", str);
      for (n = 0; argv[n] != NULL; n++)
        MSG ("  argv[%d]:%s\n", n, argv[n]);
    }

  return argv;
}

static void spawn_task (Task *task)
{
  if (0) MSG ("spawn program '%s'...\n", task->id);

  if (task->piped && task->pipe_id[0] == '\0')
    {
      if (pipe (task->pipe_a))
        {
          task->piped = 0;
          MSG ("failed to pipe() for program '%s': %s\n", task->id, STRERROR);
        }
      if (pipe (task->pipe_b))
        {
          task->piped = 0;
          MSG ("failed to pipe() for program '%s': %s\n", task->id, STRERROR);
        }
    }
  
  task->pid = fork ();
  if (task->pid < 0)
    {
      MSG ("failed to fork() for program '%s': %s\n", task->id, STRERROR);
      return;
    }

  /* child process */
  if (task->pid == 0)
    {
      char **argv;

      argv = make_command_argv (task->command);
      if (!argv || !argv[0])
        {
          MSG ("failed to parse command '%s'\n", task->command);
          exit (-1);
        }

      if (task->piped)
        {
          if (task->pipe_id[0] == '\0')
            {
              dup2 (task->pipe_a[1], 1);
              dup2 (task->pipe_b[0], 0);
              close (task->pipe_a[0]);
              close (task->pipe_a[1]);
              close (task->pipe_b[0]);
              close (task->pipe_b[1]);
            }
          else
            {
              Task *sibling;

              sibling = lookup_task (task->pipe_id);
              if (sibling && sibling->piped)
                {
                  dup2 (sibling->pipe_a[0], 0);
                  dup2 (sibling->pipe_b[1], 1);
                  close (sibling->pipe_a[0]);
                  close (sibling->pipe_a[1]);
                  close (sibling->pipe_b[0]);
                  close (sibling->pipe_b[1]);
                }
            }
        }

      execvp (argv[0], argv);
      MSG ("failed to execute command '%s': %s\n", task->command, STRERROR);
      exit (-1);
    }
    sleep(1); // 1sec delay : to find out if task starts in order or not.
}

static void spawn_tasks (void)
{
  Task *task;

  for (task = tasks; task != NULL && running; task = task->next)
    spawn_task (task);
}

static void wait_for_children (int signo)
{
  Task *task;
  pid_t pid;

 rewait:
  pid = waitpid (-1, NULL, WNOHANG);
  if (pid <= 0)
    return;

  task = lookup_task_by_pid (pid);
  if (!task)
    {
      MSG ("unknown pid %d", pid);
      return;
    }

  if (0) MSG ("program[%s] terminated\n", task->id);

  if (running && task->action == ACTION_RESPAWN)
    spawn_task (task);
  else
    task->pid = 0;

  /* some SIGCHLD signals is lost... */
  goto rewait;
}

static void terminate_children (int signo)
{
  Task *task;

  if (1) MSG ("terminated by SIGNAL(%d)\n", signo);

  running = 0;

  for (task = tasks; task != NULL; task = task->next)
    if (task->pid > 0)
      {
        if (0) MSG ("kill program[%s] by SIGNAL(%d)\n", task->id, signo);
        kill (task->pid, signo);
      }

  exit (1);
}

int main (int    argc, char **argv)
{
  struct sigaction sa;
  int terminated;
	int sfd; // signalfd() 결과 값 받기위한 변수
	unsigned int called_Sig; // signal발생시 그 signal 값을 저장할 곳
	sigset_t mask;
	
	pid_num = getpid();			// main함수돌면서 실행되는 getpid()를 통해 static 전역변수 pid_num 값 초기화
	sigemptyset(&mask);				 // signal 저장하는 set인 mask 비우기
	sigaddset(&mask, SIGCHLD); // SIGINT를 mask set에 더함 
	
	// sigprocmask에 의해 mask에 있던 signal은 사용자가 지정한 signal 함수에 반응하기 위해 Block됨
	if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) // 실패할 경우
	{
		perror("sigprocmask error\n\n");
		return -1;
	}
	
	sfd = signalfd(-1,&mask, 0);
	
	if (sfd<0)  // 실패할 경우
	{
		perror("signalfd error\n\n");
		return -1;
	}
	
  if (argc <= 1)
    {
      MSG ("usage: %s config-file\n", argv[0]);
      return -1;
    }

  if (read_config (argv[1]))
    {
      MSG ("failed to load config file '%s': %s\n", argv[1], STRERROR);
      return -1;
    }
	
  running = 1;
  spawn_tasks ();
  
  while(1){
  	struct signalfd_siginfo sfd_info;
  	read(sfd, &sfd_info, sizeof(sfd_info));
  	
  	called_Sig = sfd_info.ssi_signo;
  	
  	if (called_Sig == SIGCHLD)
  	{
  		wait_for_children(called_Sig);
  	}
  
  }

  sigemptyset (&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = terminate_children;
  if (sigaction (SIGINT, &sa, NULL))
    MSG ("failed to register signal handler for SIGINT\n");

  sigemptyset (&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = terminate_children;
  if (sigaction (SIGTERM, &sa, NULL))
    MSG ("failed to register signal handler for SIGINT\n");

  terminated = 0;
  while (!terminated)
    {
      Task *task;

      terminated = 1;
      for (task = tasks; task != NULL; task = task->next)
        if (task->pid > 0)
          {
            terminated = 0;
            break;
          }

      usleep (100000);
    }
  
  return 0;
}

