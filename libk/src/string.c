#include "string.h"

char *str_cpy(char *dest, const char *src) {
  char *orig_dest = dest;
  while ((*dest++ = *src++)) {
  }
  return orig_dest;
}

char *str_cat(char *dest, const char *src) {
  char *orig_dest = dest;
  while (*dest)
    dest++;
  while ((*dest++ = *src++)) {
  }
  return orig_dest;
}

u32 str_len(const char *str) {
  u32 len = 0;
  while (str[len])
    len++;
  return len;
}

i32 str_cmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(u8 *)s1 - *(u8 *)s2;
}

char *str_chr(const char *str, i32 c) {
  while (*str) {
    if (*str == (char)c) {
      return (char *)str;
    }
    str++;
  }
  return (*str == (char)c) ? (char *)str : NULL;
}

char *str_tok(char *str, const char *delim) {
  static char *next = NULL;

  if (str != NULL) {
    next = str;
  }

  if (next == NULL) {
    return NULL;
  }

  while (*next && str_chr(delim, *next)) {
    next++;
  }

  if (*next == '\0') {
    next = NULL;
    return NULL;
  }

  char *token_start = next;

  while (*next && !str_chr(delim, *next)) {
    next++;
  }

  if (*next) {
    *next = '\0';
    next++;
  } else {
    next = NULL;
  }

  return token_start;
}

char to_lower(char c) {
  if (c >= 'A' && c <= 'Z')
    return c + ('a' - 'A');
  return c;
}

void fat83_to_str(const char name[11], char *out) {
  u8 i, j = 0;

  for (i = 0; i < 8; i++) {
    if (name[i] == ' ')
      break;
    out[j++] = name[i];
  }

  for (i = 8; i < 11; i++) {
    if (name[i] != ' ') {
      if (j > 0)
        out[j++] = '.';
      break;
    }
  }

  for (; i < 11; i++) {
    if (name[i] != ' ')
      out[j++] = name[i];
  }

  out[j] = '\0';
}

void str_to_fat83(const char *name, char out[11]) {
  u8 i = 0, j = 0;

  for (i = 0; i < 11; i++)
    out[i] = ' ';

  if (name[0] == '.' && name[1] == '.') {
    out[0] = '.';
    out[1] = '.';
    return;
  }

  i = 0;
  while (name[i] && name[i] != '.' && j < 8) {
    char c = name[i++];
    if (c >= 'a' && c <= 'z')
      c -= 32;
    out[j++] = c;
  }

  if (name[i] == '.')
    i++;

  j = 8;
  u8 k = 0;
  while (name[i] && k < 3) {
    char c = name[i++];
    if (c >= 'a' && c <= 'z')
      c -= 32;
    out[j++] = c;
    k++;
  }
}

void memcopy(void *buff1, void *buff2, u32 cnt) {
  u8 *dst = (u8 *)buff1;
  u8 *src = (u8 *)buff2;

  while (cnt >= sizeof(u64)) {
    *(u64 *)dst = *(u64 *)src;
    dst += sizeof(u64);
    src += sizeof(u64);
    cnt -= sizeof(u64);
  }

  while (cnt--) {
    *dst++ = *src++;
  }
}
