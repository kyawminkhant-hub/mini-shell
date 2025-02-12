/************************************************************************************
*
*	file	: main.c 
*
*	author  : kmk
*
*	date	: 9 Feb 2025
*
*	brief	: mini-shell (msh)
*
*	ref	: based on Stephen Brennan's LSH
*
*************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MSH_RL_BUFSIZE 1024       
#define MSH_TOK_BUFSIZE 64        
#define MSH_TOK_DELIM " \t\r\n\a" 
#define MSH_EXS_BUFSIZE 4		      
#define MSH_PID_BUFSIZE 8		     

/*
 * Process Info
 */
int exit_status = 0; 
pid_t msh_pid;

/* 
 * Function Declarations for builtin shell commands
 */
int msh_cd(char **args);
int msh_help(char **args);
int msh_exit(char **args);

/* 
 * Function Declarations for special variables substitutions 
 */
char *sub_exit_status(void);
char *sub_msh_pid(void);

/* 
 * Special variables list 
 */
char *keywords[] = {
  "$?",
  "$$"
};

int msh_num_keywords() {
  return sizeof(keywords) / sizeof(char *); 
}

/*
 * Special variables substitutions 
 */
char* (*msh_sub[]) (void) = {
  &sub_exit_status,
  &sub_msh_pid
};

/*
 * Substitute "$?" -> exit status of last command
 * return: string value of exist_status
 */
char *sub_exit_status() 
{
  int bufsize = MSH_EXS_BUFSIZE;
  char *exit_status_str = malloc(bufsize * sizeof(char));
  
  if (sprintf(exit_status_str, "%d", exit_status) < 0)   {
    perror("msh");
  }
  
  return exit_status_str;
}

/*
 * Substitue "$$" -> shell process ID
 * return: string value of msh_pid
 */
char *sub_msh_pid() 
{
  int bufsize = MSH_PID_BUFSIZE;
  char *msh_pid_str = malloc(bufsize * sizeof(char));

  if (sprintf(msh_pid_str, "%d", msh_pid) < 0)   {
    perror("msh");
  }

  return msh_pid_str;
}

/* 
 * Builtin commands list
 */
char *builtin_str[] = {
  "cd",
  "help",
  "exit"
};

int (*builtin_fun[]) (char **) = {
  &msh_cd,
  &msh_help,
  &msh_exit
};

int msh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/* 
 * Builtin function implementations 
 */

/*
 * Builtin command: cd
 * param: args List of args.  args[0] is "cd".  args[1] is the directory.
 * return: Always returns 1, to continue executing.
 */ 
int msh_cd(char **args)
{
  char *path;

  /*
  * If no argument is provided, change directory to $HOME.
  * If more than one argument is provided, error: too many arguments.
  * Otherwise,change directory to path specified as second argument.
  */
  if (args[1] == NULL) {
    path = getenv("HOME");
  }
  else if (args[2] != NULL) {
    fprintf(stderr, "msh: too many arguments to \"cd\"\n");
    exit_status = EXIT_FAILURE;
  } 
  else {
    path = args[1];
  }
  
  if (chdir(path) != 0) {
    perror("msh");
  }
  else {
    exit_status = EXIT_SUCCESS;
  }

  return 1;
}

/* 
 * Builtin command: help
 * return: Always returns 1, to continue executing.
 */
