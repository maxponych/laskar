#include "shell.h"

void parse(char *args, char *content, char *name, u8 *write, u32 *bytes) {
  u8 quote = 0;
  u16 contentCnt = 0;
  while (*args) {
    if (*args == '"' && !quote) {
      quote = 1;
      args++;
      continue;
    } else if (*args == '"' && quote) {
      quote = 0;
      args++;
      continue;
    }
    if (quote) {
      content[contentCnt++] = *args;
    }
    if (*args == '>' && !quote) {
      *write = 1;
      args++;
      while (*args == ' ')
        args++;
      break;
    }
    args++;
  }

  *bytes = contentCnt;

  u8 i = 0;
  while (i < 12 && *args && *args != ' ' && *args != '\n' && *args != '\r') {
    name[i++] = *args++;
  }
  name[i] = '\0';

  content[contentCnt] = '\0';
}

void cmd_echo(char *args) {
  char content[128];
  char name[13];
  u8 write = 0;
  u32 bytes = 0;

  parse(args, content, name, &write, &bytes);

  char fatName[11];
  str_to_fat83(name, fatName);

  if (write) {
    fs_write_file(bytes, fatName, (u8 *)content, 0, current_dir);
  } else {
    println(content);
  }
}
