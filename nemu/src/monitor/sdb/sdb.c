/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdint.h>
#include "sdb.h"
#include "utils.h"
#include "memory/paddr.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_si(char *args) {
  int steps = args ? atoi(args) : 1;
  if (steps <= 0){
    Log("[WARNING]:The input is illegal!\n");
    steps = 1; // 对非法输入, 默认执行"si 1"
  }
  cpu_exec((uint64_t)steps);
  return 0;
}

uint64_t eval_EXPR(char *args){
  return strtol(args, NULL, 0);//TODO
}

static int cmd_info(char *args) {
  char *arg = strtok(NULL, " ");
  if (arg == NULL) {
    Log("[WARNING]:The input is illegal!\n");
  }
  else if (strcmp(arg, "r") == 0) {
    isa_reg_display();
  }
  else if (strcmp(arg, "w") == 0) {
    watchpoint_display();
  }
  else {
    Log("[WARNING]:The input \"%s\" is illegal!\n", args);
  }
  return 0;
}

static int cmd_x(char *args) {
  char *arg = strtok(NULL, " ");
  if(arg == NULL) {
    Log("[WARNING]:The input is illegal!\n");
    return 0;
  }
  int n = atoi(arg);
  arg = strtok(NULL, " ");
  if(arg == NULL) {
    Log("[WARNING]:The input is illegal!\n");
    return 0;
  }
  uint64_t addr = strtol(arg, NULL, 0); //TODO：eval_EXPR
  printf("Examine memory addresses 0x%lx to 0x%lx.", addr, addr + 4 * n -1);
  for(int i = 0; i < n; i++){
    printf(ANSI_FG_BLUE "\n0x%lx:" ANSI_NONE, addr);
    for(int j = 0; j < 4; j++){
      printf(" 0x%02x ", (*(uint8_t *)(guest_to_host(addr + j))));
    }
    addr += 4;
  }
  printf("\n");
  return 0;
}

static int cmd_p(char *args) {
  char buff[64] = {'\0'};
  char *arg;
  while((arg = strtok(NULL, " ")) != NULL){
    strcat(buff, arg);
  }
  if(buff[0] == '\0') {
    Log("[WARNING]:The input is illegal!\n");
    return 0;
  }
	bool flag = true;
  uint32_t result = expr(buff, &flag);
  if(flag == true){
    printf("0x%x %d\n", result, result);
  }
  else{
    Log("[WARNING]:The input is illegal!\n");
  }
  return 0 ;
}

static int cmd_w(char *args) {
  char buff[64] = {'\0'};
  char *arg;
  while((arg = strtok(NULL, " ")) != NULL){
    strcat(buff, arg);
  }
  if(buff[0] == '\0') {
    Log("[WARNING]:The input is illegal!\n");
    return 0;
  }
  bool flag = true;
  WP *wp = new_wp();
  strcpy(wp->expr, buff);
  wp->old_val = expr(wp->expr, &flag);
  if(flag == true){
    printf("Set watchpoint with num %d, now the value is %ld\n", wp->NO, wp->old_val);
  }
  else{
    Log("[WARNING]:The input is illegal!\n");
  }
  return 0 ;
}

static int cmd_d(char *args) {
  char *arg = strtok(NULL, " ");
  if(arg == NULL) {
    Log("[WARNING]:The input is illegal!\n");
    return 0;
  }
  int n = atoi(arg);
  if(n < 0 || n >= NR_WP) {
    Log("[WARNING]:The input is illegal!\n");
    return 0;
  }
  free_wp(n);
  return 0 ;
}


static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Step to the next machine instruction", cmd_si },
  { "info", "Display information about registers or watchpoints", cmd_info },
  {"x", "Examine memory contents starting from a specified address", cmd_x},
  {"p", "Print the evaluated result of an expression containing register names", cmd_p},
  {"w", "Pause the program when the value of the watchpoint changes", cmd_w},
  {"d", "Delete a watchpoint by its numeric identifier", cmd_d},

  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%-4s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%-4s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
