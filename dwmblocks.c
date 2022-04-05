// Copyright 2020 torrinfail
// Copyright 2022 Abd El-Twab M. Fakhry

/**
NOTES:
  - name convention:
    - all variables are lower case with underscores
    - all functions are lower case with underscores
    - all constants are UPPER CASE with underscores
  - Renamed functions
    - getsigcmds() -> signal_commands()
    - getcmds() -> commands()
    - getcmd() -> command()
    - sighandler() -> signal_handler()
    - chldhandler() -> child_handler()
    - dummysignalhandler() -> dummy_signal_handler()
    - statusloop() -> status_loop()
    - termhandler() -> terminate_handler()
    - writestatus() -> write_status()
**/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>

#ifndef NO_X
#include <X11/Xlib.h>
#endif

#ifdef __OpenBSD__
#define SIGPLUS SIGUSR1 + 1
#define SIGMINUS SIGUSR1 - 1
#else
#define SIGPLUS SIGRTMIN
#define SIGMINUS SIGRTMIN
#endif

#define LENGTH(X) (sizeof(X) / sizeof(X[0]))
#define CMDLENGTH 50
#define MIN(a, b) ((a < b) ? a : b)
#define STATUSLENGTH (LENGTH(blocks) * CMDLENGTH + 1)

typedef struct {
  char *icon;
  char *command;
  unsigned int interval;
  unsigned int signal;
} Block;

#ifndef __OpenBSD__
void dummy_signal_handler(int num);
#endif

void exec_commands(int time);
void signal_commands(unsigned int signal);
void setup_signals();
void signal_handler(int signum, siginfo_t *si, void *ucontext);
void button_handler(int sig, siginfo_t *si, void *ucontext);
void child_handler();
int get_status(char *str, char *last);
void status_loop();
void terminate_handler();
void pstdout();

#ifndef NO_X
void setroot();
static void (*write_status)() = setroot;
static int setupX();
static Display *dpy;
static int screen;
static Window root;
#else
static void (*write_status)() = pstdout;
#endif

#include "blocks.h"

static char status_bar[LENGTH(blocks)][CMDLENGTH] = {0};
static char status_str[2][STATUSLENGTH];
static char button[] = "\0";
static int status_continue = 1;

// opens process *cmd and stores output in *output
void exec_command(const Block *block, char *output) {
 	if (block->signal)
 		*output++ = block->signal;

  // copy icon to output
  int icon_size = strlen(block->icon);
  strcpy(output, block->icon);

  /* Upon successful completion, popen() shall return a pointer to an
     open stream that can be used to read or write to the pipe.
     Otherwise, it shall return a null pointer and may set errno
     to indicate the error.
  */
  FILE *command;
  if (*button) {
  	setenv("BUTTON", button, 1);
  	command = popen(block->command,"r");
  	*button = '\0';
  	unsetenv("BUTTON");
  } else {
  	command = popen(block->command,"r");
  }

  if (!command)
    return;

  fgets(output + icon_size, CMDLENGTH - icon_size - delim_len, command);

  // return if the command output is empty
  int output_size = strlen(output);
  if (output_size == strlen(block->icon)) {
    pclose(command);
    return;
  }

  // only chop off newline if one is present at the end
  int last = output[output_size - 1] == '\n' ? output_size - 1 : output_size;
  if (delim[0] != '\0') {
    strncpy(output + last, delim, delim_len);
  } else {
    output[last] = '\0';
  }
  pclose(command);
}

void exec_commands(int time) {
  const Block *current;
  for (unsigned int i = 0; i < LENGTH(blocks); ++i) {
    current = blocks + i;
    if ((current->interval != 0 && time % current->interval == 0) || time == -1)
      exec_command(current, status_bar[i]);
  }
}

void signal_commands(unsigned int signal) {
  const Block *current;
  for (unsigned int i = 0; i < LENGTH(blocks); ++i) {
    current = blocks + i;
    if (current->signal == signal)
      exec_command(current, status_bar[i]);
  }
}

