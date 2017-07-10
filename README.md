## Overview


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
