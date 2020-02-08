/* readlink wrapper to return the link name in malloc'd storage.
   Unlike xreadlink and xreadlink_with_size, don't ever call exit.

   Copyright (C) 2001, 2003-2007 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Jim Meyering <jim@meyering.net>  */

#include "areadlink.h"

#ifndef SIZE_MAX
# define SIZE_MAX ((size_t) - 1)
#endif

#ifndef SSIZE_MAX
# define SSIZE_MAX ((ssize_t)(SIZE_MAX / 2))
#endif

#ifndef SYMLINK_MAX
# define SYMLINK_MAX 1024
#endif

#define MAXSIZE (SIZE_MAX < SSIZE_MAX ? SIZE_MAX : SSIZE_MAX)

char *
areadlink_with_size(char const *file, size_t size)
{
    size_t symlink_max = SYMLINK_MAX;
    size_t INITIAL_LIMIT_BOUND = 8 * 1024;
    size_t initial_limit = (symlink_max < INITIAL_LIMIT_BOUND
                    ? symlink_max + 1
                    : INITIAL_LIMIT_BOUND);

    //为链接名字初始化buffer size
    size_t buf_size = size < initial_limit ? size + 1 : initial_limit;

    while (1)
    {
        ssize_t r;
        size_t link_length;
        char *buffer = malloc (buf_size);

        if (buffer == NULL)
        return NULL;
        r = readlink (file, buffer, buf_size);
        link_length = r;

        /* On AIX 5L v5.3 and HP-UX 11i v2 04/09, readlink returns -1
        with errno == ERANGE if the buffer is too small.  */
        if (r < 0 && errno != ERANGE)
        {
            int saved_errno = errno;
            free (buffer);
            errno = saved_errno;
            return NULL;
        }

        //readlink()会将参数path的符号链接内容存储到参数buf所指的内存空间，返回的内容不是以\000作字符串结尾，
        //但会将字符串的字符数返回，这使得添加\000变得简单。若参数bufsiz小于符号连接的内容长度，过长的内容会被截断，
        //如果 readlink 第一个参数指向一个文件而不是符号链接时，readlink 设 置errno 为 EINVAL 并返回 -1
        if (link_length < buf_size)
        {
            buffer[link_length] = 0;
            return buffer;
        }

        free (buffer);
        if (buf_size <= MAXSIZE / 2)
            buf_size *= 2;
        else if (buf_size < MAXSIZE)
            buf_size = MAXSIZE;
        else
        {
            errno = ENOMEM;
            return NULL;
        }
    }
    
}