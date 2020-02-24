
#include <stdio.h>
# include <stdbool.h>
#include <string.h>
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


#define STREQ(a, b) (strcmp (a, b) == 0)

#ifndef MAX
# define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
# define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#include "intprops.h"

#ifndef SSIZE_MAX
# define SSIZE_MAX TYPE_MAXIMUM (ssize_t)
#endif

#ifndef OFF_T_MIN
# define OFF_T_MIN TYPE_MINIMUM (off_t)
#endif

#ifndef OFF_T_MAX
# define OFF_T_MAX TYPE_MAXIMUM (off_t)
#endif

#ifndef UID_T_MAX
# define UID_T_MAX TYPE_MAXIMUM (uid_t)
#endif

#ifndef GID_T_MAX
# define GID_T_MAX TYPE_MAXIMUM (gid_t)
#endif

#ifndef PID_T_MAX
# define PID_T_MAX TYPE_MAXIMUM (pid_t)
#endif

#define CTLESC '\001'
#define CTLNUL '\177'

#if defined strdupa
# define ASSIGN_STRDUPA(DEST, S)		\
  do { DEST = strdupa (S); } while (0)
#else
# define ASSIGN_STRDUPA(DEST, S)		\
  do						\
    {						\
      const char *s_ = (S);			\
      size_t len_ = strlen (s_) + 1;		\
      char *tmp_dest_ = alloca (len_);		\
      DEST = memcpy (tmp_dest_, s_, len_);	\
    }						\
  while (0)
#endif

//使用static修饰符，函数仅在文件内部可见
//inline函数，告诉编译器把函数代码在编译时直接拷贝到程序中
//在头文件定义函数又想避免冲突使用static inline
static inline void emit_ancillary_info (void)
{
    printf(_("\nReport '%s' bugs to %s\n"), program_name, PACKAGE_BUGREPORT);
    printf(_("home page: <https://github.com/keminar/vls> %s\n"), PACKAGE_NAME);
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
