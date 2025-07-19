#include <am.h>
#include <dlfcn.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>
#include <stdio.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
//目前只实现%s和%d

size_t my_itoa(int num, char* str, int radix, int uppercase, int prefix) {
	static const char *digits_lower = "0123456789abcdef";
    static const char *digits_upper = "0123456789ABCDEF";
    if(num == 0){
        str[0] = '0', str[1] = '\0';
        return 1;
    }
    const char *digits_arr = uppercase ? digits_upper : digits_lower;
    int i = 0;
    int digit;
    unsigned int num_abs = num;
    int start_pos = 0; //数字起始位置
    char str_temp[40] = {0};
    if (num < 0) {
        str[start_pos++] = '-';
        num_abs = -(long long)num; //考虑num为INT_MIN
    }
    if(radix == 16 && prefix){
        str[start_pos++] = '0', str[start_pos++] = uppercase ? 'X' : 'x';
    }
    else if(radix == 8 && prefix){
        str[start_pos++] = '0';
    }else if(radix == 2 && prefix){
        str[start_pos++] = '0', str[start_pos++] = uppercase ? 'B' : 'b';
    }
    while(num_abs != 0) {
        digit = num_abs % radix;
        str_temp[i++] = digits_arr[digit];
        num_abs = num_abs / radix;
    }
    for (int j = 0; j < i; j++) {
        str[start_pos + j] = str_temp[i - 1 - j];
    }
    str[start_pos + i] = '\0';
	return start_pos + i;
}

extern void putch(char ch);
int printf(const char *fmt, ...) {
	char buffer[1024];
	va_list arg;
	va_start(arg, fmt);
  
	int ret = vsnprintf(buffer, sizeof(buffer), fmt, arg);  // 将格式化的内容保存在buffer中
	char *tmp = buffer;
    while (*tmp != 0) {
        putch(*tmp);
        tmp++;
    }
  
	va_end(arg);
	return ret;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
	return vsnprintf(out, -1, fmt, ap);
}

int sprintf(char *out, const char *fmt, ...) {
	va_list ap;
	va_start(ap,fmt);
	int ret = vsprintf(out,fmt,ap);
	va_end(ap);
	return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
	va_list ap;
	va_start(ap,fmt);
	int ret = vsnprintf(out,n,fmt,ap);
	va_end(ap);
	return ret;
}

