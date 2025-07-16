# "一生一芯"工程项目

这是"一生一芯"的工程项目. 通过运行
```bash
bash init.sh subproject-name
```
进行初始化, 具体请参考[实验讲义][lecture note].

[lecture note]: https://ysyx.oscc.cc/docs/

[命令]
一键回归测试 make ARCH=$ISA-nemu run


6.3
实现iringbuf
    trace_iringbuf对iringbuf[MAX_IRINGBUF]存，取指之后执行之前发生
    display_iringbuf在程序出错时，输出寄存器内容同时输出iringbuf（还是放在case NEMU_QUIT里？）

实现ftrace
    parse_args中接受传入NEMU的ELF文件
    elf.h里相关内容没看完

6.8
修改完善vsnprintf


[QUE]
abstract-machine/klib/include/klib.h中的#define __NATIVE_USE_KLIB__注释掉


[TODO]
am-kernels/tests/目录下新增一个针对klib的测试集klib-tests
重构ftrace代码

circt.stage.ChiselStage.emitSystemVerilogFile(new cpu.ALU(), args, firtoolOptions)
删除npc/build.mill、npc/playground/src/GCD.scala
添加npc/playground/src/ALU.scala


[HINT]
map_read()和map_write()用于将地址addr映射到map所指示的目标空间, 并进行访问
add_pio_map()函数用于为设备的初始化注册一个端口映射I/O的映射关系. pio_read()和pio_write()是面向CPU的端口I/O读写接口, 它们最终会调用map_read()和map_write(), 对通过add_pio_map()注册的I/O空间进行访问.
paddr_read()和paddr_write()会判断地址addr落在物理内存空间还是设备空间.pmem_read()和pmem_write()来访问真正的物理内存.map_read()和map_write()来访问相应的设备



[RUN]
nemu/目录下编译并运行NEMU:make run
make menuconfig
make ARCH=riscv32-nemu run
运行am-tests:make ARCH=riscv32-nemu run mainargs=v(mainargs的参数RTFC)