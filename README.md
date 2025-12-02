# Laskar

Laskar is a learning project of mine. The MVP goal is a more-or-less usable OS with a terminal-first UI, featuring user space, memory allocation, and a scheduler. Currently, the project uses UEFI.

## **Current Features:**

### Drivers 
Drivers for ATA PIO and keyboard with scancode translation.

### FAT32
Old, stable, simple filesystem. Used for simplicity, compatibility, and functionality.

### Ring 0 Shell
A basic but functional shell that parses and executes commands.

**Available commands:**
- `ls` - list files and directories
- `cat` - read file contents
- `cd` - change current working directory
- `clear` - clear the screen
- `echo` - output text to terminal. Supports redirection (`>`). Example: `echo "hello" > hello.txt`
- `loadkeys` - change keyboard layout (dvorak/qwerty available)
- `mkdir` - create a new directory
- `free` - show memory statistic
- `snake` - play Snake!

## Current Limitations

- Ring 0 only (no user space yet)
- No scheduler
- No hardware interrupts
- Missing commands (e.g., `rm`)

## License

Licensed under the [MIT License](./LICENSE).
