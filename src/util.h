#pragma once

#include <cstdio>
#include <cstdlib>

// Prints msg to stderr and exits.
[[noreturn]] inline void Fatal(const char* msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}