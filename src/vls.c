#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <limits.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <error.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#include "../lib/progname.h"
#include "../lib/system.h"
#include "../lib/human.h"
#include "../lib/filemode.h"
#include "../lib/idcache.h"
#include "../lib/areadlink.h"

enum filetype
{
    unknown,
    fifo,
    chardev,
    directory,
    blockdev,
    normal,
    symbolic_link,
    sock,
    whiteout,
    arg_directory
};

static int exit_status;

//最大访问深度
static size_t depth_max_num;
//过期时间
static size_t expire_day;
static size_t expire_minute;
static bool expire_opt_set;
//最大访问数
static size_t list_max_num;
//休息时间
static size_t sleep_time;
//总大小
static uintmax_t total_size;
//脚本执行时间
static int now_time;

// 把文件copy到此目录
static char *target_directory;

/* True means to display author information.  */

static bool print_author;

//打印总大小
static bool print_size;

// 是否可清掉当前行
static bool can_clear_line;

//自动模式
#define FORMAT_A 0
//回车模式
#define FORMAT_R 1
//换行模式
#define FORMAT_N 2

#define FILE_STA_NONE 0
#define FILE_STA_REMOVE 1
#define FILE_STA_EXPIRE 2
#define FILE_STA_NORMAL 3

#define DU_UNIT = 1024

//打印模式
static int print_format;

// True 则删除过期文件
static bool remove_expire_file;

/* The number of chars per hardware tab stop.  Setting this to zero
   inhibits the use of TAB characters for separating columns.  -T */
static size_t tabsize;

//打印的文件数
static size_t print_nums;

enum format
{
    long_format,		/* -l and other options that imply -l */
    one_per_line,		/* -1 */
};

static enum format format;

// 解开命令行参数
static int decode_switches(int argc, char **argv);
void usage(int status);
void usage_zh(int status);
static inline void emit_ancillary_info (void);
static void gobble_file(char const *name, enum filetype type,
                            ino_t inode, bool command_line_arg,
                            char const *dirname, char const *backup_dir, int depth);
static void attach(char *dest, const char *dirname, const char *name);                            
static void print_dir(const char *absolute_name, char const *backup_dir, int depth);
static void print_current_file (const char *absolute_name, struct stat st, char const *backup_name);

static void print_long_format(const char *absolute_name, struct stat st, char const *backup_name);
static void print_one_per_line(const char *absolute_name, struct stat st, char const *backup_name);
static int judge_dir_file(const char *absolute_name, struct stat st, char const *backup_name);

static char *
get_link_name (char const *filename, size_t size);

char *
human_readable (off_t n, char *buf);
static void
format_user (uid_t u, int width, bool stat_ok);
static void
format_group (gid_t g, int width, bool stat_ok);
static void
format_user_or_group (char const *name, unsigned long int id, int width);

//最后一级目录或文件，可能有反斜线
char *
last_component (char const *name);
//不计算反斜线的文件名长度
size_t
base_len (char const *name);
// 去掉尾部的反斜线
bool
strip_trailing_slashes (char *file);
// 取上级目录
char *mdir_name( char const *file);
static void mkdir_all(const char *dirs, struct stat src_st);

enum {
    LS_MINOR_PROBLEM = 1,
    LS_FAILURE = 2
};
enum {
    GETOPT_HELP_CHAR = CHAR_MAX - 2,
    GETOPT_VERSION_CHAR = CHAR_MAX - 3,
    AUTHOR_OPTION = CHAR_MAX + 1,
    BACKUP_OPTION,
    EXPIRE_DAY_OPTION,
    EXPIRE_MIN_OPTION,
    REMOVE_OPTION,
    SLEEP_OPTION
};

// 获取路径中的文件名
#define ASSIGN_BASENAME_STRDUPA(Dest, File_name)	\
  do							\
    {							\
      char *tmp_abns_;					\
      ASSIGN_STRDUPA (tmp_abns_, (File_name));		\
      Dest = last_component (tmp_abns_);		\
      strip_trailing_slashes (Dest);			\
    }							\
  while (0)


