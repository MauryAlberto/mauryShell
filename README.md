# mauryShell

A Unix shell written from scratch in C. Implements the core fork/exec/wait process model, manual PATH resolution, a quote-aware command parser, and a small set of built-in commands.

## Features

### Process execution
- Forks child processes with `fork()` and executes external programs via `execvp()`.
- Parent waits on child completion with `waitpid()` and properly handles `fork()` and `execvp()` failures.
- Cleans up dynamically allocated argument memory in both success and error paths.

### PATH resolution
- Parses the `$PATH` environment variable and walks each directory in order.
- Verifies executable permissions with `access(path, X_OK)` before invoking a child.
- Reports `command not found` when no matching executable is located in any PATH directory.
- Copies the PATH string before tokenizing with `strtok()` to avoid mutating environment memory.

### Command parsing
- Custom tokenizer (`parseNextSQ`) that handles single-quoted strings and unquoted words.
- Tracks whitespace context between tokens so that adjacent quoted strings concatenate correctly (e.g. `'hello''world'` → `helloworld`) while space-separated tokens become distinct arguments.
- Handles edge cases such as empty quoted strings (`''`) and leading/trailing whitespace.
- Manual `malloc`/`free` memory management for every parsed token.

### Built-in commands
| Command | Description |
| --- | --- |
| `exit` | Terminates the shell. |
| `echo <args>` | Prints its arguments, respecting single-quote tokenization. |
| `pwd` | Prints the current working directory via `getcwd()`. |
| `cd <path>` | Changes directory via `chdir()`. Supports `~` expansion using `getenv("HOME")`. |
| `type <command>` | Reports whether a command is a shell built-in or resolves to a path on disk. |

The `type` built-in walks `$PATH` using the same resolution logic as external command execution and reports the full path of the first matching executable.

## Build

Requires `gcc` and a POSIX-compliant Linux environment.

```bash
gcc -Wall -Wextra -o mauryShell mauryShell.c
./mauryShell
```

## Example session

```
mauryShell $: pwd
/home/maury/projects/mauryShell

mauryShell $: type echo
echo is a shell builtin

mauryShell $: type ls
ls is /usr/bin/ls

mauryShell $: echo 'hello world'
hello world

mauryShell $: ls -la
total 48
drwxr-xr-x  2 maury maury  4096 ...
...

mauryShell $: cd ~
mauryShell $: pwd
/home/maury

mauryShell $: exit
```

## Implementation notes

- **stdout flushing.** `setbuf(stdout, NULL)` disables stdout buffering so that the prompt always appears immediately, even when stdout is not a terminal.
- **Tokenizer design.** The parser advances a single cursor pointer through the input buffer, returning one token per call. It signals quoted vs. unquoted tokens and reports whether leading whitespace was consumed, allowing the caller (echo, external command execution) to decide whether tokens should be concatenated or treated as separate arguments.
- **Argument array construction.** External commands are invoked via `execvp(command, args)` where `args` is a `NULL`-terminated array. Tokens are accumulated into a buffer until a whitespace boundary is hit, at which point the buffer is `strdup`-ed into the next slot of `args`.
- **Memory ownership.** Tokens returned from the parser are heap-allocated and freed by the caller. After `execvp()` succeeds the child image is replaced, so the parent is responsible for freeing all argument allocations after `waitpid()` returns.

## Limitations / not yet implemented

This shell intentionally implements a focused subset of shell functionality. The following features are not supported:

- Pipes (`|`)
- I/O redirection (`>`, `<`, `>>`)
- Background processes (`&`) and job control (`fg`, `bg`, `jobs`)
- Signal handling (Ctrl+C, Ctrl+Z) for foreground children
- Double-quoted strings and escape sequences
- Environment variable expansion (`$VAR`)
- Command substitution (`` `cmd` `` or `$(cmd)`)
- Command history and line editing (arrow keys, tab completion)
- Scripting features (variables, control flow, functions)

## File layout

```
mauryShell.c    Single-file implementation
README.md       This file
```
