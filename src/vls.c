#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <limits.h>
#include <sys/types.h>
#include <error.h>
#include <errno.h>
#include "../lib/progname.h"
#include "../lib/system.h"
#include "../lib/human.h"
#include <assert.h>

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

#define true 1
#define bool _Bool
static int exit_status;

static size_t depth_num;
static size_t expire_day;
static size_t list_max_num;

/* Human-readable options for output.  */
static int human_output_opts;

/* The units to use when printing sizes other than file sizes.  */
static uintmax_t output_block_size;

/* Likewise, but for file sizes.  */
static uintmax_t file_output_block_size = 1;

static char *dst_file;

/* True means to display author information.  */

static bool print_author;

/* True means mention the size in blocks of each file.  -s  */

static bool print_block_size;

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
                            char const *dirname);

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
    REMOVE_OPTION
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
    {"author", no_argument, NULL, AUTHOR_OPTION},
    {GETOPT_HELP_OPTION_DECL},
    {GETOPT_VERSION_OPTION_DECL},
    {NULL, 0, NULL, 0}
};

int main(int argc, char **argv)
{
    int i;
    int n_files;
    set_program_name(argv[0]);
    i = decode_switches(argc, argv);
    printf("%d\r\n", i);

    //atexit(close_stdout);
    exit_status = EXIT_SUCCESS;

    // 相减是传的文件数
    n_files = argc - i;
    if (n_files <= 0) {
        // 没传入目录
        gobble_file(".", directory, NOT_AN_INODE_NUMBER, true, "");
    } else {
        // 有传入目录
        do {
            gobble_file(argv[i++], unknown, NOT_AN_INODE_NUMBER, true, "");
        } while (i < argc);
    }

    if (dst_file != NULL) {
        printf("dst=%s\r\n", dst_file);
    }
    
    exit(exit_status);
}

static uintmax_t
gobble_file(char const *name, enum filetype type, ino_t inode, 
            bool command_line_arg, char const *dirname)
{
    // 如果command_line_arg 为false或者inode 为0则正常
    assert (! command_line_arg || inode == NOT_AN_INODE_NUMBER);
    printf("%s\n", name);
}

static int decode_switches(int argc, char **argv)
{
    format = many_per_line;

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
            dst_file = (char *)optarg;
            //dst_file = strdup(optarg)
            break;
        }
        case DEPTH_OPTION:
        {
            //初始化，防被其它影响
            errno = 0;
            unsigned long int tmp_ulong;
            //optarg 表示当前选项对应的参数值
            tmp_ulong = strtoul(optarg, NULL, 0);
            // >=SIZE_MAX 判断溢出，比如用printf打印为-1,使用depth_num < 0判断不了
            if (errno != 0 || tmp_ulong >= SIZE_MAX) {
                error (LS_FAILURE, 0, _("invalid --depth: %s"),
                       optarg);
            }
            depth_num = tmp_ulong;
            break;
        }
        case EXPIRE_DAY_OPTION:
        {
            //初始化，防被其它影响
            errno = 0;
            unsigned long int tmp_ulong;
            //optarg 表示当前选项对应的参数值
            tmp_ulong = strtoul(optarg, NULL, 0);
            // >=SIZE_MAX 判断溢出
            if (errno != 0 || tmp_ulong >= SIZE_MAX) {
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
            // >=SIZE_MAX 判断溢出
            if (errno != 0 || tmp_ulong >= SIZE_MAX) {
                error (LS_FAILURE, 0, _("invalid --num: %s"),
                       optarg);
            }
            list_max_num = tmp_ulong;
            break;
        }
        case REMOVE_OPTION:
            printf("r\r\n");
            break;
        case 's':
            print_block_size = true;
            break;
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
    -n, --num=NUM               max list file nums\n\
        --remove                delete files one by one of all lists\n\
                                must be used with --expire-day together\n\
    -s, --size                  print the allocated size of each file, in blocks\n\
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