#define GETOPT_HELP_OPTION_DECL \
    "help", no_argument, NULL, GETOPT_HELP_CHAR

#define GETOPT_VERSION_OPTION_DECL \
    "version", no_argument, NULL, GETOPT_VERSION_CHAR

//参考链接：https://blog.csdn.net/qq_33850438/article/details/80172275
//no_argument(或者是0)时 ——参数后面不跟参数值，eg: --version,--help
//required_argument(或者是1)时 ——参数输入格式为：--参数 值 或者 --参数=值。eg:--dir=/home
//optional_argument(或者是2)时  ——参数输入格式只能为：--参数=值
static struct option const long_options[] = 
{
    {"backup-to", required_argument, NULL, BACKUP_OPTION},
    {"depth", required_argument, NULL, 'd'},
    {"expire-day", required_argument, NULL, EXPIRE_DAY_OPTION},
    {"expire-min", required_argument, NULL, EXPIRE_MIN_OPTION},
    {"num", required_argument, NULL, 'n'},
    {"remove", no_argument, NULL, REMOVE_OPTION},
    {"size", no_argument, NULL, 's'},
    {"sleep", required_argument, NULL, SLEEP_OPTION},
    {"author", no_argument, NULL, AUTHOR_OPTION},
    {GETOPT_HELP_OPTION_DECL},
    {GETOPT_VERSION_OPTION_DECL},
    {NULL, 0, NULL, 0}
};


char *
last_component (char const *name)
{
  char const *base = name;
  char const *p;
  bool saw_slash = false;

  while (ISSLASH (*base))
    base++;

  for (p = base; *p; p++)
    {
      if (ISSLASH (*p))
	saw_slash = true;
      else if (saw_slash)
	{
	  base = p;
	  saw_slash = false;
	}
    }

  return (char *) base;
}


size_t
base_len (char const *name)
{
  size_t len;

  for (len = strlen (name);  1 < len && ISSLASH (name[len - 1]);  len--)
    continue;

  return len;
}


bool
strip_trailing_slashes (char *file)
{
  char *base = last_component (file);
  char *base_lim;
  bool had_slash;

  /* last_component returns "" for file system roots, but we need to turn
     `///' into `/'.  */
  if (! *base)
    base = file;
  base_lim = base + base_len (base);
  had_slash = (*base_lim != '\0');
  *base_lim = '\0';
  return had_slash;
}

char *mdir_name( char const *file)
{
    size_t length;
    for (length = last_component(file) - file; length > 0 ; length--) {
        if (!ISSLASH(file[length - 1])) {
            break;
        }
    }
    bool append_dot = (length == 0);
    char *dir = malloc(length + append_dot + 1);
    if (!dir) {
        return NULL;
    }
    memcpy(dir, file, length);
    if (append_dot) {
        dir[length++] = '.';
    }
    dir[length] = '\0';
    return dir;
}

int main(int argc, char **argv)
{
    int i;
    int n_files;

    expire_opt_set = false;
    print_nums = 0;
    now_time = time(NULL);
    target_directory = NULL;
    print_format = FORMAT_N;
    can_clear_line = false;

    setbuf(stdout, NULL);
    set_program_name(argv[0]);
    i = decode_switches(argc, argv);
    //printf("i=%d\r\n", i);

    //atexit(close_stdout);
    exit_status = EXIT_SUCCESS;

    // 相减是传的文件数
    n_files = argc - i;
    if (n_files <= 0) {
        // 没传入目录
        gobble_file(".", directory, NOT_AN_INODE_NUMBER, true, "", target_directory, 0);
    } else {
        // 有传入目录
        do {
            gobble_file(argv[i++], unknown, NOT_AN_INODE_NUMBER, true, "", target_directory, 0);
        } while (i < argc);
    }

    if (print_size) {
        if (print_format == FORMAT_R) {
            printf("\r\033[K");
        } else if (print_format == FORMAT_A) {
            if (can_clear_line == true) {
                printf("\r\033[K");
            }
        }

        char hbuf[LONGEST_HUMAN_READABLE + 1];
        char const *size = human_readable ((off_t)total_size, hbuf);
        printf("total %s\n", size);
    } else if (print_format == FORMAT_R) {
        printf("\r\033[K");
    } else if (print_format == FORMAT_A) {
        if (can_clear_line == true) {
            printf("\r\033[K");
        }
    }
    exit(exit_status);
}

