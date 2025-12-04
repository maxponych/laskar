#include "shell.h"

void parse(char *args, char **content_l, char **content_r, char **name,
           u8 *write) {
  *write = 0;
  *content_l = 0;
  *content_r = 0;
  char *p = args;

  u8 quote = 0;
  while (*p) {
    if (*p == '"' && !quote) {
      quote = 1;
      *content_l = p + 1;
    } else if (*p == '"' && quote) {
      quote = 0;
      *content_r = p;
    }

    if (*p == '>' && !quote) {
      *write = 1;
      p++;
      while (*p == ' ')
        p++;
      *name = p;
      break;
    }
    p++;
  }

  if (*name) {
    char *n = *name;
    while (*n && *n != ' ' && *n != '\n' && *n != '\r')
      n++;
    *n = '\0';
  }
}

void cmd_echo(char *args) {
  char *content_l, *content_r, *name;
  u8 write;

  parse(args, &content_l, &content_r, &name, &write);
  u32 bytes = content_r - content_l;

  if (!write) {
    printlncount(content_l, bytes);
    return;
  }

  static char fatName[11];
  str_to_fat83(name, fatName);
  fs_write_file(bytes, fatName, (u8 *)content_l, 0, current_dir);
}
