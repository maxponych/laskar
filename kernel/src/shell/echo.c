#include "shell.h"

void parse(char *args, char **content_l, char **content_r, char **name,
           u8 *write) {
  *write = 0;
  *content_l = NULL;
  *content_r = NULL;
  *name = NULL;

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
      if (*(p + 1) == '>') {
        *write = 2;
        p += 2;
      } else {
        *write = 1;
        p++;
      }

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

  if (!content_l) {
    println("Missing opening quote");
    return;
  }
  if (!content_r) {
    println("Missing closing quote");
    return;
  }

  u32 bytes = content_r - content_l;

  if (!write) {
    printlncount(content_l, bytes);
    return;
  }

  if (!name) {
    println("Missing filename after >");
    return;
  }

  char *path = normalize_path(name);

  u32 flags = CREATE | ((write == 2) ? APPEND : TRUNCATE);
  u32 file = fs_open(path, flags);
  kfree(path);

  if (write == 2) {
    content_l--;
    *content_l = '\n';
    bytes++;
  }

  if (file == (u32)-1) {
    println("Failed to create file.");
    return;
  }

  u32 written = fs_write(file, content_l, bytes);
  if (written == (u32)-1) {
    println("Failed to write content.");
    fs_close(file);
    return;
  }

  fs_close(file);
}
