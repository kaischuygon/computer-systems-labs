// 
// tsh - A tiny shell program with job control
// 
// Solution by Kai Schuyler Gonzalez, 109055731
//

using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string>

#include "globals.h"
#include "jobs.h"
#include "helper-routines.h"

//
// Needed global variable definitions
//

static char prompt[] = "tsh> ";
int verbose = 0;

//
// You need to implement the functions eval, builtin_cmd, do_bgfg,
// waitfg, sigchld_handler, sigstp_handler, sigint_handler
//
// The code below provides the "prototypes" for those functions
// so that earlier code can refer to them. You need to fill in the
// function bodies below.
// 

pid_t Fork(void);
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

//
// main - The shell's main routine 
//
int main(int argc, char *argv[]) 
{
  int emit_prompt = 1; // emit prompt (default)

  //
  // Redirect stderr to stdout (so that driver will get all output
  // on the pipe connected to stdout)
  //
  dup2(1, 2);

  /* Parse the command line */
  char c;
  while ((c = getopt(argc, argv, "hvp")) != EOF) {
    switch (c) {
    case 'h':             // print help message
      usage();
      break;
    case 'v':             // emit additional diagnostic info
      verbose = 1;
      break;
    case 'p':             // don't print a prompt
      emit_prompt = 0;  // handy for automatic testing
      break;
    default:
      usage();
    }
  }

  //
  // Install the signal handlers
  //

  //
  // These are the ones you will need to implement
  //
  Signal(SIGINT,  sigint_handler);   // ctrl-c
  Signal(SIGTSTP, sigtstp_handler);  // ctrl-z
  Signal(SIGCHLD, sigchld_handler);  // Terminated or stopped child

  //
  // This one provides a clean way to kill the shell
  //
  Signal(SIGQUIT, sigquit_handler); 

  //
  // Initialize the job list
  //
  initjobs(jobs);

  //
  // Execute the shell's read/eval loop
  //
  for(;;) {
    //
    // Read command line
    //
    if (emit_prompt) {
      printf("%s", prompt);
      fflush(stdout);
    }

    char cmdline[MAXLINE];

    if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin)) {
      app_error("fgets error");
    }
    //
    // End of file? (did user type ctrl-d?)
    //
    if (feof(stdin)) {
      fflush(stdout);
      exit(0);
    }

    //
    // Evaluate command line
    //
    eval(cmdline);
    fflush(stdout);
    fflush(stdout);
  } 

  exit(0); //control never reaches here
}


// Fork error-handling wrapper
pid_t Fork(void) { 
  pid_t pid;

  if ((pid = fork()) < 0) { // fork returns -1 if an error occurs
    unix_error("Fork error");
  }
  return pid;
}

// eval - Evaluate the command line that the user has just typed in
// 
// If the user has requested a built-in command (quit, jobs, bg or fg)
// then execute it immediately. Otherwise, fork a child process and
// run the job in the context of the child. If the job is running in
// the foreground, wait for it to terminate and then return.  Note:
// each child process must have a unique process group ID so that our
// background children don't receive SIGINT (SIGTSTP) from the kernel
// when we type ctrl-c (ctrl-z) at the keyboard.
void eval(char *cmdline)  {
  char *argv[MAXARGS];
  char buf[MAXLINE]; // holds modified command line
  strcpy(buf,cmdline); // copy command line to buf
  int bg = parseline(cmdline, argv);
  pid_t pid; // Process id 
  if (argv[0] == NULL) // ignore empty lines 
    return;
  // My additions:
  if(builtin_cmd(argv) == 0) { // if not a built in command
    sigset_t mask; // signal mask
    // Block signals
    sigemptyset(&mask); // clears all signals from set
    sigaddset(&mask, SIGCHLD); // adds signal to set
    sigprocmask(SIG_BLOCK, &mask, NULL); // block signals
    if((pid = Fork()) == 0) { // Create new child process
      sigprocmask(SIG_UNBLOCK, &mask, NULL); // unblock signal
      setpgid(0, 0); // from hints provided in pdf
      if(execve(argv[0], argv, environ) < 0) { // execve the command that was specified and check if command exists
        printf("%s: command not found.\n",argv[0]);
        exit(0);
      } 
    }
    // Parent process, pid is now equal to the child's PID
    if(!bg) { // run jobs in foreground
      addjob(jobs, pid, FG, cmdline);
      if(sigprocmask(SIG_UNBLOCK, &mask, NULL) < 0) // unblock signal
        unix_error("sigprocmask error");
      waitfg(pid); // wait for fg process to terminate
    }
    else { // run jobs in background
      addjob(jobs, pid, BG, cmdline);
      if(sigprocmask(SIG_UNBLOCK, &mask, NULL) < 0) // unblock signal
        unix_error("sigprocmask error");
      printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline); // prints background job that has been added
    }
  }
  return;
}

