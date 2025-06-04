#include <common.h>
#include <elf.h>
#include <assert.h>
#include "isa.h"

#ifndef TRACE_H
#define TRACE_H

#define MAX_IRINGBUF 16

typedef struct {
  word_t pc;
  uint32_t inst;
} ItraceNode;

//#ifdef CONFIG_IRINGBUF
extern ItraceNode iringbuf[MAX_IRINGBUF];
extern int p_cur;
extern bool full;
//#endif

//called in isa_exec_once
//考虑到出错指令记录, 需要在取指之后执行之前
void trace_iringbuf(word_t pc, uint32_t inst);

//called in isa_reg_display
//iringbuf和reg一起输出
void display_iringbuf(void);

//called in paddr_read and paddr_wtite 
void mtrace_pread(paddr_t addr, int len, word_t data);

void mtrace_pwrite(paddr_t addr, int len, word_t data);


#define FUNC_CALL 0
#define FUNC_RET 1

extern MUXDEF(CONFIG_RV64, Elf64_Shdr, Elf32_Shdr) strtab_shdr;
extern MUXDEF(CONFIG_RV64, Elf64_Shdr, Elf32_Shdr) sym_shdr;

extern FILE * elf_fp;

void init_ftrace(const char * elf_file);

void trace_func(paddr_t addr, int op);

#endif