#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

char *areadlink_with_size(char const *filename, size_t size_hint);