#include "string.h"

i32 str_cmp(char *s1, char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return (u8)*s1 - (u8)*s2;
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
