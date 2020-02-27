# vls

功能：
===
1. 查看文件夹内文件, 支持递归查看子文件夹. 代替(ls -l 或 find . -type f 或 find . -maxdepth 2)
2. 过期文件清理并支持备份. 代替(find . -mtime +3|xargs rm -rf)
3. 统计目录总大小. 代替(du -sh dir/)

中文帮助
===
```
export LANG=zh_CN.UTF-8
```

安装
===
二进制包[rpm](http://cloudme.io/vls.rpm) [deb](http://cloudme.io/vls.deb) [aur](https://aur.archlinux.org/packages/vls/)

源码安装
```
./configure
make
make install
```

用法
===
```
: vls [OPTION]... [FILE]...
1. 查看文件夹内文件, 支持递归查看子文件夹. 代替(ls -l 或 find . -type f 或 find . -maxdepth 2)
2. 过期文件清理并支持备份. 代替(find . -mtime +3|xargs rm -rf)
3. 统计目录总大小. 代替(du -sh dir/) 
注：参数[FILE]不提供时默认操作当前目录.

凡对长选项来说不可省略的参数,对于短选项也是不可省略的.
    -a                          同时设置-d 和 -n 为int最大值\n\
        --backup-to=TARGET      备份要删除的过期文件到目标目录
                                必须和 --expire-day | --expire-min 以及 --remove 一起使用时才有效
    -d, --depth=NUM             显示子文件夹深度，默认为0
        --expire-day=NUM        检查最后修改日期为 n*24 小时前的文件.
        --expire-min=NUM        检查最后修改日期为 n 分钟前的文件.
    -l                          使用长列表格式
                                如果使用了 --expire-day 或 --expire-min 此选项失效 
    -n, --num=NUM               最大显示文件数, 默认 1000
        --remove                删除过期文件
                                需要配合 --expire-day 或 --expire-min 使用
    -r                          同一行覆盖刷新输出
    -s, --size                  打印访问过的文件的总大小
        --sleep=NUM             每打印一个文件的休息间隔 (微秒) , 默认 400微秒
```

示例
===
```
# 查看两级文件夹文件
vls -d2 -l

# 查看文件夹内修改时间超过2分钟前的文件
vls --expire-min=2

# 删除文件夹内修改时间超过2分钟前的文件
vls --expire-min=2 --remove

# 删除/src下的过期文件前做备份
vls --expire-min=2 --remove --backup-to=/tmp/ /src/

#查看文件夹大小
vls -asr

#增加cpu, io休息时间
vls --sleep=10000
```

Todo
===
* ~~增加 -a 参数同时设置-d和-n为int最大值~~
* 增加参数统计子目录各自大小
* ~~输出文件名转义~~
* ~~回退显示模式需要判断是否超出屏幕宽度~~
* 读取文件文本内容并sleep后输出
* ~~用户名和用户组列做定长截断~~
* 增加类stat file形式的一个文件属性多列显示

编程小提示
===
修改版本修改configure.ac

增加新代码文件

    * 修改对应目录的Makefile.am和src目录的Makefile.am
    * 运行 aclocal  autoconf autoheader automake 生成相关文件

很多文件是autoscan automake生成的，可以百度相关教程 如：https://www.jianshu.com/p/17e777868d6b   