static void
gobble_file(char const *name, enum filetype type, ino_t inode, 
            bool command_line_arg, char const *dirname, char const *backup_dir, int depth)
{
    // 如果command_line_arg 为false或者inode 为0则正常
    assert (! command_line_arg || inode == NOT_AN_INODE_NUMBER);
    //printf("name=%s\n", name);
    
    char *absolute_name;
    int err;
    char *backup_name = NULL;
    // 全路径或是顶级路径
    if (name[0] == '/' || dirname[0] == 0)
        absolute_name = (char *) name;
    else
    {
        //alloca是在栈(stack)上申请空间,该变量离开其作用域之后被自动释放，无需手动调用释放函数。
        //strlen长度不包括尾部的\0所以要+2
        absolute_name = alloca(strlen(name) + strlen(dirname) + 2);
        attach(absolute_name, dirname, name);
    }
    struct stat st;
    // stat 函数与 lstat 函数的区别：
    // 当一个文件是符号链接时，lstat 函数返回的是该符号链接本身的信息；而 stat 函数返回的是该链接指向文件的信息。
    if (command_line_arg) {
        err = stat(absolute_name, &st);
    } else {
        //printf("list_num=%d\n", print_nums);
        if (print_nums >= list_max_num) {
            return;
        }
        // 要放在判断之后，不然下次循环再加了
        print_nums++;
        err = lstat(absolute_name, &st);
    }
    if (err != 0) {
        error(LS_FAILURE, err, _("cannot access %s\n"), absolute_name);
        return;
    }
    if (backup_dir) {
        char *name_base;
        ASSIGN_BASENAME_STRDUPA (name_base, name);
        if (! dot_or_dotdot(name_base)) {
            backup_name = alloca(strlen(name_base) + strlen(backup_dir) + 2);
            attach(backup_name, backup_dir, name_base);
        } else {
            backup_name = strdup(backup_dir);
        }
    }

    //@todo 根据st.st_dev, st.st_ino检查文件是否被访问过，防止软链导致死循环

    /*
       S_ISLNK(st_mode)：是否是一个连接.
       S_ISREG(st_mode)：是否是一个常规文件.
       S_ISDIR(st_mode)：是否是一个目录
       S_ISCHR(st_mode)：是否是一个字符设备.
       S_ISBLK(st_mode)：是否是一个块设备
       S_ISFIFO(st_mode)：是否 是一个FIFO文件.
       S_ISSOCK(st_mode)：是否是一个SOCKET文件
       ————————————————
       原文链接：https://blog.csdn.net/yuan_hong_wei/article/details/50326569
        */
    if (S_ISLNK (st.st_mode)) {
        //printf("link: %s \n", absolute_name);
        print_current_file(absolute_name, st, backup_name);
    } else if (S_ISDIR(st.st_mode)) {
        //printf("depth=%d\n", depth);
        //printf("dir: %s \n", absolute_name);
        if (depth >= depth_max_num) {
            if (!command_line_arg || depth_max_num == 0) {
                print_current_file(absolute_name, st, backup_name);
            }
            return;
        } else {
            //如果要往下访问，提前+1
            depth++;
            print_dir(absolute_name, backup_name, depth);
            if (!command_line_arg || depth_max_num == 0) {
                print_current_file(absolute_name, st, backup_name);
            }
        }
    } else {
        //printf("file: %s \n", absolute_name);
        print_current_file(absolute_name, st, backup_name);
    }
    // 让电脑休息一下
    if (sleep_time > 0) {
        usleep(sleep_time);
    }
}

