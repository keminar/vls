#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <limits.h>
#include "../lib/progname.h"
#include "../lib/system.h"

static int decode_switches(int argc, char **argv);
void usage(int status);
static inline void emit_ancillary_info (void);

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

//原文链接：https://blog.csdn.net/qq_33850438/article/details/80172275
//no_argument(或者是0)时 ——参数后面不跟参数值，eg: --version,--help
//required_argument(或者是1)时 ——参数输入格式为：--参数 值 或者 --参数=值。eg:--dir=/home
//optional_argument(或者是2)时  ——参数输入格式只能为：--参数=值
static struct option const long_options[] = 
{
    {"copy", required_argument, NULL, COPY_OPTION},
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
    set_program_name(argv[0]);
    decode_switches(argc, argv);
    /* code */
    return 0;
}

static int decode_switches(int argc, char **argv)
{
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
        //printf("c=%d\r\n", c);
        if (c == -1) {
            break;
        }
        switch (c)
        {
        case COPY_OPTION:
            printf("c\r\n");
            break;
        case DEPTH_OPTION:
        printf("d\r\n");
            break;
        case EXPIRE_DAY_OPTION:
            printf("e\r\n");
            break;
            case 'h':
        printf("h\r\n");
            break;
        case 'n':
        printf("n\r\n");
            break;
        case REMOVE_OPTION:
        printf("r\r\n");
            break;
        case 's':
        printf("s\r\n");
            break;
        case AUTHOR_OPTION:
            printf("aaat\r\n");
            break;
        case GETOPT_VERSION_CHAR:
            printf("bb\r\n");
            break;
        case GETOPT_HELP_CHAR:
            usage(EXIT_SUCCESS);
            break;
        default:
            usage(LS_FAILURE);
            break;
        }                            
    }
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
        --copy=TARGET           copy list files to TARGET directory\n\
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
