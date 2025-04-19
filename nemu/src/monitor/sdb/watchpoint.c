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

#include "sdb.h"
#include <stdbool.h>
#include <stdint.h>

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp(){
  if(!free_) {
    Log("[WARNING]:No enough resources [wnatchpoint]!\n");
    return NULL;
  }
  WP *ret = free_;
  free_ = free_->next;
  ret->next = head;
  head = ret;
  return ret;
}

void free_wp(int i){
  WP *wp = &wp_pool[i];
  if(wp == head){
    head = head->next;
    wp->next = free_;
    free_ = wp;
    printf("[WATCHPOINT]:Success free point %d!\n", i);
    return ;
  }
  WP *pre = head;
  while(pre->next) {
    if(pre->next == wp){
      pre->next = wp->next;
      wp->next = free_;
      free_ = wp;
      printf("[WATCHPOINT]:Success free point %d!\n", i);
      return ;
    }
    pre = pre->next;
  }
  Log("[WARNING]:Illegal Operation:[This watchpoint is not in use]!\n");
  return ;
}

void watchpoint_display(void){
  WP *p = head;
  while(p){
    printf("[WATCHPOINT]Num:%2d Expr:%s\n", p->NO, p->expr);
    p = p->next;
  }
}

void check_watchpoints(void){
  WP *p = head;
  bool flag;
  while(p) {
    flag = true;
    uint64_t new_val = expr(p->expr, &flag);
    if(new_val != p->old_val){
      uint64_t new_val = expr(p->expr, &flag);
      printf("[INFO]:Hint watchpoint \"%s\" with number %d!\n", p->expr, p->NO);
      printf("       [old value] %9ld [new value] %9ld\n", p->old_val, new_val);
      printf("       [old value] 0x%7lx [new value] 0x%7lx\n", p->old_val, new_val);
      p->old_val = new_val;
      if(nemu_state.state == NEMU_RUNNING){
				nemu_state.state = NEMU_STOP;
      }
    }
    p = p->next;
  }
}