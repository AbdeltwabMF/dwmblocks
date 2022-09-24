#ifndef BLOCKS_H
#define BLOCKS_H
/**
  Modify this file to change what commands output to your statusbar, and
recompile using the make command.
**/
static const Block blocks[] = {
    /*Icon*/ /*Command*/ /*Update Interval*/ /*Update Signal*/
    {"", "sb-record", 2, 16},
    {"", "sb-newsfeed", 180, 15},
    {"", "sb-layout", 0, 14},
    {"🕌 ", "next-prayer --hybrid", 29, 13},
    {"", "sb-mail", 2, 12},
    {"", "sb-cpu", 1, 11},
    {"", "sb-memory", 2, 10},
    {"", "sb-home", 2, 9},
    {"", "sb-root", 2, 8},
    {"", "sb-internet", 1, 7},
    {"", "sb-volume", 0, 6},
    {"", "sb-brightness", 0, 5},
    {"", "sb-battery", 2, 4},
    {"", "sb-pacups", 2, 3},
    {"", "sb-date", 2, 2},
    {"", "sb-clock", 1, 1},
};

/**
  Sets delimiter between status commands. NULL character ('\0') means no
delimiter.
**/

static char *delim = " | ";
static unsigned int delim_len = 5;
#endif