static void
print_dir(const char *dirname, char const *backup_dir, int depth)
{
    DIR *dirp;
    struct dirent *next;
    int index = 0;
    //printf("%s\n", dirname);

    errno = 0;
    dirp = opendir(dirname);
    if (!dirp) {
        error(LS_FAILURE, errno, _("cannot open directory %s\n"), dirname);
        return;
    }
    while (1)
    {
        // 重置错误码
        errno = 0;
        next = readdir(dirp);
        if (next) {
            if (! dot_or_dotdot(next->d_name)) {
                index++;
                enum filetype type = unknown;
                switch (next->d_type)
                {
                    case DT_BLK:  type = blockdev;		break;
                    case DT_CHR:  type = chardev;		break;
                    case DT_DIR:  type = directory;		break;
                    case DT_FIFO: type = fifo;		break;
                    case DT_LNK:  type = symbolic_link;	break;
                    case DT_REG:  type = normal;		break;
                    case DT_SOCK: type = sock;		break;
                }
                gobble_file(next->d_name, type, next->d_ino, false, dirname, backup_dir, depth);
            }
        } else if (errno != 0) {
            error(LS_FAILURE, errno, _("reading directory %s\n"), dirname);
            break;
        } else {
            break;
        }
    }
    if (index == 0) {
        // @todo 目录为空，检查是否要删除目录
    }
    closedir(dirp);
}

// 因为有存在打印多个目录的文件，所以统计用完整目录打印，不然要处理很多格式问题
static void print_current_file (const char *absolute_name, struct stat st, char const *backup_name)
{
    total_size += st.st_size;
    switch (format)
    {
        case one_per_line:
            print_one_per_line (absolute_name, st, backup_name);
            break;
        case long_format:
            {
                if (expire_opt_set) {
                    print_one_per_line (absolute_name, st, backup_name);
                } else {
                    print_long_format(absolute_name, st, backup_name);
                }
                break;
            }
    }
}

// 打印多些数据
static void print_long_format(const char *absolute_name, struct stat st, char const *backup_name)
{
    //todo 如果输出超过可见区域长度会提前换行，导致回车清除不干净
    if (print_format == FORMAT_R) {
        printf("\r\033[K");
    }
    char modebuf[12];
    struct tm *when_local;
    char timeStr[80];
    char *p;
    char buf[1024]; 

    p = buf;
    filemodestring (&st, modebuf);
    sprintf (p, "%s ", modebuf);
    p += strlen (p);

    char hbuf[LONGEST_HUMAN_READABLE + 1];
    char const *size = human_readable (st.st_size, hbuf);
    int pad;
    for (pad = 5 - strlen(size); 0 < pad; pad--)
        *p++ = ' ';
    while ((*p++ = *size++))
        continue;
    *p = '\0';
    fputs (buf, stdout);
    putchar (' ');

    format_user (st.st_uid, 10, true);
    format_group (st.st_gid, 10, true);
    p = buf;

    when_local = localtime (&st.st_mtime);

    strftime(timeStr,sizeof(timeStr),"%F %T",when_local);
    fputs(timeStr, stdout);
    putchar (' ');
    fputs(absolute_name, stdout);
    if (S_ISLNK (st.st_mode)) {
        char *linkname = get_link_name(absolute_name, st.st_size);
        if (linkname != NULL) {
            fputs(" -> ", stdout);
            fputs(linkname, stdout);
            free(linkname);
        }
    } else if (S_ISDIR(st.st_mode)) {
        putchar('/');
    }
    if (print_format == FORMAT_N) {
        putchar ('\n');
    }
}

