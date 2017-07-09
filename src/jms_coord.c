#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#include "functions.h"


int main(int argc, char* argv[])
	{
	int i, j, jobs_max, jobs_cur = 0, fd_jms_in, fd_jms_out, pools = 0, stat;
	int fd_pool_in, fd_pool_out;
	char jms_in[20], jms_out[20], path[20];
	char *command;
	char pool_in[512], pool_out[512];
	char buf[512], buf_out[512], buf_copy[512], buf_in[512];
	char fullpath[512], fullpath_pool[512];
	int jobid, poolid;
	char arg1[2], arg2[2];
	
	pid_t pid, pid_pool;
	
	
	int **pools_info = malloc(pools * sizeof(int *));
	for (i = 0; i < pools; i++) 
		pools_info[i] = malloc(2 * sizeof(int));
		
	//Parse arguments from command line
	if (argc == 9)									
		{
		for (i = 1 ; i < (argc-1) ; i++)
			{
			if (!strcmp(argv[i], "-w"))
				{
				stpcpy(jms_out, argv[++i]);
				continue;
				}
			if (!strcmp(argv[i], "-r"))
				{
				stpcpy(jms_in, argv[++i]);
				continue;
				}
			if (!strcmp(argv[i], "-l"))
				{
				stpcpy(path, argv[++i]);
				continue;
				}
			if (!strcmp(argv[i], "-n"))
				{
				jobs_max = atoi(argv[++i]);
				continue;
				}
			printf("Wrong input arguments! \n");
			return -1;
			}
		}
	else
		{
		printf("Wrong input arguments! \n");
		return -1;	
		}
		
	//Create directory to save the files of executables	
	if (mkdir(path, 0700) == -1) 
		{
        perror("mkdir");
        exit(4);
		}	
	

	//Create fifo pipes to communicate with console
	if (mkfifo(jms_in , 0666) == -1 )
		{
		if (errno != EEXIST) 
			{
			perror("mkfifo");
			exit(6);
			}
		}
	
	if (mkfifo(jms_out , 0666) == -1 )
		{
		if (errno != EEXIST) 
			{
			perror("mkfifo");
			exit(6);
			}
		}
	
	//Open fifos for communication with console	
	if ((fd_jms_in = open(jms_in , O_RDONLY)) < 0)
		{
		perror ("open");
		exit(1) ;
		}
	
	if ((fd_jms_out = open(jms_out , O_WRONLY)) < 0)
		{
		perror ("open");
		exit(1) ;
		}	
	
	while(1)
		{
		//Wait for any child process of coord (ex pool processes)					   
		pid_pool = waitpid(-1, &stat, WNOHANG);

		if (pid_pool > 0)
			{
			for (i = 1 ; i <= pools ; i++)
				{									
				if (pools_info[i][0] == pid_pool)
					{
					pools_info[i][1] = POOL_FINISHED;
					break;
					}	
				}		
			}		
			
		if (read(fd_jms_in,  buf_in, sizeof(buf_in) + 1) == -1)
			{
			perror("read");
			exit(3);
			}		
			
		strcpy(buf_copy, buf_in);
		
		command = strtok(buf_in," \t\n\r");
		
		if (!strcmp(command, "submit"))
			{	
			jobs_cur++;
			
			//Create new pool
			if ((jobs_cur % jobs_max) == 1)  
				{
				pools++;
								
				if (getcwd(fullpath, sizeof(fullpath)) == NULL)
					{
					perror("getcwd");
					exit(5);
					}
				sprintf(fullpath + strlen(fullpath), "/%s", path);	
				
				sprintf(pool_in, "%s/pool%d_in", fullpath, pools);
				sprintf(pool_out, "%s/pool%d_out", fullpath, pools);

		
				//Create fifo pipes to communicate with new pool
				if (mkfifo(pool_in , 0666) == -1 )
					{
					if (errno != EEXIST) 
						{
						perror("mkfifo");
						exit(6);
						}
					}				
						
				if (mkfifo(pool_out , 0666) == -1 )
					{
					if (errno != EEXIST) 
						{
						perror("mkfifo");
						exit(6);
						}
					}
				
			
				pid = fork();
				
				//Pool process
				if (pid == 0) 
					{						
					if (getcwd(fullpath_pool, sizeof(fullpath_pool)) == NULL)
						{
						perror("getcwd() error");
						exit(5);
						}
						
					sprintf(fullpath_pool + strlen(fullpath_pool), "/pool");						
					sprintf(arg1, "%d", pools);
					sprintf(arg2, "%d", jobs_max);
					
					execl(fullpath_pool, fullpath_pool, arg1, arg2 , path, (char*)NULL);							
					}
				//Coord	process
				else if (pid > 0)	
					{
					pools_info = realloc(pools_info, pools * sizeof(int *));
					pools_info[pools] = malloc(2 * sizeof(int));
					
					pools_info[pools][0] = pid;
					pools_info[pools][1] = POOL_ACTIVE;		
					}
				else
					{
					perror("fork");	
					exit(7);	
					}
				}

			if (getcwd(fullpath, sizeof(fullpath)) == NULL)
				{
				perror("getcwd");
				exit(5);
				}
			sprintf(fullpath, "%s/%s", fullpath, path);
			
		
			sprintf(pool_in, "%s/pool%d_in", fullpath, pools);
			sprintf(pool_out, "%s/pool%d_out", fullpath, pools);
	
			//Open fifo with pool
			if ((fd_pool_in = open(pool_in , O_WRONLY)) < 0)
				{ 
				perror("open"); 
				exit(1);
				}

			//Send data to latest pool
			if (write(fd_pool_in,  buf_copy, sizeof(buf_copy) + 1) == -1)
				{
				perror("Error in writing\n");
				exit(2);
				}
				
			if ((fd_pool_out = open(pool_out , O_RDONLY)) < 0)
				{ 
				perror("open"); 
				exit(1);
				}						
				
			//Wait for response from pool
			if (read(fd_pool_out,  buf, sizeof(buf) + 1) == -1)
				{
				perror("read");
				exit(3);
				}	
		
			sprintf(buf_out, "JobID: %d, PID: %s\n", jobs_cur, buf);
			}
		else if (!strcmp(command, "status"))
			{
			command = strtok(NULL," \t\n\r");	
			
			jobid = atoi(command);
			
			if (jobid % jobs_max == 0)
				poolid = jobid / jobs_max;
			else
				poolid = jobid / jobs_max + 1;
						
			
			if (pools_info[poolid][1] == POOL_FINISHED)
				sprintf(buf_out, "JobID %d Status: Finished\n", jobid);	
			
			else
				{					
				if (getcwd(fullpath, sizeof(fullpath)) == NULL)
					{
					perror("getcwd");
					exit(5);
					}
				sprintf(fullpath, "%s/%s", fullpath, path);	
					
					
				sprintf(pool_in, "%s/pool%d_in", fullpath, poolid);
				sprintf(pool_out, "%s/pool%d_out", fullpath, poolid);	
					
				if ((fd_pool_in = open(pool_in , O_WRONLY)) < 0)
					{ 
					perror("open"); 
					exit(1);
					}
			
				if (write(fd_pool_in,  buf_copy, sizeof(buf_copy) + 1) == -1)
					{
					perror("write");
					exit(2);
					}	
				
				if ((fd_pool_out = open(pool_out , O_RDONLY)) < 0)
					{ 
					perror("open"); 
					exit(1);
					}					
								
				if (read(fd_pool_out,  buf, sizeof(buf) + 1) == -1)
					{
					perror("read");
					exit(3);
					}	
				
				close(fd_pool_in);
				close(fd_pool_out);
				
				sprintf(buf_out, "JobID %d Status: %s\n", jobid, buf);					
				}
			}
		else if (!strcmp(command, "status-all"))
			{
			memset(buf_out, 0, sizeof(buf_out));

			for (i = 1 ; i <= pools ; i++)
				{
				if (pools_info[i][1] == POOL_ACTIVE)
					{					
					if (getcwd(fullpath, sizeof(fullpath)) == NULL)
						{
						perror("getcwd");
						exit(5);
						}
					sprintf(fullpath, "%s/%s", fullpath, path);	
						
					sprintf(pool_in, "%s/pool%d_in", fullpath, i);
					sprintf(pool_out, "%s/pool%d_out", fullpath, i);
					
					if ((fd_pool_in = open(pool_in , O_WRONLY)) < 0)
						{ 
						perror("open"); 
						exit(1);
						}
						
					if (write(fd_pool_in,  buf_copy, sizeof(buf_copy) + 1) == -1)
						{
						perror("write");
						exit(2);
						}
							
					if ((fd_pool_out = open(pool_out , O_RDONLY)) < 0)
						{ 
						perror("open"); 
						exit(1);
						}	

					if (read(fd_pool_out,  buf, sizeof(buf) + 1) == -1)
						{
						perror("read");
						exit(3);
						}	
								
					
					close(fd_pool_in);
					close(fd_pool_out);

					sprintf(buf_out + strlen(buf_out), "%s", buf);				
					}														
				}
			}
		else if (!strcmp(command, "show-active"))
			{
			sprintf(buf_out, "Active jobs:");		
			
			for (i = 1 ; i <= pools ; i++)
				{
				if (pools_info[i][1] == POOL_ACTIVE)
					{					
					if (getcwd(fullpath, sizeof(fullpath)) == NULL)
						{
						perror("getcwd");
						exit(5);
						}
					sprintf(fullpath, "%s/%s", fullpath, path);	
						
					sprintf(pool_in, "%s/pool%d_in", fullpath, i);
					sprintf(pool_out, "%s/pool%d_out", fullpath, i);
					
					if ((fd_pool_in = open(pool_in , O_WRONLY)) < 0)
						{ 
						perror("open"); 
						exit(1);
						}
						
					if (write(fd_pool_in,  buf_copy, sizeof(buf_copy) + 1) == -1)
						{
						perror("write");
						exit(2);
						}
							
					if ((fd_pool_out = open(pool_out , O_RDONLY)) < 0)
						{ 
						perror("open"); 
						exit(1);
						}	

					if (read(fd_pool_out,  buf, sizeof(buf) + 1) == -1)
						{
						perror("read");
						exit(3);
						}	
													
					close(fd_pool_in);
					close(fd_pool_out);
					
					sprintf(buf_out + strlen(buf_out), "\n%s", buf);
					}														
				}	
			}		
		else if (!strcmp(command, "show-finished"))
			{
			sprintf(buf_out, "Finished jobs:");		
			
			for (i = 1 ; i <= pools ; i++)
				{
				if (pools_info[i][1] == POOL_ACTIVE)
					{						
					if (getcwd(fullpath, sizeof(fullpath)) == NULL)
						{
						perror("getcwd");
						exit(5);
						}
					sprintf(fullpath, "%s/%s", fullpath, path);	
						
					sprintf(pool_in, "%s/pool%d_in", fullpath, i);
					sprintf(pool_out, "%s/pool%d_out", fullpath, i);
					
					if ((fd_pool_in = open(pool_in , O_WRONLY)) < 0)
						{ 
						perror("open"); 
						exit(1);
						}
						
					if (write(fd_pool_in,  buf_copy, sizeof(buf_copy) + 1) == -1)
						{
						perror("write");
						exit(2);
						}
							
					if ((fd_pool_out = open(pool_out , O_RDONLY)) < 0)
						{ 
						perror("open"); 
						exit(1);
						}	

					if (read(fd_pool_out,  buf, sizeof(buf) + 1) == -1)
						{
						perror("read");
						exit(3);
						}	
								
					
					close(fd_pool_in);
					close(fd_pool_out);
					
					sprintf(buf_out + strlen(buf_out) + 1, "\n%s", buf);
					}
				else
					{
					for (j = 1 ; j <= jobs_max ; j++)
						{
						sprintf(buf_out + strlen(buf_out), "\nJobID %d", (i-1)*jobs_max + j);
						}
					}
				}		
			}
		else if (!strcmp(command, "show-pools"))
			{
			sprintf(buf_out, "Pool & NumOfJobs:");		
			
			for (i = 1 ; i <= pools ; i++)
				{
				if (pools_info[i][1] == POOL_ACTIVE)
					{						
					if (getcwd(fullpath, sizeof(fullpath)) == NULL)
						{
						perror("getcwd");
						exit(5);
						}
					sprintf(fullpath, "%s/%s", fullpath, path);	
						
					sprintf(pool_in, "%s/pool%d_in", fullpath, i);
					sprintf(pool_out, "%s/pool%d_out", fullpath, i);
					
					if ((fd_pool_in = open(pool_in , O_WRONLY)) < 0)
						{ 
						perror("open"); 
						exit(1);
						}
						
					if (write(fd_pool_in,  buf_copy, sizeof(buf_copy) + 1) == -1)
						{
						perror("write");
						exit(2);
						}
							
					if ((fd_pool_out = open(pool_out , O_RDONLY)) < 0)
						{ 
						perror("open"); 
						exit(1);
						}	
					
					if (read(fd_pool_out,  buf, sizeof(buf) + 1) == -1)
						{
						perror("read");
						exit(3);
						}	
													
					close(fd_pool_in);
					close(fd_pool_out);
					
					sprintf(buf_out + strlen(buf_out), "\n%s", buf);
					}														
				}		
			}
		else if (!strcmp(command, "suspend"))
			{
			command = strtok(NULL," \t\n\r");	
			
			jobid = atoi(command);
			
			if (jobid % jobs_max == 0)
				poolid = jobid / jobs_max;
			else
				poolid = jobid / jobs_max + 1;
				
			
			if (pools_info[poolid][1] == POOL_FINISHED)
				{
				sprintf(buf_out, "JobID %d has finished\n", jobid);			
				}				
			else
				{					
				if (getcwd(fullpath, sizeof(fullpath)) == NULL)
					{
					perror("getcwd");
					exit(5);
					}
				sprintf(fullpath, "%s/%s", fullpath, path);	
					
					
				sprintf(pool_in, "%s/pool%d_in", fullpath, poolid);
				sprintf(pool_out, "%s/pool%d_out", fullpath, poolid);	
					
				if ((fd_pool_in = open(pool_in , O_WRONLY)) < 0)
					{ 
					perror("open"); 
					exit(1);
					}
			
				if (write(fd_pool_in,  buf_copy, sizeof(buf_copy) + 1) == -1)
					{
					perror("write");
					exit(2);
					}	
				
				if ((fd_pool_out = open(pool_out , O_RDONLY)) < 0)
					{ 
					perror("open"); 
					exit(1);
					}					
								
				if (read(fd_pool_out,  buf, sizeof(buf) + 1) == -1)
					{
					perror("read");
					exit(3);
					}	
				
				close(fd_pool_in);
				close(fd_pool_out);
				
				sprintf(buf_out, "%s", buf);					
				}	
			}
		else if (!strcmp(command, "resume"))
			{
			command = strtok(NULL," \t\n\r");	
			
			jobid = atoi(command);
			
			if (jobid % jobs_max == 0)
				poolid = jobid / jobs_max;
			else
				poolid = jobid / jobs_max + 1;
			
			if (pools_info[poolid][1] == POOL_FINISHED)
				{
				sprintf(buf_out, "JobID %d has finished\n", jobid);			
				}				
			else
				{					
				if (getcwd(fullpath, sizeof(fullpath)) == NULL)
					{
					perror("getcwd");
					exit(5);
					}
				sprintf(fullpath, "%s/%s", fullpath, path);	
					
					
				sprintf(pool_in, "%s/pool%d_in", fullpath, poolid);
				sprintf(pool_out, "%s/pool%d_out", fullpath, poolid);	
					
				if ((fd_pool_in = open(pool_in , O_WRONLY)) < 0)
					{ 
					perror("open"); 
					exit(1);
					}
			
				if (write(fd_pool_in,  buf_copy, sizeof(buf_copy) + 1) == -1)
					{
					perror("write");
					exit(2);
					}	
				
				if ((fd_pool_out = open(pool_out , O_RDONLY)) < 0)
					{ 
					perror("open"); 
					exit(1);
					}					
								
				if (read(fd_pool_out,  buf, sizeof(buf) + 1) == -1)
					{
					perror("read");
					exit(3);
					}	
				
				close(fd_pool_in);
				close(fd_pool_out);
				
				sprintf(buf_out, "%s", buf);					
				}	
			}
		else if (!strcmp(command, "shutdown"))
			{
			for (i = 1 ; i <= pools ; i++)
				{
				if (pools_info[i][1] == POOL_ACTIVE)
					{
					if (kill(pools_info[i][0], SIGTERM) == -1)
						{
						perror("kill");
						exit(9);	
						}					
					}	
					
				if (getcwd(fullpath, sizeof(fullpath)) == NULL)
						{
						perror("getcwd");
						exit(5);
						}
				sprintf(fullpath, "%s/%s", fullpath, path);	
				
				
				sprintf(pool_in, "%s/pool%d_in", fullpath, i);
				sprintf(pool_out, "%s/pool%d_out", fullpath, i);	
					
					
				if (unlink(pool_in) == -1)
					{
					perror("unlink");
					exit(8);
					}
			
				if (unlink(pool_out) == -1)
					{
					perror("unlink");
					exit(8);
					}	
				}	
	
			close(fd_jms_in);
			close(fd_jms_out);
			
			for (i = 0 ; i <= pools ; i++)
				free(pools_info[i]);
			free(pools_info);
			
			exit(0);
			}
		else
			sprintf(buf_out, "Wrong command! Please try again...\n");	
		
		if (write(fd_jms_out,  buf_out, sizeof(buf_out) + 1) == -1)
			{
			perror("write");
			exit(2);
			}	
		}
	}