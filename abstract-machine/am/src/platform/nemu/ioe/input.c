#include <am.h>
#include <nemu.h>

#define KEYDOWN_MASK 0x8000

//参考native的ioe实现, '如何检测多个按键被同时按下'暂未实现
//多个按键被同时持续按下时, 无法检测到非最后一个按键
void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  uint32_t k = inl(KBD_ADDR);
  kbd->keydown = (k & KEYDOWN_MASK ? true : false);
  kbd->keycode = k & ~KEYDOWN_MASK;
}
