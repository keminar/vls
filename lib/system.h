
#include <stdio.h>
#include "progname.h"

// 转换成字符串常量
#define _(Msgid) ((const char*)Msgid)

//使用static修饰符，函数仅在文件内部可见
//inline函数，告诉编译器把函数代码在编译时直接拷贝到程序中
//在头文件定义函数又想避免冲突使用static inline
static inline void emit_ancillary_info (void)
{
    printf(_("\nReport '%s' bugs to liminggui, qq:372884399\n"), program_name);
    fputs(_("Licence GNU software\n"), stdout);
}
