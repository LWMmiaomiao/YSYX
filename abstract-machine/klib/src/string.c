#include <stddef.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t length = 0;
	while(*s){
		length++;
		s++;
	}
	return length;
}

char *strcpy(char *dst, const char *src) {
  char *ret = dst;
	while(*src)
		*dst++ = *src++;
	*dst = '\0';
	return ret;
}

char *strncpy(char *dst, const char *src, size_t n) {
  char* ret = dst;
	while (n && (*dst++ = *src++))
		n--;
	if (n) {
		while (--n)
			*dst++ = '\0';
	}
	return ret;
}

char *strcat(char *dst, const char *src) {
  char *ret = dst;
	while(*dst)
		dst++;
	while(*src)
		*dst++ = *src++;
	*dst = '\0';
	return ret;
}

int strcmp(const char *s1, const char *s2) {
  while(*s1 && *s1 == *s2) {
		s1++, s2++;
	}
	return (int)(*s1) - (int)(*s2);
}

int strncmp(const char *s1, const char *s2, size_t n) {
  while(n != 0 && *s1 && *s2 && *s1 == *s2) {
		s1++, s2++, n--;
	}
	return (n == 0) ? 0 : (int)(*s1) - (int)(*s2);
}

void *memset(void *s, int c, size_t n) {
  uint8_t *p = s;
  while(n--)
		*p++ = (uint8_t)c;
	return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  uint8_t *d = dst;
  const uint8_t *s = src;
  if (d < s) {
      for (size_t i = 0; i < n; i++) {
          d[i] = s[i];
      }
  } else if (d > s) {
      for (size_t i = n; i > 0; i--) {
          d[i - 1] = s[i - 1];
      }
  }
  return dst;
}

// void *memcpy(void *out, const void *in, size_t n) {
//   uint32_t *pdst = (uint32_t *)out, *psrc = (uint32_t *)in;
//   uint8_t *tmp1 = NULL, *tmp2 = NULL;
//   size_t c1 = n/4, c2 = n%4;
//   if (pdst > psrc && (uint8_t *)pdst < (uint8_t *)psrc + n){
//     tmp1 = (uint8_t *)pdst + n - 1;
//     tmp2 = (uint8_t *)psrc + n - 1;
//     while(c2--)
//       *tmp1-- = *tmp2--;
//     tmp1++, tmp2++;
//     pdst = (uint32_t *)tmp1;
//     psrc = (uint32_t *)tmp2;
//     pdst--, psrc--;
//     while (c1--)
//       *pdst-- = *psrc--;
//   }
//   else{
//     while(c1--)
//       *pdst++ = *psrc++;
//     tmp1 = (uint8_t *)pdst;
//     tmp2 = (uint8_t *)psrc;
//     while(c2--)
//       *tmp1++ = *tmp2++;
//   }
//   return out;
// }

void *memcpy(void *out, const void *in, size_t n) {
  char *d = out;
  const char *s = in;

  size_t align = (8 - ((uintptr_t)d % 8)) % 8;
  while (align-- && n--) {
      *d++ = *s++;
  }

  uint64_t *d64 = (uint64_t*)d;
  const uint64_t *s64 = (const uint64_t*)s;
  while (n >= 8) {
      *d64++ = *s64++;
      n -= 8;
  }

  d = (char*)d64;
  s = (const char*)s64;
  while (n--) {
      *d++ = *s++;
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
	const uint8_t *p1 = s1, *p2 = s2;
	while(n != 0 && *p1 && *p1 == *p2) {
		p1++, p2++;
		n--;
	}
	return (n == 0) ? 0 : (int)(*p1) - (int)(*p2);
}

#endif
