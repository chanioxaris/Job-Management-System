#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>

#include "functions.h"

//Custom signal handler for pool process	
void catch_sig(int signo)
	{
	int i, pool_jobs_cur;
	int jobs_info[pool_jobs_cur+1][3];
	
	for(i = 1 ; i <= pool_jobs_cur ; i++)
		{
		if(jobs_info[i][1] == JOB_ACTIVE || jobs_info[i][1] == JOB_SUSPENDED) 
			{
			kill(jobs_info[i][0], SIGTERM);
			wait(NULL); 
			}		
		}
	exit(0);	
	}
	

int countWords(char *str)
	{
	int state = 1, count = 0;
 
	//Scan string character by character 
	while (*str)
		{
		//If next character is a separator, set the state as OUT
		if (*str == ' ' || *str == '\n' || *str == '\t')			
			state = 1; 			
		else if (state)
			{
			state = 0;
			count++;
			}
			++str;
		}
	return count;
	}		

//Return time in HHMMSS format	
int format_time()
	{
	time_t rawtime;
	struct tm *timeinfo;

	rawtime = time(NULL);
	timeinfo = localtime(&rawtime);
	
	return (timeinfo->tm_hour*10000 + timeinfo->tm_min*100 + timeinfo->tm_sec);
	}

//Return date in YYYYMMDD format		
int format_date()
	{
	time_t rawtime;
	struct tm *timeinfo;

	rawtime = time(NULL);
	timeinfo = localtime(&rawtime);
	
	return ((timeinfo->tm_year + 1900)*10000 + (timeinfo->tm_mon + 1)*100 + timeinfo->tm_mday);
	}	