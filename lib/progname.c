#include "progname.h"
#include <string.h>

// 指向字符常量的指针，内容由main函数初始化
const char *program_name = NULL;

// 设置进程名
void set_program_name(const char *argv0)
{
    //某些平台，libtool会创建临时执行路径
    //为了删除临时路径中的<dirname>/.libs/或者<dirname>/.libs/lt-前缀
    const char *slash;
    const char *base;

    //字符/在另一个字符串argv0中末次出现的位置
    slash = strrchr(argv0, '/');
    //文件名的指针位置
    base = (slash != NULL ? slash + 1 : argv0);

    //argv0指针指向的是字符串常量的第一个字符
    //bash指针指向的是字符串常量的第N个字符
    //相减大于7指的是/前有大于7个字符，取7是因为/.libs/的长度是7
    //strncmp比较base前7个字符如果等于/.libs/
    if (base - argv0 >= 7 && strncmp(base -7, "/.libs/", 7) == 0) {
        // 去掉/.libs/
        argv0 = base;
        // 如果base是lt-开头的话
        if (strncmp(base, "lt-", 3) == 0) {
            //去掉lt-
            argv0 = base + 3;
        }
    }

    program_name = argv0;
}