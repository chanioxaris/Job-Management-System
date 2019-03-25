## Overview

A job management system implemented for unix systems. A coordinator handles the jobs submitted from user, using job pools. Each pool can handle up to a number of jobs, defined as a command line parameter. 
<br />
Each job redirects his standard output and standard error to two different files, which are stored under a directory named after the string that includes the Job ID, Process pid, Date and Time. Also, a custom signal handler implemented to ensure a smooth shutdown of the application. More details included below.

<br />
The below image describes the structure of the application.
<br />

![Jms](https://github.com/chanioxaris/job-manager/blob/master/img/jms.png)



### Console

The console is the user interface and handles the communication him and the coordinator using a pair of two [named pipes](https://en.wikipedia.org/wiki/Named_pipe). Also prints the results of operation submitted from user, to inform about the status of pools and jobs.

### Coord

The coordinator creates and handles the operation of pools and jobs. The communication with pools, are been established using a pair of [named pipes](https://en.wikipedia.org/wiki/Named_pipe) for each pool process. Pools are created dynamically, so it can handle any number of submitted jobs.

 
### Custom signal handler

A custom signal handler implemented using [sigaction](https://en.wikipedia.org/wiki/Sigaction), to catch and manage the [SIGTERM signal](https://en.wikipedia.org/wiki/Signal_(IPC)). It is highly recommended to use a custom signal handler, so there are no [zombie processes](https://en.wikipedia.org/wiki/Zombie_process) remaining after the shutdown of the application. If a pool receives a [SIGTERM signal](https://en.wikipedia.org/wiki/Signal_(IPC)), then redirects it to any active or suspended processes, waits for all of them to exit and then terminates his operation. All other signals are ignored and the default operation takes place.



## Interface description

A user can perform the following operations:
- #### submit [Job]
Submit a new job (e.g "ls -l", "pwd", "sleep 100", "cat" etc.).
- #### status [Job ID]
Print the status of job with requested ID.
- #### status-all
Print the status (active, suspended, finished) of every job.
- #### show-active
Print the active job's IDs.
- #### show-pools
Print the active pool linux pids, followed by the number of active jobs by pool.
- #### show-finished
Print the finished job's IDs.
- #### suspend [Job ID]
Suspend the execution of job with requested ID.
- #### resume [Job ID]
Resume the execution of job with requested ID.
- #### shutdown
Terminate the service and free the memory.


## Compile

`./makefile`

## Usage

`./jms_coord -l [directory name] -n [jobs per pool] -w [fifo name (out)] -r [fifo name (in)]`

`./jms_console  -w [fifo name (in)] -r [fifo name (out)]`
