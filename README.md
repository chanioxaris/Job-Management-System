## Overview

A job management system implemented for unix systems. A coordinator handles the jobs submitted from user, using job pools. Each pool can handle up to a number of jobs, defined as a command line parameter. 
<br />
Each job redirects his standard output and standard error to two different files, which are stored under a directory named after the string that includes the Job ID, Process pid, Date and Time. Also, a custom signal handler implemented to ensure a smooth shutdown of the application. More details included below.

<br />
The below image describes the structure of the application.
<br />

![Jms](https://github.com/chanioxaris/Job-Management-System/blob/master/img/jms.png)



### Console

### Coord
 
### Custom signal handler


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
