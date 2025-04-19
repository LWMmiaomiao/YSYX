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

#include "debug.h"
#include <isa.h>
#include "memory/paddr.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <stdbool.h>
#include <sys/types.h>

#define MAX_TOKEN_LEN 31
#define SUB_EXP_END(i) (tokens[i].type == TK_DEC || tokens[nr_token - 1].type == TK_HEX \
  || tokens[nr_token - 1].type == TK_RBRACKET || tokens[nr_token - 1].type == TK_REG)

typedef enum {
  TK_NOTYPE, TK_PLUS, TK_SUB, TK_MUL, TK_DIV, 
  TK_LBRACKET, TK_RBRACKET, TK_DEC, TK_EQ, TK_NEQ,
  TK_HEX, TK_REG, TK_DEREF
  /* TODO: Add more token types */
} TK_TYPE;

static struct rule {
  const char *regex;
  TK_TYPE token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  {"-? *0[xX][0-9a-fA-F]+", TK_HEX},      // hex
  {" +", TK_NOTYPE},    // spaces
  {"\\+", TK_PLUS},     // plus
  {"-", TK_SUB},        // sub
  {"\\*", TK_MUL},      // mul or deref
  {"/", TK_DIV},        // div
  {"\\(", TK_LBRACKET}, // left bracket
  {"\\)", TK_RBRACKET}, // right bracket
  {"-? *[0-9]+", TK_DEC},   // decimal
  {"==", TK_EQ},        // equal
  {"!=", TK_NEQ},       // not equal
  
  {"\\$[a-z0-9]+", TK_REG},      // register
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  TK_TYPE type;
  char str[MAX_TOKEN_LEN]; //TODO缓冲区将要溢出的时候, 要进行相应的处理
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        // if(substr_len > MAX_TOKEN_LEN && rules[i].token_type != TK_NOTYPE) {
        //   Log("[WARNING]:The buffer is about to overflow!\n");
        //   return false;
        // }
        // switch (rules[i].token_type) {
        //   case TK_NOTYPE:break;
        //   case TK_DEC:
        //     strncpy(tokens[nr_token].str, substr_start, substr_len);
        //     tokens[nr_token].str[substr_len] = '\0';
        //   default: 
        //     tokens[nr_token].type = rules[i].token_type;
        //     nr_token++;
        // }
        if(rules[i].token_type == TK_NOTYPE) {
          break;
        }
        if(substr_len > MAX_TOKEN_LEN) {
            Log("[WARNING]:The buffer is about to overflow!\n");
            return false;
        }

        tokens[nr_token].type = rules[i].token_type;
        if(rules[i].token_type == TK_MUL && (nr_token == 0 || !SUB_EXP_END(i-1))) {
          tokens[nr_token].type = TK_DEREF;
        }
        if(rules[i].token_type == TK_DEC || rules[i].token_type == TK_HEX || rules[i].token_type == TK_REG){
          strncpy(tokens[nr_token].str, substr_start, substr_len);
          tokens[nr_token].str[substr_len] = '\0';
        }
        nr_token++;
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }
  return true;
}


static bool check_parentheses(int p, int q, bool *const success){
  if(!(tokens[p].type == TK_LBRACKET && tokens[q].type == TK_RBRACKET)){
		return false;
  }
  int cnt = 0;
  bool flag = true;
  for(int i = p + 1; i < q; i++){
    if(tokens[i].type == TK_LBRACKET) cnt++;
    else if(tokens[i].type == TK_RBRACKET) cnt--;
    if(cnt < 0){
      flag = false; // 注意(18-13)/(59-6)的处理过程
      if(cnt < -1){
        *success = false;
        return false;
      }
    }
  }
  if(cnt != 0){
    *success = flag = false;
  }
  return flag;
}

static sword_t eval(int p, int q, bool *const success) {
	if(p > q){
    *success = false;
    return 0;
  }else if(p == q){
    if(tokens[p].type == TK_REG){
			return isa_reg_str2val(tokens[p].str, success);
    }else {
      return (sword_t)strtol(tokens[p].str, NULL, 0);
    }
  }else if(check_parentheses(p, q, success)){
    return eval(p + 1, q - 1, success);
  }else {
    if(*success == false) return 0;
    int op = -1; // the position of 主运算符 in the token expression
    //此时表达式不在一对括号内，当存在不在括号内的token，主运算符一定在这些token中
    int inBracket = 0;
    int lastPLusSub = -1, lastMulDiv = -1, lastEqNeq = -1, lastDeref = -1;
    for(int i = p; i <= q; i++){
      if(tokens[i].type == TK_LBRACKET) inBracket++;
      else if(tokens[i].type == TK_RBRACKET) inBracket--;
      if(inBracket != 0) continue;
      if(tokens[i].type == TK_PLUS || tokens[i].type == TK_SUB){
        lastPLusSub = i;
      }
      else if(tokens[i].type == TK_MUL || tokens[i].type == TK_DIV){
        lastMulDiv = i;
      }
      else if(tokens[i].type == TK_EQ || tokens[i].type == TK_NEQ){
        lastEqNeq = i;
      }
      else if(tokens[i].type == TK_DEREF){
        lastDeref = i;
      }
    }
    op =  lastEqNeq   != -1 ? lastEqNeq :
          lastPLusSub != -1 ? lastPLusSub : 
          lastMulDiv  != -1 ? lastMulDiv : lastDeref;
    if(op == -1){
      *success = false;
      return 0;
    }
    sword_t valLeft = 0;
    if(tokens[op].type != TK_DEREF){
      valLeft = eval(p, op - 1, success);
    }
    sword_t valRight = eval(op + 1, q, success);
    switch(tokens[op].type){
      case TK_PLUS:return valLeft + valRight;
      case TK_SUB:return valLeft - valRight;
      case TK_MUL:return valLeft * valRight;
      case TK_DIV:return valLeft / valRight;
      case TK_EQ: return valLeft == valRight;
			case TK_NEQ: return valLeft != valRight;
			case TK_DEREF: return *((uint64_t *)guest_to_host(valRight));
      default:Assert(0, "[WARNING]The program should not run here!\n");
    }
  }
  return 0;
}

word_t expr(char *e, bool *const success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  uint32_t result = eval(0, nr_token - 1, success);
  if(*success == false){
    Log("[WARNING]:Illegal input!\n");
  }
	// printf("eval: %u\n", result);
	return result;
}
