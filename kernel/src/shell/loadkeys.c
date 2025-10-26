#include "shell.h"

void cmd_loadkeys(char *args) {
  if (!args) {
    println("Usage: loadkeys <qwerty|dvorak>");
    return;
  }

  if (str_cmp(args, "qwerty") == 0) {
    set_layout(QWERTY);
    println("Switched to QWERTY layout");
  } else if (str_cmp(args, "dvorak") == 0) {
    set_layout(DVORAK);
    println("Switched to DVORAK layout");
  } else {
    println("Unknown layout");
  }
}
