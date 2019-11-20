#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BUF_SIZE 200
#define DEBUG_FLAG 0
char buf[BUF_SIZE];


void prn(char *str){
	printf("\n//%s\n", str);
}
void print_buf(){
	if(DEBUG_FLAG)
		printf("%s",buf);
}
int get_line(char * filename, char* str){

	FILE * fp;
	int check_line = 0;

	fp =  fopen(filename, "r");
	while(fgets(buf, BUF_SIZE, fp) != 0 ){	
		check_line++;
	//	printf("%s", buf);
		if(strstr(buf, str) != NULL)
			break;
	}
	
	print_buf();
	fclose(fp);
	return check_line;
}

int get_line_2(char * filename, int line){
	
	FILE* fp =  fopen(filename, "r");
	int check_line = 0;

	while(fgets(buf, BUF_SIZE, fp) != 0 ){	
	//	printf("%s", buf);
		check_line++;
		if(check_line == line)
			break;
	}
	
	if (line == -1 && DEBUG_FLAG){
		printf("last line in %s :", filename);	
	} 
	print_buf();
	fclose(fp);

	return check_line;
}
int get_line_3(char * filename, char * str){
	
	FILE* fp =  fopen(filename, "r");
	int check_count = 0;

	while(fgets(buf, BUF_SIZE, fp) != 0 ){	
		if( strstr(buf,str) != NULL)
			check_count++;
	}
	fclose(fp);

	return check_count;
}
char* get_strtok(char * delim, int order){
	
	int check_order = 0;
	char *token = strtok (buf, delim);
  	
	while (token != NULL)
 	{
		check_order++;
		if(check_order == order)
			break;	
    		token = strtok (NULL, delim);
  	}
	/*
	printf("[in get_strtok]\n");
	printf("tok : %s\n", token);
	printf("buf : %s\n", buf); 
	*/
	return token;
}	

int main(){

	int fd;
	int res;
	int memTotal, memFree;
	int uptime;
	int processor_cnt, sibling;
	time_t t;
	struct tm tm;

	prn("meminfo");
	get_line("/proc/meminfo", "MemTotal");
	memTotal = atoi(get_strtok(" :",2)); 	
	//print_buf();
	get_line("/proc/meminfo", "MemFree");
	memFree = atoi(get_strtok( ":", 2));
	printf("** memory usage : %d kB\n", memTotal-memFree);
	printf("** memory free  : %d kB\n", memFree);

	prn("kernel version");
	get_line("/proc/version","Linux version");
	printf("%s\n",get_strtok(" ", 3));

	prn("cpu info");
	get_line("/proc/cpuinfo", "sibling");
	sibling = atoi(get_strtok(":",2));
	get_line("/proc/cpuinfo", "cpu cores");		
	get_line("/proc/cpuinfo", "cpu MHz");
	processor_cnt = get_line_3("/proc/cpuinfo","processor");
	printf("**logical processor count : %d\n", processor_cnt);
	printf("**physical processor count : %d\n", processor_cnt/sibling);

	prn("up time");	
	get_line_2("/proc/uptime",1);
	uptime=(int)atof(get_strtok(" ", 1));
	if(DEBUG_FLAG)
		printf("uptime: %d\n", uptime);	
	
	t=time(NULL);
	tm = *localtime(&t);
	
	if(DEBUG_FLAG)
		printf("**now: %d-%d %d:%d:%d:%d\n", tm.tm_year + 1900, tm.tm_mon + 1,tm.tm_mday, tm.tm_hour    , tm.tm_min, tm.tm_sec);
	
	t -= uptime;
	tm = *localtime(&t);
	printf("**boot time: %d-%d %d:%d:%d:%d\n", tm.tm_year + 1900, tm.tm_mon + 1,tm.tm_mday, tm.tm_hour    , tm.tm_min, tm.tm_sec);
	

	prn("context switches");
	get_line("/proc/stat","ctxt");	
	printf("** ctxt : %d\n", atoi(get_strtok(" ",2)));

	prn("interrupt number");
	res = get_line_2("/proc/interrupts",-1);	
	printf("** interrupt count: %d\n", res-1);

	prn("load average in 15min");
	get_line("/proc/loadavg","");
	printf("** load avg in 15m :%f\n", atof(get_strtok(" ",3)));	

	return 0;
}


