
# Mini-shell

Mini-shell is a basic shell implementation in C based on [Stephen Brennan's LSH][i]. This project is built to understand the inner workings of a shell, improve pointer manipulation skills, and explore system calls in depth. It includes basic shell functions: **a shell prompt**, **special variables**, **parsing arguments**, **executing built-in commands** and **external commands** while it's still missing many features including multi-line commands, quoting arguments, signal handling, job control, piping and so on.

## Run on Linux

* Compile `gcc main.c -o msh`
* Run `./msh`

## References

* [Stephen Brennan's LSH][i]
* [EMERTXE: Embedded Linux Minishell Project][j]
* [man pages][m]


[i]: https://brennan.io/2015/01/16/write-a-shell-in-c/

[j]: https://www.emertxe.com/embedded-systems/embedded-linux-on-arm/elarm-projects/embedded-linux-minishell-project/

[m]: https://man7.org/linux/man-pages/