char *
human_readable (off_t n, char *buf)
{
    // 为了编译能过，第一个if写成这样
    if (n / DU_UNIT > DU_UNIT * DU_UNIT * DU_UNIT) {
        sprintf (buf, "%lld.%lldT", (long long)n / DU_UNIT / DU_UNIT / DU_UNIT / DU_UNIT, (long long)((n /DU_UNIT ) % (DU_UNIT * DU_UNIT * DU_UNIT) != 0));
    } else if (n > DU_UNIT * DU_UNIT * DU_UNIT ) {
        sprintf (buf, "%lld.%lldG", (long long)n / DU_UNIT / DU_UNIT / DU_UNIT, (long long)(n % (DU_UNIT * DU_UNIT * DU_UNIT) != 0));
    } else if (n > DU_UNIT * DU_UNIT ) {
        sprintf (buf, "%lldM", (long long)n / DU_UNIT / DU_UNIT);
    } else if (n > DU_UNIT ) {
        sprintf (buf, "%lldK", (long long)n / DU_UNIT);
    } else {
        sprintf (buf, "%jd", (intmax_t)n);
    }
    return buf;
}

static void
format_user_or_group (char const *name, unsigned long int id, int width)
{
    if (name)
    {
        int width_gap = width - strlen (name);
        int pad = MAX (0, width_gap);
        fputs (name, stdout);

        do
            putchar (' ');
        while (pad--);
    }
    else
    {
        printf ("%*lu ", width, id);
    }
}

/* Print the name or id of the user with id U, using a print width of
   WIDTH.  */

static void
format_user (uid_t u, int width, bool stat_ok)
{
    format_user_or_group (! stat_ok ? "?" :
                          getuser (u), u, width);
}

/* Likewise, for groups.  */

static void
format_group (gid_t g, int width, bool stat_ok)
{
    format_user_or_group (! stat_ok ? "?" :
                          getgroup (g), g, width);
}


// 只打印文件名
static void print_one_per_line(const char *absolute_name, struct stat st, char const *backup_name)
{
    int fs;
    fs = judge_dir_file(absolute_name, st, backup_name);
    switch (fs)
    {
        case FILE_STA_EXPIRE:
        {
            if (print_format == FORMAT_A) {
                if (can_clear_line) {
                    printf("\r\033[K");
                }
                can_clear_line = false;
            } else if (print_format == FORMAT_R) {
                printf("\r\033[K");
            }
            fputs ("expire ", stdout);
            break;
        }
        case FILE_STA_NORMAL:
        {
            if (print_format == FORMAT_A) {
                if (can_clear_line) {
                    printf("\r\033[K");
                }
                can_clear_line = true;
            } else if (print_format == FORMAT_R) {
                printf("\r\033[K");
            }
            fputs ("normal ", stdout);
            break;
        }
        case FILE_STA_REMOVE:
        {
            if (print_format == FORMAT_A) {
                if (can_clear_line) {
                    printf("\r\033[K");
                }
                can_clear_line = false;
            } else if (print_format == FORMAT_R) {
                printf("\r\033[K");
            }
            fputs ("remove ", stdout);
            break;
        }
        default:
        {
            if (print_format == FORMAT_R) {
                printf("\r\033[K");
            }
            break;
        }
    }
    fputs(absolute_name, stdout);
    // 这个模式不对链接文件输出指向的目标名
    if (S_ISDIR(st.st_mode)) {
        putchar('/');
    }
    if (print_format == FORMAT_N || (print_format == FORMAT_A && can_clear_line == false)) {
        putchar ('\n');
    }
}

static char *
get_link_name (char const *filename, size_t size)
{
    char *linkname;
    linkname = areadlink_with_size (filename, size);
    if (linkname == NULL) {
        error (LS_FAILURE, errno, _("cannot read symbolic link %s"),
                  filename);
    }
    return linkname;
}

static void mkdir_all(const char *dirs, struct stat src_st) {
    struct stat dst_sb, parent_sb;
    char *parent;
    if (stat(dirs, &dst_sb) != 0) { 
        parent = mdir_name(dirs);
        if (stat(parent, &parent_sb) != 0) {
            mkdir_all(parent, src_st);
        }
        if (parent != NULL) {
            free(parent);
        }
        //printf("mkdir %s\n", dirs);
        if (mkdir(dirs, src_st.st_mode) != 0) {
            error(LS_FAILURE, errno, _("cannot create directory %s"), dirs);
        }
    } else if (S_ISDIR(dst_sb.st_mode)) {
        return;
    } else {
        error(LS_FAILURE, 0, _("%s is not a directory"), dirs);
    }
}

