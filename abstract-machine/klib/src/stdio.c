#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
//目前只实现%s和%d
void itoa(int num, char* str, int radix)
{
    int i = 0;
    int sum;
    unsigned int num1 = num;
    int start_pos = 0;
    char str1[33] = {0};
    if (num < 0 && radix == 10) {
        str[i++] = '-';
        start_pos = 1;
        num1 = -(long long)num; //考虑num为INT_MIN？
    }
    if (num == 0) {             
        str1[i++] = '0';
    }
    while(num1 != 0) {
        sum = num1 % radix;
        str1[i++] = (sum > 9) ? (sum - 10) + 'a' : sum + '0';
        num1 = num1 / radix;
    }
    for (int j = 0; j < i; j++) {
        str[start_pos + j] = str1[i - 1 - j];
    }
    str[start_pos + i] = '\0';
}

int printf(const char *fmt, ...) {
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap)
{
	char *start = out;
	while(*fmt){
		if(*fmt == '%'){
			fmt++;
			switch(*fmt){
				case 's':{
					char *s = va_arg(ap, char *);
					strcpy(out, s);
					out += strlen(s);
					fmt++;
					break;
				}
				case 'd':{
					int num = va_arg(ap, int);
          			char temp[64];
          			itoa(num, temp, 10);
					strcpy(out, temp);
					out += strlen(temp);
          
					fmt++;
					break;
				}
				default:
					panic("Not implemented");
			}
		}
		else{
			*out = *fmt;
			out++; fmt++;
		}
	}
	*out = '\0';
	return strlen(start);
}

int sprintf(char *out, const char *fmt, ...) {
	va_list ap;
	va_start(ap,fmt);
	int ret = vsprintf(out,fmt,ap);
	va_end(ap);
	return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}



#endif
