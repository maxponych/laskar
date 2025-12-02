#include "shell.h"

u8 size[2] = {20, 20};
u8 x_middle;
u8 y_middle;

u8 left_border;
u8 right_border;
u8 bottom_border;
u8 top_border;
u32 color = 0x00FF55FF;
u16 tick = 1500;

u32 snake_capacity = 128;
u8 (*snake)[2] = 0;
u32 snake_length = 8;
u8 end = 0;
u8 apple[2] = {0};

void init_globals() {
  snake = kmalloc(snake_capacity * sizeof(u8[2]));
  x_middle = (screen_width() / 16) / 2;
  y_middle = (screen_height() / 16) / 2;

  left_border = x_middle - (size[0] / 2);
  right_border = x_middle + (size[0] / 2);
  bottom_border = y_middle - (size[1] / 2);
  top_border = y_middle + (size[1] / 2);
}

void grow_capacity() {
  u32 new_capacity = snake_capacity * 2;
  u8(*new_snake)[2] = kmalloc(new_capacity * sizeof(u8[2]));

  for (u32 i = 0; i < snake_capacity; i++) {
    new_snake[i][0] = snake[i][0];
    new_snake[i][1] = snake[i][1];
  }

  kfree(snake);
  snake_capacity = new_capacity;
  snake = new_snake;
}

u16 random() {
  static u16 seed = 0;
  seed ^= poll_pit();
  seed = (seed * 1103515245 + 12345) & 0xFFFF;
  return seed;
}

void set_apple() {
  u8 collision = 1;
  while (collision == 1) {
    apple[0] = (random() % (right_border - left_border - 1)) + left_border + 1;
    apple[1] =
        (random() % (top_border - bottom_border - 1)) + bottom_border + 1;
    collision = 0;
    for (int i = 0; i < snake_length; i++) {
      if (snake[i][0] == apple[0] && snake[i][1] == apple[1]) {
        collision = 1;
        break;
      }
    }
  }
}

void draw_map() {
  for (u8 i = bottom_border; i <= top_border; i++) {
    fillc16x16(right_border, i, color);
    fillc16x16(left_border, i, color);
  }

  for (u8 i = left_border; i <= right_border; i++) {
    fillc16x16(i, top_border, color);
    fillc16x16(i, bottom_border, color);
  }
}

void init_snake(u8 len) {
  for (u8 i = 1; i <= len; i++) {
    fillc16x16(left_border + i, y_middle, 0x0000AA00);
    snake[i - 1][0] = left_border + (len - i);
    snake[i - 1][1] = y_middle;
  }
}

void clear_map() {
  for (u16 y = bottom_border + 1; y < top_border; y++) {
    for (u16 x = left_border + 1; x < right_border; x++) {
      fillc16x16(x, y, 0x00000000);
    }
  }
}

void update_snake(char move, char prev) {
  for (u32 i = snake_length - 1; i > 0; i--) {
    snake[i][0] = snake[i - 1][0];
    snake[i][1] = snake[i - 1][1];
  }

  if (move == 'w' || move == 'W') {
    snake[0][1] = snake[0][1] - 1;
  }
  if (move == 'a' || move == 'A') {
    snake[0][0] = snake[0][0] - 1;
  }
  if (move == 's' || move == 'S') {
    snake[0][1] = snake[0][1] + 1;
  }
  if (move == 'd' || move == 'D') {
    snake[0][0] = snake[0][0] + 1;
  }
  if (snake[0][0] == left_border || snake[0][0] == right_border) {
    end = 1;
  }
  if (snake[0][1] == bottom_border || snake[0][1] == top_border) {
    end = 1;
  }
  for (u8 i = snake_length - 1; i > 0; i--) {
    if (snake[0][0] == snake[i][0] && snake[0][1] == snake[i][1]) {
      end = 1;
    }
  }
  if (snake[0][0] == apple[0] && snake[0][1] == apple[1]) {
    set_apple();
    snake_length++;
  }
  fillc16x16(apple[0], apple[1], 0x00FF5555);
  for (u8 i = 0; i < snake_length; i++) {
    fillc16x16(snake[i][0], snake[i][1], 0x0000AA00);
  }
}

void putnum(u8 n, u8 x, u8 y, u32 color) {
  char buf[4];
  int i = 0;

  if (n == 0) {
    putc('0', x, y, color);
    return;
  }

  while (n > 0) {
    buf[i++] = '0' + (n % 10);
    n /= 10;
  }

  for (int j = i - 1; j >= 0; j--) {
    putc(buf[j], x, y, color);
    x++;
  }
}

void cmd_snake(char *args) {
  init_globals();
  clear_screen();
  draw_map();
  char prev = ' ';
  char move = 'd';
  u64 next = tick_update() + tick;
  snake_length = 8;
  init_snake(snake_length);
  set_apple();

  while (end == 0) {
    u32 t = tick_update();
    u8 scancode = kb_read();
    char key = game_translate(scancode);
    putnum(snake_length, 0, 0, 0x0000AAAA);

    if (snake_length == (size[0] - 2) * (size[1] - 2)) {
      kfree(snake);
      clear_screen();
      set_color(0x0000FF00, 0x00000000);
      println("You won!");
      set_color(0x00FFFFFF, 0x00000000);
      return;
    }

    if (snake_length + 2 >= snake_capacity) {
      grow_capacity();
    }

    if (key > 0) {
      if (key == 'w' || key == 'W') {
        move = key;
      }
      if (key == 'a' || key == 'A') {
        move = key;
      }
      if (key == 's' || key == 'S') {
        move = key;
      }
      if (key == 'd' || key == 'D') {
        move = key;
      }
    }

    if (t >= next) {
      if (((prev == 'w' || prev == 'W') && (move == 's' || move == 'S')) ||
          ((prev == 's' || prev == 'S') && (move == 'w' || move == 'W')) ||
          ((prev == 'a' || prev == 'A') && (move == 'd' || move == 'D')) ||
          ((prev == 'd' || prev == 'D') && (move == 'a' || move == 'A'))) {
        move = prev;
      }
      prev = move;
      clear_map();
      update_snake(move, prev);
      next += tick;
    }
  }
  end = 0;
  kfree(snake);
  clear_screen();
  print("Your score is: ");
  printnum(snake_length);
  printc('\n');
}