// 删除目录和文件
static int judge_dir_file(const char *absolute_name, struct stat st, char const *backup_name)
{
    size_t diff = 0;
    if (!expire_opt_set) {
        return FILE_STA_NONE;
    }
    if (expire_day > 0) {
        diff = 86400 * expire_day;
    }
    if (expire_minute > 0) {
        diff += 60 * expire_minute;
    }
    if (now_time - st.st_mtime > diff) {
        if (remove_expire_file) {        
            // 备份数据
            if (target_directory) {
                if (S_ISDIR(st.st_mode)) {
                    mkdir_all(backup_name, st);
                } else if (S_ISLNK(st.st_mode) || S_ISREG(st.st_mode)) {
                    char *parent = mdir_name(backup_name);
                    mkdir_all(parent, st);
                    if (rename(absolute_name, backup_name) != 0) {
                        error(LS_FAILURE, errno, _("cannot move %s to %s"), absolute_name, backup_name);
                    }
                }
            }
            if (S_ISDIR(st.st_mode)) {
                rmdir(absolute_name);
            } else if (S_ISLNK(st.st_mode) || S_ISREG(st.st_mode)) {
                remove(absolute_name);
            }
            return FILE_STA_REMOVE;
        } else {
            return FILE_STA_EXPIRE;
        }
    } else {
        return FILE_STA_NORMAL;
    }
}

// 合并目录和文件名
static void attach(char *dest, const char *dirname, const char *name)
{
    // copy一个指针为了下面++时保留指针原始位置
    const char *dirnamep = dirname;

    //目录不是 "." ,因为单.是默认值，如加在文件前面且没有/会改变文件路径
    if (dirname[0] != '.' || dirname[1] != 0) {
        while (*dirnamep) {
            //*号和++属于同一优先级，且方向都是从右向左的，*p++和*(p++)作用相同
            *dest++ = *dirnamep++;
        }

        // 如果目录长度不为空，且最后不为不为"/" 则追加"/"
        if (dirnamep > dirname && dirnamep[-1] != '/')
            *dest++ = '/';
        
    }
    while (*name)
    {
        *dest++ = *name++;
    }
    // 结束符
    *dest = 0;
}

