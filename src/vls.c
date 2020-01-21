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
//最大访问数
static size_t list_max_num;
//休息时间
static size_t sleep_time;
//总blocks
static uintmax_t total_blocks;
//脚本执行时间
static int now_time;

/* Human-readable options for output.  */
static int human_output_opts;

/* The units to use when printing sizes other than file sizes.  */
static uintmax_t output_block_size;

/* Likewise, but for file sizes.  */
static uintmax_t file_output_block_size = 1;

// 把文件copy到此目录
static char *copy_to_dir;

/* True means to display author information.  */

static bool print_author;

/* True means mention the size in blocks of each file.  -s  */

static bool print_block_size;

// True 则删除过期文件
static bool remove_expire_file;

enum format
{
    long_format,		/* -l and other options that imply -l */
    one_per_line,		/* -1 */
    many_per_line,		/* -C */
    horizontal,			/* -x */
    with_commas			/* -m */
};

static enum format format;

static int decode_switches(int argc, char **argv);
void usage(int status);
static inline void emit_ancillary_info (void);
static uintmax_t gobble_file(char const *name, enum filetype type,
                            ino_t inode, bool command_line_arg,
                            char const *dirname, int depth);
static void attach(char *dest, const char *dirname, const char *name);                            
static void print_dir_files(const char *absolute_name, int depth);

enum {
    LS_MINOR_PROBLEM = 1,
    LS_FAILURE = 2
};
enum {
    GETOPT_HELP_CHAR = CHAR_MAX - 2,
    GETOPT_VERSION_CHAR = CHAR_MAX - 3,
    AUTHOR_OPTION = CHAR_MAX + 1,
    EXPIRE_DAY_OPTION,
    COPY_OPTION,
    DEPTH_OPTION,
    REMOVE_OPTION,
    SLEEP_OPTION
};

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
    {"copy-to", required_argument, NULL, COPY_OPTION},
    {"depth", required_argument, NULL, DEPTH_OPTION},
    {"expire-day", required_argument, NULL, EXPIRE_DAY_OPTION},
    {"human-readable", no_argument, NULL, 'h'},
    {"num", required_argument, NULL, 'n'},
    {"remove", no_argument, NULL, REMOVE_OPTION},
    {"size", no_argument, NULL, 's'},
    {"sleep", required_argument, NULL, SLEEP_OPTION},
    {"author", no_argument, NULL, AUTHOR_OPTION},
    {GETOPT_HELP_OPTION_DECL},
    {GETOPT_VERSION_OPTION_DECL},
    {NULL, 0, NULL, 0}
};

int main(int argc, char **argv)
{
    int i;
    int n_files;

    now_time = time(NULL);

    set_program_name(argv[0]);
    i = decode_switches(argc, argv);
    //printf("i=%d\r\n", i);

    //atexit(close_stdout);
    exit_status = EXIT_SUCCESS;

    // 相减是传的文件数
    n_files = argc - i;
    if (n_files <= 0) {
        // 没传入目录
        total_blocks += gobble_file(".", directory, NOT_AN_INODE_NUMBER, true, "", 0);
    } else {
        // 有传入目录
        do {
            total_blocks += gobble_file(argv[i++], unknown, NOT_AN_INODE_NUMBER, true, "", 0);
        } while (i < argc);
    }

    if (copy_to_dir != NULL) {
        printf("dst=%s\r\n", copy_to_dir);
    }
    if (print_block_size) {
        printf("total %d\n", total_blocks);
    }
    exit(exit_status);
}

static uintmax_t
gobble_file(char const *name, enum filetype type, ino_t inode, 
            bool command_line_arg, char const *dirname, int depth)
{
    uintmax_t blocks = 0;
    // 如果command_line_arg 为false或者inode 为0则正常
    assert (! command_line_arg || inode == NOT_AN_INODE_NUMBER);
    //printf("name=%s\n", name);
    
    char *absolute_name;
    int err;
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
        //printf("list_num=%d\n", list_max_num);
        // 显示条数，必须为==0,再减就是正数最大值了，可以用SIZE_MAX检查溢出
        if (list_max_num == 0 || list_max_num >= SIZE_MAX) {
            return 0;
        }
        // 要放在判断0之后，不然下次循环再减就是正数最大了
        list_max_num--;
        err = lstat(absolute_name, &st);
    }
    if (err != 0) {
        error(LS_FAILURE, err, _("cannot access %s\n"), absolute_name);
        return 0;
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
        printf("link: %s \n", absolute_name);
    } else if (S_ISDIR(st.st_mode)) {
        //printf("depth=%d\n", depth);
        printf("dir: %s \n", absolute_name);
        if (depth >= depth_max_num) {
            return 0;
        }
        //如果要往下访问，提前+1
        depth++;
        print_dir_files(absolute_name, depth);
    } else {
        printf("file: %s \n", absolute_name);                    
    }
    // 让电脑休息一下
    if (sleep_time > 0) {
        usleep(sleep_time);
    }
    blocks = ST_NBLOCKS(st);
    return blocks;
}

static void
print_dir_files(const char *dirname, int depth)
{
    DIR *dirp;
    struct dirent *next;
    static bool first = true;
    int index = 0;
    //printf("%s\n", dirname);

    errno = 0;
    dirp = opendir(dirname);
    if (!dirp) {
        error(LS_FAILURE, errno, _("cannot open directory"), dirname);
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
                total_blocks += gobble_file(next->d_name, type, next->d_ino, false, dirname, depth);
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
    format = many_per_line;
    sleep_time = 400;
    list_max_num = 10000;
    print_block_size = false;

    for (;;) {
        int oi = -1;
        //形式如 a:b::cd: ，分别表示程序支持的命令行短选项有-a、-b、-c、-d，冒号含义如下：
        //(1)只有一个字符，不带冒号——只表示选项， 如-c 
        //(2)一个字符，后接一个冒号——表示选项后面带一个参数，如-a 100
        //(3)一个字符，后接两个冒号——表示选项后面带一个可选参数，选项与参数之间不能有空格, 形式应该如-b200
        int c = getopt_long(argc, argv, 
                            "hln:s",
                            long_options, &oi);

        //每次执行会打印多次, 最后一次也是-1
        //printf("c=%d, optind=%d\r\n", c, optind);
        if (c == -1) {
            break;
        }
        switch (c)
        {
        case COPY_OPTION:
        {
            copy_to_dir = (char *)optarg;
            //copy_to_dir = strdup(optarg)
            break;
        }
        case DEPTH_OPTION:
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
            printf("%ul,%ul\n", tmp_ulong, SIZE_MAX);
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
            break;
        }
        case 'h':
            human_output_opts = human_autoscale | human_SI | human_base_1024;
            file_output_block_size = output_block_size = 1;
            break;
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
            break;
        case 's':
            print_block_size = true;
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
        --copy-to=TARGET        copy list files to TARGET directory\n\
        --depth=NUM             list subdirectories recursively depth\n\
        --expire-day=NUM        File's data was last modified n*24 hours ago.\n\
    -h, --human-readable        with -l, print sizes in human readable format\n\
                                (e.g., 1K 234M 2G)\n\
    -l                          use a long listing format\n\
    -n, --num=NUM               max list file nums, default 10000\n\
        --remove                delete files one by one of all lists\n\
                                must be used with --expire-day together\n\
    -s, --size                  print the allocated size of each file, in blocks\n\
        --sleep=NUM             sleep time (ms) when show every one file, default 400\n\
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
