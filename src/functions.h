#ifndef _FUNCTIONS_
#define _FUNCTIONS_
	
#define POOL_ACTIVE 0
#define POOL_FINISHED 1	
		
#define JOB_ACTIVE 0
#define JOB_SUSPENDED 1
#define JOB_FINISHED 2	
	
void catch_sig(int);
	
int countWords(char *);
	
int format_time();
	
int format_date();

extern int pool_jobs_cur;
extern int jobs_info[100][3];

#endif