// 解析参数
static int decode_switches(int argc, char **argv)
{
    format = one_per_line;
    depth_max_num = 1;
    sleep_time = 400;
    list_max_num = 1000;
    print_size = false;
    expire_day = 0;
    expire_minute = 0;

    {
        char const *p = getenv ("TABSIZE");
        tabsize = 8;
        if (p)
        {
            unsigned long int tmp_ulong;
            tmp_ulong = strtoul(p, NULL, 0);
            if (errno != 0 || (int)tmp_ulong > INT_MAX || (int)tmp_ulong < 0) {
                error (LS_FAILURE, 0, _("ignoring invalid tab size in environment variable TABSIZE: %s"),
                       p);
            } else {
                tabsize = tmp_ulong;
            }
        }
    }

    for (;;) {
        int oi = -1;
        //形式如 a:b::cd: ，分别表示程序支持的命令行短选项有-a、-b、-c、-d，冒号含义如下：
        //(1)只有一个字符，不带冒号——只表示选项， 如-c 
        //(2)一个字符，后接一个冒号——表示选项后面带一个参数，如-a 100
        //(3)一个字符，后接两个冒号——表示选项后面带一个可选参数，选项与参数之间不能有空格, 形式应该如-b200
        int c = getopt_long(argc, argv, 
                            "d:ln:rs",
                            long_options, &oi);

        //每次执行会打印多次, 最后一次也是-1
        //printf("c=%d, optind=%d\r\n", c, optind);
        if (c == -1) {
            break;
        }
        switch (c)
        {
            case BACKUP_OPTION:
            {
                if (target_directory) {
                    error(LS_FAILURE, 0, _("MULTIPLE target directories specified"));
                } else {
                    struct stat st;
                    if (stat(optarg, &st) != 0) {
                        error(LS_FAILURE, errno, _("accessing %s"), optarg);
                    }
                    if (! S_ISDIR(st.st_mode)) {
                        error(LS_FAILURE, 0, _("target %s is not a directory"), optarg);
                    } 
                    target_directory = (char *)optarg;   
                }
                break;
            }
            case 'd':
            {
                //初始化，防被其它影响
                errno = 0;
                unsigned long int tmp_ulong;
                //optarg 表示当前选项对应的参数值
                tmp_ulong = strtoul(optarg, NULL, 0);

                // 转成有符号的判断溢出，否则识别不了负数
                if (errno != 0 || (int)tmp_ulong > INT_MAX || (int)tmp_ulong < 0) {
                    error (LS_FAILURE, 0, _("invalid --depth: %s"),
                           optarg);
                }
                //printf("%ul,%ul\n", tmp_ulong, SIZE_MAX);
                depth_max_num = tmp_ulong;
                break;
            }
            case EXPIRE_DAY_OPTION:
            {
                //初始化，防被其它影响
                errno = 0;
                unsigned long int tmp_ulong;
                //optarg 表示当前选项对应的参数值
                tmp_ulong = strtoul(optarg, NULL, 0);
                // 转成有符号的判断溢出，否则识别不了负数
                if (errno != 0 || (int)tmp_ulong > INT_MAX || (int)tmp_ulong < 0) {
                    error (LS_FAILURE, 0, _("invalid --expire-day: %s"),
                           optarg);
                }
                expire_day = tmp_ulong;
                expire_opt_set = true;
                if ( print_format == FORMAT_N ) {
                    print_format = FORMAT_A;
                }
                break;
            }
            case EXPIRE_MIN_OPTION:
            {
                //初始化，防被其它影响
                errno = 0;
                unsigned long int tmp_ulong;
                //optarg 表示当前选项对应的参数值
                tmp_ulong = strtoul(optarg, NULL, 0);
                // 转成有符号的判断溢出，否则识别不了负数
                if (errno != 0 || (int)tmp_ulong > INT_MAX || (int)tmp_ulong < 0) {
                    error (LS_FAILURE, 0, _("invalid --expire-min: %s"),
                           optarg);
                }
                expire_minute = tmp_ulong;
                expire_opt_set = true;
                if ( print_format == FORMAT_N ) {
                    print_format = FORMAT_A;
                }
                break;
            }
            case 'l':
                format = long_format;
                break;
            case 'n':
            {
                //初始化，防被其它影响
                errno = 0;
                unsigned long int tmp_ulong;
                //optarg 表示当前选项对应的参数值
                tmp_ulong = strtoul(optarg, NULL, 0);
                // 转成有符号的判断溢出，否则识别不了负数
                if (errno != 0 || (int)tmp_ulong > INT_MAX || (int)tmp_ulong < 0) {
                    error (LS_FAILURE, 0, _("invalid --num: %s"),
                           optarg);
                }
                list_max_num = tmp_ulong;
                break;
            }
            case REMOVE_OPTION:
                remove_expire_file = true;
                //如果要删除,单行输出
                format = one_per_line;
                break;
            case 'r':
            {
                print_format = FORMAT_R;
                break;   
            }
            case 's':
                print_size = true;
                break;
            case SLEEP_OPTION:
            {
                //初始化，防被其它影响
                errno = 0;
                unsigned long int tmp_ulong;
                //optarg 表示当前选项对应的参数值
                tmp_ulong = strtoul(optarg, NULL, 0);
                // 转成有符号的判断溢出，否则识别不了负数
                if (errno != 0 || (int)tmp_ulong > INT_MAX || (int)tmp_ulong < 0) {
                    error (LS_FAILURE, 0, _("invalid --sleep: %s"),
                           optarg);
                }
                sleep_time = tmp_ulong;
                break;
            }
            case AUTHOR_OPTION:
                print_author = true;
                break;
            case GETOPT_VERSION_CHAR:
                printf("ver=0.1\r\n");
                break;
            case GETOPT_HELP_CHAR:
                usage(EXIT_SUCCESS);
                break;
            default:
                usage(LS_FAILURE);
                break;
        }
    }
    //optind：表示的是下一个将被处理到的参数在argv中的下标值
    return optind;
}

