#include "types.h"

char to_lower(char c);
void fat83_to_str(const char name[11], char *out);
void str_to_fat83(const char *name, char out[11]);

void memcopy(void *buff1, void *buff2, u32 cnt);

char *str_cpy(char *dest, const char *src);
char *str_cat(char *dest, const char *src);
i32 str_cmp(const char *s1, const char *s2);
char *str_tok(char *str, const char *delim);
u32 str_len(const char *str);