// builtin_cmd - If the user has typed a built-in command then execute
//    it immediately. The command name would be in argv[0] and
//    is a C string. We've cast this to a C++ string type to simplify
//    string comparisons; however, the do_bgfg routine will need 
//    to use the argv array as well to look for a job number.
int builtin_cmd(char **argv) {
  string cmd(argv[0]);
  if(cmd == "quit") // quit command
    exit(0);
  if(cmd == "jobs") { // jobs command
    listjobs(jobs);
    return 1;
  }
  if(cmd == "bg" || cmd == "fg") { // background / foreground command
    do_bgfg(argv);
    return 1;
  } 
  return 0;     /* not a builtin command */
}

// do_bgfg - Execute the builtin bg and fg commands
void do_bgfg(char **argv) {
  struct job_t *j = NULL; // job pointer
    
  // Ignore command if no argument
  if (argv[1] == 0) {
    printf("%s command requires PID or %%jobid argument\n", argv[0]);
    return;
  }
    
  // Parse the required PID or %JID arg
  pid_t pid = atoi(argv[1]);
  if (isdigit(argv[1][0])) { // if pid
    if (!(j = getjobpid(jobs, pid))) {
      printf("(%d): No such process\n", pid);
      return;
    }
  }
  else if (argv[1][0] == '%') { // if jid
    int jid = atoi(&argv[1][1]);
    if (!(j = getjobjid(jobs, jid))) {
      printf("%s: No such job\n", argv[1]);
      return;
    }
  }	    
  else { // error msg
    printf("%s: argument must be a PID or %%JID\n", argv[0]);
    return;
  }
  // My additions:
  string cmd(argv[0]); // command line string
  kill(-j->pid, SIGCONT); // send signal
  if(cmd == "bg") { // background
    j->state = BG; // change state to background
    printf("[%d] (%d) %s",j->jid, j->pid, j->cmdline); // print background process info
  }
  else { // foreground
    j->state = FG; // change state if foreground
    waitfg(j->pid); // wait for fg
  }
  return;
}

// waitfg - Block until process pid is no longer the foreground process
void waitfg(pid_t pid) {
  job_t *j = getjobpid(jobs, pid); // job pointer
  if(pid == 0) return;
  // wait for process pid to no longer be fg process
  if(j != NULL) {
    while(pid == fgpid(jobs)) {} // based on hint that was provided in the pdf, a busy loop that terminates when process changes state
  }
  return;
}

/////////////////////////////////////////////////////////////////////////////
//
// Signal handlers
//

// sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
//     a child job terminates (becomes a zombie), or stops because it
//     received a SIGSTOP or SIGTSTP signal. The handler reaps all
//     available zombie children, but doesn't wait for any other
//     currently running children to terminate.  
void sigchld_handler(int sig) {
  // printf("Caught SIGCHLD.\n"); // for checking to see if in sigchild handler
  int status;
  pid_t pid;
  while((pid = waitpid(-1,&status, WNOHANG|WUNTRACED)) > 0) { // wait for fg process signal
    // child is currently stopped
    if(WIFSTOPPED(status)) { // Job stopped by signal 20
      job_t *j = getjobpid(jobs,pid); // job pointer
      j->state = ST; // change state

      printf("Job [%d] (%d) stopped by signal %d\n", pid2jid(pid), pid, WSTOPSIG(status)); // job stopped message
    }
    // child terminated because of signal
    else if(WIFSIGNALED(status)) { // Job terminated by signal 2
      printf("Job [%d] (%d) terminated by signal %d\n", pid2jid(pid), pid, WTERMSIG(status)); // job terminated due to signal message
      deletejob(jobs,pid);
    }
    // child terminated normally
    else if(WIFEXITED(status))
      deletejob(jobs,pid);
  }
  return;
}

// sigint_handler - The kernel sends a SIGINT to the shell whenver the
//    user types ctrl-c at the keyboard.  Catch it and send it along
//    to the foreground job.
void sigint_handler(int sig) { // ctrl+c signal handler
  // printf("\nCaught SIGINT.\n"); // for checking to see if in sigint handlers
  pid_t pid = fgpid(jobs); // get pid of foreground job
  if(pid < 0)
    unix_error("sigint handler error");
  else if(pid != 0)
    kill(-pid,SIGINT); // send signal
  return;
}

// sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
//     the user types ctrl-z at the keyboard. Catch it and suspend the
//     foreground job by sending it a SIGTSTP.  
void sigtstp_handler(int sig) { // ctrl+z signal handler
  // printf("\nCaught SIGTSTP.\n"); // for checking to see if in sigtstp handler
  pid_t pid = fgpid(jobs);
  if(pid < 0)
    unix_error("sigstp handler error");
  else if(pid != 0)
    kill(-pid,SIGSTOP); // send signal
  return;
}
/*********************
 * End signal handlers
 *********************/
