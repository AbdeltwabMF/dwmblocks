#ifndef BLOCKS_H
#define BLOCKS_H
/**
  Modify this file to change what commands output to your statusbar, and
recompile using the make command.
**/
static const Block blocks[] = {
    /*Icon*/ /*Command*/ /*Update Interval*/ /*Update Signal*/
    {"", "sb-newsfeed", 30, 15},
    {"", "sb-layout", 0, 9},
    {"🕌 ", "next-prayer --hybrid", 20, 14},
    {"", "sb-mail", 2, 1},
    {"", "sb-cpu", 1, 2},
    {"", "sb-memory", 2, 3},
    {"", "sb-home", 180, 4},
    {"", "sb-root", 60, 5},
    {"", "sb-internet", 5, 6},
    {"", "sb-volume", 0, 10},
    {"", "sb-brightness", 0, 11},
    {"", "sb-battery", 5, 7},
    {"", "sb-pacups", 30, 8},
    {"", "sb-date", 1, 12},
    {"", "sb-clock", 1, 13},
};

/**
  Sets delimiter between status commands. NULL character ('\0') means no
delimiter.
**/

static char *delim = " | ";
static unsigned int delim_len = 5;
#endif

