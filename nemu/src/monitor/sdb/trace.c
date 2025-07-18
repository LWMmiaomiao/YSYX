#include "trace.h"
#include "isa.h"

ItraceNode iringbuf[MAX_IRINGBUF];
int p_cur = 0;
bool full = false;

//called in isa_exec_once
//考虑到出错指令记录, 需要在取指之后执行之前
void trace_iringbuf(word_t pc, uint32_t inst) {
  iringbuf[p_cur].pc = pc;
  iringbuf[p_cur].inst = inst;
  p_cur = (p_cur + 1) % MAX_IRINGBUF;
  full = full || p_cur == 0;
}

//called in isa_reg_display
//iringbuf和reg一起输出
void display_iringbuf() {
  if (!full && p_cur == 0) return;

  int end = p_cur;
  int i = full?p_cur:0;

  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
  char buf[128];
  char *p;
  printf("[Iringbuf]:Most recently executed instructions:\n");
  do {
    p = buf;
    p += sprintf(buf, "%s" FMT_WORD ": %08x ", (i+1)%MAX_IRINGBUF==end?" --> ":"     ", iringbuf[i].pc, iringbuf[i].inst);
    disassemble(p, buf+sizeof(buf)-p, iringbuf[i].pc, (uint8_t *)&iringbuf[i].inst, 4);

    if ((i+1)%MAX_IRINGBUF==end) printf(ANSI_FG_RED);
    printf("%s", buf);
  } while ((i = (i+1)%MAX_IRINGBUF) != end);
  printf(ANSI_NONE);
}

//called in paddr_read and paddr_wtite 
void mtrace_pread(paddr_t addr, int len, word_t data) {
  #ifndef CONFIG_MTRACE_END
  #define CONFIG_MTRACE_END 0x88000000
  #endif
  #ifndef CONFIG_MTRACE_START
  #define CONFIG_MTRACE_START 0x80000000
  #endif
  if(addr < CONFIG_MTRACE_END && addr >= CONFIG_MTRACE_START){
    log_write("[MTRACE]PC "FMT_PADDR" Memory read at " FMT_PADDR " len=%d, data=" FMT_WORD "\n", cpu.pc, addr, len, data);
  }
}

void mtrace_pwrite(paddr_t addr, int len, word_t data) {
  #ifndef CONFIG_MTRACE_END
  #define CONFIG_MTRACE_END 0x88000000
  #endif
  #ifndef CONFIG_MTRACE_START
  #define CONFIG_MTRACE_START 0x80000000
  #endif
  if(addr < CONFIG_MTRACE_END && addr >= CONFIG_MTRACE_START){
    log_write("[MTRACE]PC "FMT_PADDR" Memory write at " FMT_PADDR " len=%d, data=" FMT_WORD "\n", cpu.pc, addr, len, data);
  }
}

MUXDEF(CONFIG_RV64, Elf64_Shdr, Elf32_Shdr) strtab_shdr;
MUXDEF(CONFIG_RV64, Elf64_Shdr, Elf32_Shdr) sym_shdr;

FILE * elf_fp = NULL;

void init_ftrace(const char * elf_file)  {
	unsigned long ret;
	MUXDEF(CONFIG_RV64, Elf64_Ehdr, Elf32_Ehdr) ehdr;
	FILE * fp = fopen(elf_file, "rb");
	Assert(fp, "Can not open \"%s\"", elf_file);
	Log("Open ELF file \"%s\" successfully", elf_file);
	elf_fp = fp;
	ret = fread(&ehdr, sizeof(ehdr), 1, fp);
	assert(ret != 0);
	Assert(
		ehdr.e_ident[0] == 0x7f &&
		ehdr.e_ident[1] == 'E' &&
    ehdr.e_ident[2] == 'L' &&
		ehdr.e_ident[3] == 'F',
		"Invalid ELF file"
	);
	ret = fseek(fp, ehdr.e_shoff, SEEK_SET);

	for(int i = 0; i < ehdr.e_shnum; i++)
	{
		ret = fread(&sym_shdr, sizeof(sym_shdr), 1, fp);
		if(sym_shdr.sh_type == SHT_SYMTAB)
			break;
	}
	/* find the corresponding STRTAB section using sh_link, which may only work when OS/ABI is SV */
	ret = fseek(fp,ehdr.e_shoff + sym_shdr.sh_link * sizeof(strtab_shdr), SEEK_SET);
	ret = fread(&strtab_shdr, sizeof(strtab_shdr), 1, fp);
}

void trace_func(paddr_t addr, int op)
{
	unsigned long ret;
	MUXDEF(CONFIG_RV64, Elf64_Sym, Elf32_Sym) sym;
	if(elf_fp == NULL)
		return;
	ret = fseek(elf_fp, strtab_shdr.sh_offset, SEEK_SET);
	assert(ret == 0);
	char * strtab = malloc(strtab_shdr.sh_size);
	ret = fread(strtab, strtab_shdr.sh_size, 1, elf_fp);
	ret = fseek(elf_fp, sym_shdr.sh_offset, SEEK_SET);
	for(int i = 0; i < sym_shdr.sh_size; i += sizeof(sym))
	{
		ret = fread(&sym, sizeof(sym), 1, elf_fp);
		if(MUXDEF(CONFIG_RV64, ELF64_ST_TYPE, ELF32_ST_TYPE)(sym.st_info) == STT_FUNC 
		   && addr >= sym.st_value && addr < sym.st_value + sym.st_size)
		{
			if(op == FUNC_CALL)
				log_write("SDB: (pc=" FMT_PADDR ") call   " FMT_PADDR " (in %s)\n", cpu.pc, addr, strtab + sym.st_name);
			else if(op == FUNC_RET)
				log_write("SDB: (pc=" FMT_PADDR ") return " FMT_PADDR " (in %s)\n", cpu.pc, addr, strtab + sym.st_name);
		}
	}
	free(strtab);
}

#ifdef CONFIG_ETRACE
void trace_trap(Decode *s) {
	if(s->dnpc == cpu.csr[MTVEC])
		log_write("SDB: (mcause = " FMT_WORD ") Hit trap at " FMT_WORD ", jump to " FMT_WORD "\n",cpu.csr[MCAUSE], s->pc, s->dnpc);
}
#endif