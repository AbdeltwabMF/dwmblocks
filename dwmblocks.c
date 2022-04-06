// Copyright 2020 torrinfail
// Copyright 2022 Abd El-Twab M. Fakhry

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
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
void dummysignalhandler(int num);
#endif
void sighandler(int signum, siginfo_t *si, void *ucontext);
void buttonhandler(int sig, siginfo_t *si, void *ucontext);
void getcmds(int time);
void getsigcmds(unsigned int signal);
void setupsignals();
void chldhandler();
void statusloop();
void termhandler();
void pstdout();
int getstatus(char *str, char *last);
#ifndef NO_X
void setroot();
static void (*writestatus)() = setroot;
static int setupX();
static Display *dpy;
static int screen;
static Window root;
#else
static void (*writestatus)() = pstdout;
#endif

#include "blocks.h"

static char status_bar[LENGTH(blocks)][CMDLENGTH] = {0};
static char status_str[2][STATUSLENGTH];
static char button[] = "\0";
static int status_continue = 1;

// opens process *cmd and stores output in *output
void getcmd(const Block *block, char *output) {
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
  FILE *commandf;
  if (*button) {
    setenv("BUTTON", button, 1);
    commandf = popen(block->command, "r");
    *button = '\0';
    unsetenv("BUTTON");
  } else {
    commandf = popen(block->command, "r");
  }

  if (!commandf)
    return;

  fgets(output + icon_size, CMDLENGTH - icon_size - delim_len, commandf);

  // return if the getcmd output is empty
  int output_size = strlen(output);
  if (output_size == 0) {
    pclose(commandf);
    return;
  }

  // only chop off newline if one is present at the end
  int last = output[output_size - 1] == '\n' ? output_size - 1 : output_size;
  if (delim[0] != '\0') {
    strncpy(output + last, delim, delim_len);
  } else {
    output[last] = '\0';
  }
  pclose(commandf);
}

void getcmds(int time) {
  const Block *current;
  for (unsigned int i = 0; i < LENGTH(blocks); ++i) {
    current = blocks + i;
    if ((current->interval != 0 && time % current->interval == 0) || time == -1)
      getcmd(current, status_bar[i]);
  }
}

void getsigcmds(unsigned int signal) {
  const Block *current;
  for (unsigned int i = 0; i < LENGTH(blocks); ++i) {
    current = blocks + i;
    if (current->signal == signal)
      getcmd(current, status_bar[i]);
  }
}

void setupsignals() {
	struct sigaction sa = { .sa_sigaction = sighandler, .sa_flags = SA_SIGINFO };
#ifndef __OpenBSD__
  /* initialize all real time signals with dummy handler */
  for (int i = SIGRTMIN; i <= SIGRTMAX; ++i) {
    signal(i, dummysignalhandler);
		sigaddset(&sa.sa_mask, i);
	}
#endif

  for (unsigned int i = 0; i < LENGTH(blocks); ++i) {
    if (blocks[i].signal > 0)
			sigaction(SIGMINUS+blocks[i].signal, &sa, NULL);
  }
}

int getstatus(char *str, char *last) {
  strcpy(last, str);
  str[0] = '\0';
  for (unsigned int i = 0; i < LENGTH(blocks); ++i)
    strcat(str, status_bar[i]);
  str[strlen(str) - strlen(delim)] = '\0';
  return strcmp(str, last); // 0 if they are the same
}

#ifndef NO_X
void setroot() {
  if (!getstatus(status_str[0], status_str[1]))
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
  if (!getstatus(status_str[0], status_str[1]))
    return;
  printf("%s\n", status_str[0]);
  fflush(stdout);
}

void statusloop() {
  setupsignals();
  int i = 0;
  getcmds(-1);
  while (1) {
    getcmds(i++);
    writestatus();
    if (!status_continue)
      break;
    sleep(1.0);
  }
}

#ifndef __OpenBSD__
/* this signal handler should do nothing */
void dummysignalhandler(int signum) { return; }

void buttonhandler(int sig, siginfo_t *si, void *ucontext) {
  *button = ('0' + si->si_value.sival_int) & 0xff;
  getsigcmds(si->si_value.sival_int >> 8);
  writestatus();
}
#endif

void sighandler(int signum, siginfo_t *si, void *ucontext) {
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
		getsigcmds(signum-SIGPLUS);
		writestatus();
	}
}

void termhandler() { status_continue = 0; }

void chldhandler()
{
	while (0 < waitpid(-1, NULL, WNOHANG));
}

int main(int argc, char **argv) {
  for (int i = 0; i < argc; i++) {
    if (!strcmp("-d", argv[i]))
      strncpy(delim, argv[++i], delim_len);
    else if (!strcmp("-p", argv[i]))
      writestatus = pstdout;
  }

#ifndef NO_X
  if (!setupX())
    return 1;
#endif

  delim_len = MIN(delim_len, strlen(delim));
  delim[delim_len++] = '\0';
  signal(SIGTERM, termhandler);
  signal(SIGINT, termhandler);
	signal(SIGCHLD, chldhandler);
  statusloop();

#ifndef NO_X
  XCloseDisplay(dpy);
#endif

  return 0;
}

