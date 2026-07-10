#include "shell.h"

char *cwd = NULL;

void init_cwd() {
  cwd = kmalloc(2);
  if (cwd != NULL) {
    str_cpy(cwd, "/");
  }
}

char *normalize_path(const char *path) {
  if (path == NULL || path[0] == '\0') {
    char *result = kmalloc(str_len(cwd) + 1);
    str_cpy(result, cwd);
    return result;
  }

  u32 base_len =
      (path[0] != '/') ? str_len(cwd) + str_len(path) + 2 : str_len(path) + 1;
  char *temp = kmalloc(base_len);
  if (path[0] != '/') {
    str_cpy(temp, cwd);
    if (temp[str_len(temp) - 1] != '/')
      str_cat(temp, "/");
    str_cat(temp, path);

  } else {
    str_cpy(temp, path);
  }
  char **tokens = kmalloc(64 * sizeof(char *));
  u32 token_cap = 64;
  u32 count = 0;
  char *token = str_tok(temp, "/");
  while (token != NULL) {
    if (str_cmp(token, ".") == 0) {
    } else if (str_cmp(token, "..") == 0) {
      if (count > 0)
        count--;

    } else {
      if (count >= token_cap) {
        token_cap *= 2;
        char **new_tokens = kmalloc(token_cap * sizeof(char *));
        for (u32 i = 0; i < count; i++) {
          new_tokens[i] = tokens[i];
        }
        kfree(tokens);
        tokens = new_tokens;
      }
      tokens[count++] = token;
    }
    token = str_tok(NULL, "/");
  }
  u32 result_len = 1;
  for (u32 i = 0; i < count; i++) {
    result_len += str_len(tokens[i]) + 1;
  }
  char *result = kmalloc(result_len + 1);
  if (count == 0) {
    str_cpy(result, "/");

  } else {
    result[0] = '\0';
    for (u32 i = 0; i < count; i++) {
      str_cat(result, "/");
      str_cat(result, tokens[i]);
    }
  }
  kfree(temp);
  kfree(tokens);
  return result;
}

void cmd_pwd(char *args) { println(cwd); }

void cmd_cd(char *args) {
  char *new_path = normalize_path(args);
  Stat *entry = fs_stat(new_path);
  if (entry == NULL) {
    println("No such directory");
    kfree(new_path);
    return;
  }
  if (!(entry->attr & 0x10)) {
    println("Not a directory");
    kfree(new_path);
    return;
  }

  kfree(cwd);
  cwd = new_path;
}
