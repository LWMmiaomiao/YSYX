# "一生一芯"工程项目

这是"一生一芯"的工程项目. 通过运行
```bash
bash init.sh subproject-name
```
进行初始化, 具体请参考[实验讲义][lecture note].

[lecture note]: https://ysyx.oscc.cc/docs/


6.3
实现iringbuf
    trace_iringbuf对iringbuf[MAX_IRINGBUF]存，取指之后执行之前发生
    display_iringbuf在程序出错时，输出寄存器内容同时输出iringbuf（还是放在case NEMU_QUIT里？）

实现ftrace
    parse_args中接受传入NEMU的ELF文件
[TODO]
gdb: CFLAGS += -g -Og
NEMUFLAGS += -e $(IMAGE).elf