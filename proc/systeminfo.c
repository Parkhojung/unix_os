#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 200
char buf[BUF_SIZE];


void prn(char *str){
	printf("\n//%s\n", str);
}
void print_buf(){
	printf("%s",buf);
}
void get_line(char * filename, char* str){

	FILE * fp;

	fp =  fopen(filename, "r");
	while(fgets(buf, BUF_SIZE, fp) != 0 ){	
	//	printf("%s", buf);
		if(strstr(buf, str) != NULL)
			break;
	}
	
	print_buf();
	fclose(fp);
}

void get_line_2(char * filename, int line){
	
	FILE* fp =  fopen(filename, "r");
	int check_line = 1;

	while(fgets(buf, BUF_SIZE, fp) != 0 ){	
	//	printf("%s", buf);
		if(check_line == line)
			break;
		check_line++;
	}
	
	print_buf();
	fclose(fp);
}
	

int main(){

	int fd;
	int res;

	prn("meminfo");
	get_line("/proc/meminfo", "MemTotal");
	get_line("/proc/meminfo", "MemFree");

	prn("kernel version");
	get_line("/proc/version","Linux version");

	prn("cpu info");
	get_line("/proc/cpuinfo", "sibling");
	get_line("/proc/cpuinfo", "cpu cores");		
	get_line("/proc/cpuinfo", "model name");

	prn("up time");	
	get_line_2("/proc/uptime",1);

	prn("context switches");
	get_line("/proc/stat","ctxt");	

	prn("interrupt number");
	

	prn("load average in 15min");
	get_line("/proc/loadavg","");
	
	return 0;
}


