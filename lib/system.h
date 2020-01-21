
#include <stdio.h>
# include <stdbool.h>
#include "progname.h"
#include "../config.h"


#define ST_NBLOCKSIZE 1024
enum
{
    NOT_AN_INODE_NUMBER = 0
};
// 转换成字符串常量
#define _(Msgid) ((const char*)Msgid)

#define ST_NBLOCKS(statbuf) \
    ((statbuf).st_size / ST_NBLOCKSIZE + ((statbuf).st_size % ST_NBLOCKSIZE != 0))

#if defined _WIN32 || defined __WIN32__ || defined __CYGWIN__ || defined __EMX__ || defined __DJGPP__
/* Win32, Cygwin, OS/2, DOS */
# define ISSLASH(C) ((C) == '/' || (C) == '\\')
#endif

#ifndef DIRECTORY_SEPARATOR
# define DIRECTORY_SEPARATOR '/'
#endif

#ifndef ISSLASH
# define ISSLASH(C) ((C) == DIRECTORY_SEPARATOR)
#endif

//使用static修饰符，函数仅在文件内部可见
//inline函数，告诉编译器把函数代码在编译时直接拷贝到程序中
//在头文件定义函数又想避免冲突使用static inline
static inline void emit_ancillary_info (void)
{
    printf(_("\nReport '%s' bugs to %s\n"), program_name, PACKAGE_BUGREPORT);
    printf(_("home page: <https://github.com/keminar/vls>\n"), PACKAGE_NAME);
    fputs(_("Licence GNU software\n"), stdout);
}

static inline bool
dot_or_dotdot (char const *file_name)
{
    if (file_name[0] == '.')
    {
        char sep = file_name[(file_name[1] == '.') + 1];
        return (! sep || ISSLASH (sep));
    }
    else
        return false;
}