int msh_help(char **args)
{
  int i;

  printf("Mini-Shell based on Stephen Brennan's LSH\n");
  printf("Usage: <command> <arguments> \n");
  printf("The following are built in:\n");

  printf("Builtin commands: \n");
  for (i = 0; i < msh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Special variables: \n");
  for (i = 0; i < msh_num_keywords(); i++) {
    printf("  %s\n", keywords[i]);
  }

  printf("Use the \'man\' command for information on external commands.\n");
  exit_status = EXIT_SUCCESS;
  
  return 1;
}

/*
 * Builtin command: exit
 * return: Always returns 0, to terminate execution.
 */
int msh_exit(char **args)
{
  return 0;
}

/*
 * Read a line of input from stdin.
 * return: a pointer to the buffer containing the input line.
 */
char *msh_read_line(void)
{
  char *line = NULL;
  ssize_t bufsize = 0; 
  
  if (getline(&line, &bufsize, stdin) == -1) {
    /* feof() returns non-zero value [true] if reached EOF. */
    if (feof(stdin)) {        
      exit(EXIT_SUCCESS); 
    }
    else {
      perror("readlie");
      exit(EXIT_FAILURE);
    }
  } 

  return line; 
}

/*
 * Split a line into tokens (tokens mean splitted strings).
 * param: line pointer.
 * return: a double pointer to Null-termintated array of tokens.
 */
char **msh_split_line(char *line)
{
  int bufsize = MSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;
  int i;
  int flag = 0;

  if (!tokens) {
    fprintf(stderr, "msh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, MSH_TOK_DELIM);
  while (token != NULL) {
    /* 
     * if a spacial variable keyword is found in token, substitute the current token. 
     */
    for (i=0; i<msh_num_keywords(); i++) {
      if (strcmp(token, keywords[i]) == 0) {
        token = (*msh_sub[i])();
      }
    } 
    tokens[position] = token;  
    position++;
    
    if (position >= bufsize) {
      bufsize += MSH_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "msh: allocation error\n");
	      exit(EXIT_FAILURE);
      }
    }
    
    token = strtok(NULL, MSH_TOK_DELIM);
  }

  tokens[position] = NULL;
  return tokens;
}

/* 
 * Launch an external command (other than builtin commands) and wait for it to terminate.
 * param: args Null-terminated list of arguments (including command name).
 * return: Always return 1, to continue execution.
 */
int msh_launch(char **args) 
{
  pid_t pid, wpid;
  int status; 

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("msh");
    }
    exit(EXIT_FAILURE); 
  }
  else if (pid < 0) {
    // Error forking
    perror("msh");
  }
  else {
    // Parent process
    
    /* 
     * Wait until a child process terminated normally and terminated signal was caught.
     * pid argument: specifies a set of child processes.
     * status: exit status of child process.
     * WUNTRACED: the status of any child processes specified by pid that are stopped.
     */
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status)); 

    exit_status = WEXITSTATUS(status);
  }
 
  return 1;
}

/* 
 * Execute shell builtin or external commands.
 * param: args Null-terminated list of arguments (including command name).
 * return: 1 if the shell should continue running, 0 if it should terminate.
 */
int msh_execute(char **args) 
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i=0; i<msh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_fun[i])(args);
    }
  }

  return msh_launch(args);
}

/*
 * Release memory allocated after each command execution
 */
void clean_up(char **args) 
{
  int i;

  for (i = 0; args[i] != NULL; i++) {
    free(args[i]);
  }
  free(args);
}

/*
 * Shell Prompt 
 */
void msh_prompt(void) 
{
  char *homedir = getenv("HOME");        
  size_t homedir_len  = strlen(homedir); 
  char *cwd = getcwd(NULL, 0);           
  int pos;

  if (cwd == NULL) {
    perror("getcwd");
  }
  
  /* Replace the home directory with '~' in stdout */
  if (strncmp(cwd, homedir, homedir_len) == 0) { 
    // skip the home directory and point to the next
    printf("~%s\n", cwd + homedir_len);  
  }
  else {
    printf("%s\n", cwd);
  }

  printf("→ ");
  free(cwd);
}

/*
 * Shell loop
 */  
void msh_loop(void)
{
   char *line;
   char **args;
   int status;

   do {	   
     printf("\n");
     
     msh_prompt(); 
     line = msh_read_line();
     args = msh_split_line(line);
     status = msh_execute(args);

     clean_up(args);
   } while (status);
   /* if status is 1 (true), shell should continue running. */
}

int main (int argc, char **argv)
{
   // Load config files, if any.
   msh_pid = getpid();

   // Print art and info.
   printf(
          "           _       _               _          _ _      \n"
          " _ __ ___ (_)_ __ (_)          ___| |__   ___| | |     \n"
          "| '_ ` _ \\| | '_ \\| |  _____  / __| '_ \\ / _ \\ | | \n"
          "| | | | | | | | | | | |_____| \\__ \\ | | |  __/ | |   \n"
          "|_| |_| |_|_|_| |_|_|         |___/_| |_|\\___|_|_|    \n"
	  "                                                       \n"
	 ); 
   printf("mini-shell 1.0 on linux.\n");
   printf("Type \"help\" for information.\n");

   // Run command loop.
   msh_loop();

   // Peform any shutdown/cleanup.
   
   return EXIT_SUCCESS;
}
