# Shell Lab
### _Kai Schuyler Gonzalez_

## Trace Progress
|             	|             	|             	|             	|
|-------------	|-------------	|-------------	|-------------	|
| [x] Trace01 	| [x] Trace02 	| [x] Trace03 	| [x] Trace04 	|
| [x] Trace05 	| [x] Trace06 	| [x] Trace07 	| [x] Trace08 	|
| [x] Trace09 	| [x] Trace10 	| [x] Trace11 	| [x] Trace12 	|
| [x] Trace13 	| [x] Trace14 	| [x] Trace15 	| [x] Trace16 	|

## Interview Questions
[Rubric](https://piazza.com/redirect/s3?bucket=uploads&prefix=attach%2Fk4xbw98e3pq67i%2FismoyuhqK5X%2Fk8orxp9lato0%2Fshelllabrubric.png)
### Fork / execve:
* Can explain entry / exit logic of fork:
    * A fork creates a new child that runs concurrently with it's parent process. The child has an identical but seperate address space. On a success, the PID of the child process is returned in the parent and 0 is returned in the child.
* Can explain args to exec and what happens if exec fails:
    * Exec replaces the current process with a new process.
    * `execve(argv[0], argv, environ)`
        * `argv[0]` is the program that is going to replace the current process
        * `argv` provides the new process it's argument array
        * `environ` provides the process's environment variable
    * `Execve` normally does not return. On a failure, it returns a -1 and returns to the calling program.
* Can explain process group purpose and implementations:
    * Used to control the distribution of a signal. When a signal is directed to a process group, the signal is delivered to each process that is a member of the group. 
    * `SIGINT` (signal interrupt), `SIGTSTP` (terminal stop) and `SIGQUIT` (quit) are all implementations of the distribution of signals to process groups.  
### Race condition:
* Can explain modifications to jobs and why they are needed:
    * In `do_bgfg()`:
    ```
    kill(-j->pid, SIGCONT); // kill job and send signal
    if(cmd == "bg") { // background
        j->state = BG; // change state to background
        printf("[%d] (%d) %s",j->jid, j->pid, j->cmdline); // print background process info
    }
    else { // foreground
        j->state = FG; // change state if foreground
        waitfg(j->pid); // wait for fg
    }
    ```
    * I change the state of job depending on bg or fg instruction. If a job is changed to background, I print its information. If changed to foreground, I have it wait until a signal is sent when the process is complete.
* Can identify race and fix for race:
    * A race is a synchronization error. It can happen inside a fork and can be a problem when you want a process to run only after a certain signal handler is run. For example, a shell can create a fg job and must wait for the job to terminate and be reaped by the SIGCHLD handles before accepting the next user command.  
    * Fix: Block signals and unblock them when you want the process to execute.
### SIGTSTP:
* Can explain code function:
    * If pid is nonzero, the handler function sends a signal to jobs.
* Can explain control flow when user presses Ctl-Z when using shell:
    * Causes kernel to send SIGTSTP signal to every process in the foreground progress group.
    * In default case, the result is to suspend the fg job.
### SIGCHLD
* Occurs when process terminates or stops. Kernel sends SIGCHLD signal to parent.
* Explain three cases of SIGCHLD(stopped, normal, signaled):
    * Stopped:
        * Changes state to ST and prints a message.
    * Normal:
        * Deletes job
    * Signaled:
        * prints message with signal number.
        * Deletes job.

* Can explain how child state is updated for each of Ctl-C: (Page 799)
    *  Modified SIGCHLD handler to reap as many zombie children as possible each time it is invoked.
        ```
        while((pid = waitpid(-1,&status, WNOHANG|WUNTRACED)) > 0)
        ```
    * Ctl-C makes the function `sigint_handler` execute. This function then sends a signal interrupt `SIGINT` that causes the following statement in `sigchld_handler` to execute:
        ```
        if(WIFSTOPPED(status)) { // Job stopped by signal 20
        job_t *j = getjobpid(jobs,pid); // job pointer
        j->state = ST; // change state

        printf("Job [%d] (%d) stopped by signal %d\n", pid2jid(pid), pid, WSTOPSIG(status)); // job stopped message
        }
        ```
        * Here, the child job's state is changed to ST.
* Can explain how child state is updated for each of Ctl-Z:
    * Ctl-Z makes the function `sigstp_handler` execute. This function then sends a signal interrupt `SIGSTOP` that causes the following statement in `sigchld_handler` to execute:
        ```
        else if(WIFSIGNALED(status)) { // Job terminated by signal 2
        printf("Job [%d] (%d) terminated by signal %d\n", pid2jid(pid), pid, WTERMSIG(status)); // job terminated due to signal message
        deletejob(jobs,pid);
        }
        ```
        * Here, the child's job is deleted.

## Additional Notes:
* When we type ctl-c/ctl-z, an interrupt signal `SIGINT` / `SIGSTP` is sent to the job in the foreground. If there is no job, nothing happens.
### STAT (process state) Legend:
**First Letter:**
* S: Sleeping
* T: stopped
* R: running
** Second Letter: **
* s: session leader
* +: foreground process group