void setup_signals() {
 	struct sigaction sa = { .sa_sigaction = signal_handler, .sa_flags = SA_SIGINFO };
#ifndef __OpenBSD__
  /* initialize all real time signals with dummy handler */
  for (int i = SIGRTMIN; i <= SIGRTMAX; ++i) {
    signal(i, dummy_signal_handler);
 		sigaddset(&sa.sa_mask, i);
 	}
#endif

  for (unsigned int i = 0; i < LENGTH(blocks); ++i) {
    if (blocks[i].signal > 0) {
 			sigaction(SIGMINUS + blocks[i].signal, &sa, NULL);
      // ignore signal when handling SIGUSR1
  		sigaddset(&sa.sa_mask, SIGRTMIN+blocks[i].signal);
  	}
    sa.sa_sigaction = button_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &sa, NULL);
  }
}

int get_status(char *str, char *last) {
  strcpy(last, str);
  str[0] = '\0';
  for (unsigned int i = 0; i < LENGTH(blocks); ++i)
    strcat(str, status_bar[i]);
  str[strlen(str) - strlen(delim)] = '\0';
  return strcmp(str, last);  // 0 if they are the same
}

#ifndef NO_X
void setroot() {
  if (!get_status(status_str[0], status_str[1]))
    return;
  XStoreName(dpy, root, status_str[0]);
  XFlush(dpy);
}

int setupX() {
  dpy = XOpenDisplay(NULL);
  if (!dpy) {
    fprintf(stderr, "dwmblocks: Failed to open display\n");
    return 0;
  }
  screen = DefaultScreen(dpy);
  root = RootWindow(dpy, screen);
  return 1;
}
#endif

void pstdout() {
  // Only write out if text has changed.
  if (!get_status(status_str[0], status_str[1]))
    return;
  printf("%s\n", status_str[0]);
  fflush(stdout);
}

void status_loop() {
  setup_signals();
  int i = 0;
  exec_commands(-1);
  while (1) {
    exec_commands(i++);
    write_status();
    if (!status_continue)
      break;
    sleep(1.0);
  }
}

#ifndef __OpenBSD__
/* this signal handler should do nothing */
void dummy_signal_handler(int signum) { return; }

void button_handler(int sig, siginfo_t *si, void *ucontext) {
  *button = ('0' + si->si_value.sival_int) & 0xff;
  signal_commands(si->si_value.sival_int >> 8);
  write_status();
}
#endif

void signal_handler(int signum, siginfo_t *si, void *ucontext) {
  if (si->si_value.sival_int) {
    pid_t parent = getpid();
    if (fork() == 0) {
#ifndef NO_X
      if (dpy)
        close(ConnectionNumber(dpy));
#endif
      int i;
      for (i = 0; i < LENGTH(blocks) && blocks[i].signal != signum-SIGRTMIN; i++);

      char shcmd[1024];
      sprintf(shcmd, "%s; kill -%d %d", blocks[i].command, SIGRTMIN+blocks[i].signal, parent);
      char *cmd[] = { "/bin/sh", "-c", shcmd, NULL };
      char button[2] = { '0' + si->si_value.sival_int, '\0' };
      setenv("BUTTON", button, 1);
      setsid();
      execvp(cmd[0], cmd);
      perror(cmd[0]);
      exit(EXIT_SUCCESS);
    }
	} else {
		signal_commands(signum-SIGPLUS);
		write_status();
	}
}

void terminate_handler() { status_continue = 0; }

void child_handler() { while (0 < waitpid(-1, NULL, WNOHANG)); }


int main(int argc, char **argv) {
  for (int i = 0; i < argc; i++) {
    if (!strcmp("-d", argv[i]))
      strncpy(delim, argv[++i], delim_len);
    else if (!strcmp("-p", argv[i]))
      write_status = pstdout;
  }

#ifndef NO_X
  if (!setupX())
    return 1;
#endif

  delim_len = MIN(delim_len, strlen(delim));
  delim[delim_len++] = '\0';
  signal(SIGTERM, terminate_handler);
  signal(SIGINT, terminate_handler);
	signal(SIGCHLD, child_handler);
  status_loop();

#ifndef NO_X
  XCloseDisplay(dpy);
#endif

  return 0;
}

