# Simple Shell

This simple shell is a lightweight command-line interpreter designed to execute commands and provide essential features found in standard shells.

## Features

### 1. **Command Execution**
Enter any command followed by its arguments, and the shell will execute it.

### 2. **Output Redirection**
Redirect the output of a command to a file:
```
% ls -l > FILES.txt
```

The above command will save the output of `ls -l` into `FILES.txt`.

### 3. **Background Jobs**
Run commands in the background, allowing the shell to immediately accept new commands:
```
% sleep 10 &
```

This will run the `sleep 10` command in the background.

### 4. **Pipes**
Send the output of one command as input to another:
```
% ls | wc
```

The output of `ls` is piped to `wc`, which then counts lines, words, and characters.

### 5. **Interrupt Command**
While a command is running, you can interrupt it using:
```
% ^C
```

## Usage

1. **Starting the Shell**
   - After building the project using the Makefile, use the `check` target to start the shell:
   ```
   $ make check
   ```
2. **Executing Commands**
- Simply type in the command and press enter.

3. **Exiting the Shell**
- Use the `exit` command or `Ctrl+D`.

## Technical Details

- The shell uses `fork` to create child processes for command execution.
- It uses `execvp` to replace the child process with the desired command.
- Piping is achieved using UNIX pipes.
- Signal handling for `^C` interrupts is implemented using a custom signal handler for `SIGINT`.

## Troubleshooting & Help

- If encountering issues with command execution, ensure there are spaces between each operator (word) and neighboring word.
- For more details on system calls or library functions, refer to the UNIX man pages: `man open`, `man close`, `man pipe`, `man dup`, `man dup2`, `man perror`, `man kill`, etc.

