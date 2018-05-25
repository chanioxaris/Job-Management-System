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


void main(int argc, char* argv[]) {
	int i, pool_id = atoi(argv[1]), pool_jobs_cur = 0, jobs_max = atoi(argv[2]), pool_jobs_remain = jobs_max;
	int jobs_info[jobs_max+1][3];
	char buf_pool[512], buf_pool_copy[512], buf_pool_out[512];
	char fullpath[512], path[20], pool_in[512], pool_out[512];
	char *command_pool;
	int fd_job_in, fd_job_out, fd_pool_in, fd_pool_out;

	pid_t pid_job, pid_child;
	int	stat;

	//Custom Signal handler for SIGTERM
	static struct sigaction pool_handler;

	pool_handler.sa_handler = catch_sig;
	sigfillset(&(pool_handler.sa_mask));
	sigaction(SIGTERM, &pool_handler, NULL);

	strcpy(path, argv[3]);

	if (getcwd(fullpath, sizeof(fullpath)) == NULL) {
		perror("getcwd");
		exit(5);
	}

	sprintf(fullpath, "%s/%s", fullpath, path);

	sprintf(pool_in, "%s/pool%d_in", fullpath, pool_id);
	sprintf(pool_out, "%s/pool%d_out", fullpath, pool_id);

	//Open fifos for communication with coord
	if ((fd_job_in = open(pool_in , O_RDONLY)) < 0) {
		perror("open");
		exit(1);
	}

	if ((fd_job_out = open(pool_out , O_WRONLY)) < 0) {
		perror("open");
		exit(1);
	}

	while(1) {
		//Wait for any child process (ex any job of this pool to finish)
		pid_child = waitpid(-1, &stat, WNOHANG);

		if (pid_child > 0) {
			for (i = 1 ; i <= pool_jobs_cur ; i++) {
				if (jobs_info[i][0] == pid_child) {
					jobs_info[i][1] = JOB_FINISHED;
					pool_jobs_remain--;
					break;
				}
			}

			if (pool_jobs_remain == 0)
				break;
		}

		if (read(fd_job_in,  buf_pool, sizeof(buf_pool) + 1) == -1) {
			perror("read");
			exit(3);
		}

		strcpy(buf_pool_copy, buf_pool);
		command_pool = strtok(buf_pool," \t\n\r");

		//Check command on pool
		if (!strcmp(command_pool, "submit")) {
			pool_jobs_cur++;

			pid_job = fork();

			//Job process
			if (pid_job == 0) {
				int fd_out, fd_error;
				char job_dir[1024], fullpath_job[1024];

				sprintf(job_dir, "%s/%d_%d_%d_%d", path, (pool_id-1)*jobs_max + pool_jobs_cur, getpid(), format_date(), format_time());

				if (mkdir(job_dir, 0700) == -1) {
					perror("mkdir");
					exit(4);
				}

				//Create arguments for exec
				int counter = countWords(buf_pool_copy);
				char *args[counter];
				counter = 0;
				command_pool = strtok(NULL," \n");

				while (command_pool != NULL) {
					args[counter++] = command_pool;
					command_pool = strtok (NULL, " \n");
				}

				args[counter] = NULL;

				//Redirect stdout and stderror to files
				sprintf(fullpath_job, "%s/stdout", job_dir);
				if ((fd_out = open(fullpath_job , O_CREAT | O_WRONLY, 0700)) == -1) {
					perror("open");
					exit(1);
				}

				sprintf(fullpath_job, "%s/stderror", job_dir);
				if ((fd_error = open(fullpath_job , O_CREAT | O_WRONLY, 0700)) == -1) {
					perror("open");
					exit(1);
				}

				close(1);
				dup(fd_out);
				close(2);
				dup(fd_error);

				execvp(args[0], args);

				close(fd_out);
				close(fd_error);

				exit(0);
			}
			else if (pid_job > 0) {
				//Update data for job
				jobs_info[pool_jobs_cur][0] = pid_job;
				jobs_info[pool_jobs_cur][1] = JOB_ACTIVE;
				jobs_info[pool_jobs_cur][2] = format_time();

				sprintf(buf_pool_out, "%d", pid_job);
			}
			else {
				perror("fork");
				exit(7);
			}
		}
		else if (!strcmp(command_pool, "status")) {
			command_pool = strtok(NULL," \t\n\r");

			int jobid_pool = atoi(command_pool);

			if (jobid_pool % jobs_max == 0)
				jobid_pool = jobs_max;
			else
				jobid_pool = jobid_pool % jobs_max;


			if (jobs_info[jobid_pool][1] == JOB_ACTIVE)
				sprintf(buf_pool_out, "Active (running for %d seconds)", format_time() - jobs_info[jobid_pool][2]);

			else if (jobs_info[jobid_pool][1] == JOB_SUSPENDED)
				sprintf(buf_pool_out, "Suspended");

			else if (jobs_info[jobid_pool][1] == JOB_FINISHED)
				sprintf(buf_pool_out, "Finished");
			}
		else if (!strcmp(command_pool, "status-all")) {
			memset(buf_pool_out, 0, sizeof(buf_pool_out));

			for (i = 1 ; i <= pool_jobs_cur ; i++) {
				if (jobs_info[i][1] == JOB_ACTIVE)
					sprintf(buf_pool_out + strlen(buf_pool_out), "\nJobID %d Status: Active (running for %d seconds)", (pool_id-1)*jobs_max + i, format_time() - jobs_info[i][2]);
				else if (jobs_info[i][1] == JOB_SUSPENDED)
					sprintf(buf_pool_out + strlen(buf_pool_out), "\nJobID %d Status: Suspended", (pool_id-1)*jobs_max + i);

				else if (jobs_info[i][1] == JOB_FINISHED)
					sprintf(buf_pool_out + strlen(buf_pool_out), "\nJobID %d Status: Finished", (pool_id-1)*jobs_max + i);
			}
		}
		else if (!strcmp(command_pool, "show-active")) {
			memset(buf_pool_out, 0, sizeof(buf_pool_out));

			for (i = 1 ; i <= pool_jobs_cur ; i++) {
				if (jobs_info[i][1] == JOB_ACTIVE)
					sprintf(buf_pool_out + strlen(buf_pool_out), "\nJobID %d", (pool_id-1)*jobs_max + i);
			}
		}
		else if (!strcmp(command_pool, "show-finished")) {
			memset(buf_pool_out, 0, sizeof(buf_pool_out));

			for (i = 1 ; i <= pool_jobs_cur ; i++) {
				if (jobs_info[i][1] == JOB_FINISHED)
					sprintf(buf_pool_out + strlen(buf_pool_out), "\nJobID %d", (pool_id-1)*jobs_max + i);
			}
		}
		else if (!strcmp(command_pool, "show-pools")) {
			int counter = 0;

			for (i = 1 ; i <= pool_jobs_cur ; i++) {
				if (jobs_info[i][1] == JOB_ACTIVE || jobs_info[i][1] == JOB_SUSPENDED)
					counter++;
			}

			sprintf(buf_pool_out, "%d %d\n", getpid(), counter);
		}
		else if (!strcmp(command_pool, "suspend"))
		{
			command_pool = strtok(NULL," \t\n\r");

			int jobid_pool = atoi(command_pool);

			if (jobid_pool % jobs_max == 0)
				jobid_pool = jobs_max;
			else
				jobid_pool = jobid_pool % jobs_max;

			if (jobs_info[jobid_pool][1] == JOB_ACTIVE) {
				if (kill(jobs_info[jobid_pool][0], SIGSTOP) == -1) {
					perror("kill");
					exit(9);
				}

				sprintf(buf_pool_out, "Sent suspend signal to JobID %d\n", jobid_pool);
				jobs_info[jobid_pool][1] = JOB_SUSPENDED;
			}
			else if (jobs_info[jobid_pool][1] == JOB_SUSPENDED)
				sprintf(buf_pool_out, "JobID %d is already suspended\n", jobid_pool);
			else if (jobs_info[jobid_pool][1] == JOB_FINISHED)
				sprintf(buf_pool_out, "JobID %d has finished\n", jobid_pool);
		}
		else if (!strcmp(command_pool, "resume")) {
			command_pool = strtok(NULL," \t\n\r");

			int jobid_pool = atoi(command_pool);

			if (jobid_pool % jobs_max == 0)
				jobid_pool = jobs_max;
			else
				jobid_pool = jobid_pool % jobs_max;

			if (jobs_info[jobid_pool][1] == JOB_ACTIVE)
				sprintf(buf_pool_out, "JobID %d is already running\n", jobid_pool);
			else if (jobs_info[jobid_pool][1] == JOB_SUSPENDED) {
				if (kill(jobs_info[jobid_pool][0], SIGCONT) == -1) {
					perror("kill");
					exit(9);
				}

				sprintf(buf_pool_out, "Sent resume signal to JobID %d\n", jobid_pool);
				jobs_info[jobid_pool][1] = JOB_ACTIVE;
			}
			else if (jobs_info[jobid_pool][1] == JOB_FINISHED)
				sprintf(buf_pool_out, "JobID %d has finished\n", jobid_pool);
		}
		else if (!strcmp(command_pool, "shutdown"))
			break;

		//Send data back to coord
		if (write(fd_job_out,  buf_pool_out, sizeof(buf_pool_out) + 1) == -1) {
			perror("write");
			exit(2);
		}

		sleep(1);
	}

	close(fd_pool_in);
	close(fd_pool_out);

	exit(0);
}