// n取0时不写入, 但计算将要写入字符数
int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
	size_t pos = 0;      // 当前写入位置
    int len = 0;         // 总字符数
    const char *p = fmt; // 当前扫描格式串位置
    
    while (*p) {
        if (*p != '%') {
            len++;
            if (n > 0 && pos < n - 1) {
                out[pos++] = *p;
            }
            p++;
            continue;
        }
        
        p++; // 跳过 '%'
        int left_align = 0, zero_pad = 0, print_sign = 0, space_sign = 0, special_fmt = 0;
        int width = 0;
        int precision = -1; // -1 表示未指定精度
        
        // 解析标志
        while (*p == '-' || *p == '0' || *p == '+' || *p == ' ' || *p == '#') {
            if (*p == '-') left_align = 1; 	//左对齐
            if (*p == '0') zero_pad = 1;   	//零填充
			if (*p == '+') print_sign = 1; 	//显示符号
			if (*p == ' ') space_sign = 1; 	//正数空格负数负号
			if (*p == '#') special_fmt = 1; //特殊格式
            p++; //后三个尚未完全实现
        }

        // 解析宽度（数字或 *）
        if (*p == '*') {
            width = va_arg(ap, int);
            if (width < 0) {
                left_align = 1;
                width = -width;
            }
            p++;
        } else if (*p <= '9' && *p >= '0') {
            while (*p <= '9' && *p >= '0') {
                width = width * 10 + (*p - '0');
                p++;
            }
        }

		if(left_align){
			zero_pad = 0;
		}
        if(print_sign){
            space_sign = 0;
        }
        
        // 解析精度（. 后跟数字或 *）
        if (*p == '.') {
            p++;
            if (*p == '*') {
                precision = va_arg(ap, int);
                p++;
                if (precision < 0) precision = -1; // 负精度视为未指定
            } else {
                precision = 0;
                while (*p <= '9' && *p >= '0') {
                    precision = precision * 10 + (*p - '0');
                    p++;
                }
            }
        }
        
        // 处理转换说明符
        char spec = *p++;
        int base = 10;
        int uppercase_hex = 0;
        int negative = 0;
        unsigned long long num = 0;
        char num_buf[32]; // 数字转换缓冲区
        int digits = 0;   // 数字位数
        const char *str = NULL;
        size_t str_len = 0;
        
        switch (spec) {
            case 'd': case 'i': {
                int arg = va_arg(ap, int);
                if (arg < 0) {
                    negative = 1;
                    num = (unsigned long long)(-(long long)arg);
                } else {
                    num = (unsigned long long)arg;
                }
                base = 10;
                break;
            }
            case 'u':
                num = va_arg(ap, unsigned int);
                base = 10;
                break;
            case 'x': case 'X':
                num = va_arg(ap, unsigned int);
                base = 16;
                uppercase_hex = (spec == 'X');
                break;
            case 'c': {
                char c = (char)va_arg(ap, int);
                num_buf[0] = c;
                digits = 1;
                break;
            }
            case 's':
                str = va_arg(ap, const char *);
                if (!str) str = "(null)";
                str_len = strlen(str);
                if (precision >= 0 && precision < str_len) {
                    str_len = precision;
                }
                break;
            case '%':
                num_buf[0] = '%';
                digits = 1;
                break;
            default:
                // 无效说明符：原样输出
                num_buf[0] = '%';
                num_buf[1] = spec;
                digits = 2;
                break;
        }
        
        // 处理数字转换（除字符串外）
        if (spec != 's') {
            if (spec != 'c' && spec != '%') {
                digits = my_itoa(num, num_buf, base, uppercase_hex, special_fmt);
            }
            
            // 处理精度和零值
            int num_width = digits;
            if (precision > digits) {
                num_width = precision;
            } else if (precision == 0 && num == 0 && spec != 'c') {
                num_width = 0; // 零值且精度为0：不输出数字
            }
            
            // 计算字段总长度（含符号）
            int sign_len = negative ? 1 : 0;
            int content_len = sign_len + num_width;
            int pad_len = (width > content_len) ? width - content_len : 0;
            
            // 输出右对齐填充（空格）
            if (!left_align && !zero_pad) {
                for (int i = 0; i < pad_len; i++) {
                    len++;
                    if (n > 0 && pos < n - 1) out[pos++] = ' ';
                }
            }
            
            // 输出符号
            if (n > 0 && pos < n - 1){
                len++;
                if (negative) {
                    out[pos++] = '-';
                }
                else if(print_sign) {
                    out[pos++] = '+';
                }
                else if(space_sign){
                    out[pos++] = ' ';
                }
            }
            
            // 输出零填充（或左对齐的零）
            if (!left_align && zero_pad) {
                for (int i = 0; i < pad_len; i++) {
                    len++;
                    if (n > 0 && pos < n - 1) out[pos++] = '0';
                }
            }
            
            // 输出精度补零
            for (int i = digits; i < num_width; i++) {
                len++;
                if (n > 0 && pos < n - 1) out[pos++] = '0';
            }
            
            // 输出数字
            for (int i = 0; i < digits; i++) {
                len++;
                if (n > 0 && pos < n - 1) out[pos++] = num_buf[i];
            }
            
            // 输出左对齐填充
            if (left_align) {
                for (int i = 0; i < pad_len; i++) {
                    len++;
                    if (n > 0 && pos < n - 1) out[pos++] = ' ';
                }
            }
        } else {
            // 处理字符串
            int pad_len = (width > str_len) ? width - str_len : 0;
            
            // 右对齐填充
            if (!left_align) {
                for (int i = 0; i < pad_len; i++) {
                    len++;
                    if (n > 0 && pos < n - 1) out[pos++] = ' ';
                }
            }
            
            // 输出字符串
            for (int i = 0; i < str_len; i++) {
                len++;
                if (n > 0 && pos < n - 1) out[pos++] = str[i];
            }
            
            // 左对齐填充
            if (left_align) {
                for (int i = 0; i < pad_len; i++) {
                    len++;
                    if (n > 0 && pos < n - 1) out[pos++] = ' ';
                }
            }
        }
    }
    
    // 添加结尾的 '\0'
    if (n > 0) {
        out[pos] = '\0';
    }
    return len;
}



#endif