void usage(int status)
{
    char const *lang_env;
    
    lang_env = getenv ("LANG");
    if (lang_env != NULL && strcmp(lang_env, "zh_CN.UTF-8") == 0) {
        usage_zh(status);
        return;
    }

    if (status != EXIT_SUCCESS) {
        fprintf(stderr, _("Try '%s --help' for more information\n"), program_name);
    } else {
        printf(_("Usage: %s [OPTION]... [FILE]...\n"), program_name);
        fputs(_("\
Slow list information about the FILEs (the current directory by default).\n\
\n\
"), stdout);
        fputs(_("\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
        fputs(_("\
        --backup-to=TARGET      backup deleted expire files to TARGET directory\n\
                                must be used with --expire-day | --expire-min and --remove together\n\
    -d, --depth=NUM             list subdirectories recursively depth\n\
        --expire-day=NUM        File's data was last modified n*24 hours ago.\n\
        --expire-min=NUM        File's data was last modified n minutes ago.\n\
    -l                          use a long listing format\n\
                                this option will disappear when with --expire-day\n\
                                or --expire-min together\n\
    -n, --num=NUM               max list file nums, default 1000\n\
        --remove                delete files one by one of all lists\n\
                                must be used with --expire-day together\n\
    -r                          reback to line head and rewrite this line\n\
    -s, --size                  print the total size of all files\n\
        --sleep=NUM             sleep time (us) when show every one file, default 400\n\
"), stdout);
        fputs(_("\
\n\
Exit status:\n\
    0   if OK,\n\
    1   if minor problems(e.g., cannot access subdirectory),\n\
    2   if serious trouble (e.g., cannot access command-line argument).\n\
"), stdout);
        emit_ancillary_info();
    }
    exit(status);
}

// 中文
void usage_zh(int status)
{
    if (status != EXIT_SUCCESS) {
        fprintf(stderr, _("Try '%s --help' for more information\n"), program_name);
    } else {
        printf(_("用法: %s [OPTION]... [FILE]...\n"), program_name);
        fputs(_("\
1. 查看文件夹内文件, 支持递归查看子文件夹. 代替(ls -l 或 find . -type f 或 find . -maxdepth 2)\n\
2. 过期文件清理并支持备份. 代替(find . -mtime +3|xargs rm -rf)\n\
3. 统计目录总大小. 代替(du -sh dir/) \n\
注：参数[FILE]不提供时默认操作当前目录.\n\
\n\
"), stdout);
        fputs(_("\
凡对长选项来说不可省略的参数,对于短选项也是不可省略的.\n\
"), stdout);
        fputs(_("\
        --backup-to=TARGET      备份要删除的过期文件到目标目录\n\
                                必须和 --expire-day | --expire-min 以及 --remove 一起使用时才有效\n\
    -d, --depth=NUM             显示子文件夹深度，默认为0\n\
        --expire-day=NUM        检查最后修改日期为 n*24 小时前的文件.\n\
        --expire-min=NUM        检查最后修改日期为 n 分钟前的文件.\n\
    -l                          使用长列表格式\n\
                                如果使用了 --expire-day 或 --expire-min 此选项失效 \n\
    -n, --num=NUM               最大显示文件数, 默认 1000\n\
        --remove                删除过期文件\n\
                                需要配合 --expire-day 或 --expire-min 使用\n\
    -r                          同一行覆盖刷新输出\n\
    -s, --size                  打印访问过的文件的总大小\n\
        --sleep=NUM             每打印一个文件的休息间隔 (微秒) , 默认 400微秒\n\
"), stdout);
        fputs(_("\
\n\
Exit status:\n\
    0   正确,\n\
    1   轻微错误(如: 无法访问子文件夹),\n\
    2   严重错误 (如: 无法解析命令行参数).\n\
"), stdout);
        emit_ancillary_info();
    }
    exit(status);
}
