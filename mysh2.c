#include <fcntl.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_COMMAND_LENGTH 500
#define MAX_ARGS 100

void executeCommand(char *argv[]);
void execute_redirection(char *file, char *args[], int key, int stdin,
                         int stdout);
void execute_pipe_command(char *args1[], char *args2[]);
void expand_wildcards(char *arg, char **argv, int *argc);
void pipe_and_redirect(char *args[], char *input_file, char *output_file,
                       char *args_pipe[]);
void parse_and_execute(char *command, int saved_stdin, int saved_stdout);
void batch_mode(const char *filename);
void interactive_mode();

void batch_mode(const char *filename) {
  char command[MAX_COMMAND_LENGTH];
  FILE *file = fopen(filename, "r");
  if (!file) {
    perror("open file error");
    exit(EXIT_FAILURE);
  }
  while (fgets(command, MAX_COMMAND_LENGTH, file)) {
    command[strcspn(command, "\n")] = 0;
    parse_and_execute(command, STDIN_FILENO, STDOUT_FILENO);
  }

  fclose(file);
}

void execute_pipe_command(char *args1[], char *args2[]) {
  int pipe_fds[2];
  pid_t pid1, pid2;

  if (pipe(pipe_fds) == -1) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }

  pid1 = fork();
  if (pid1 == 0) {
    close(pipe_fds[0]);
    dup2(pipe_fds[1], STDOUT_FILENO);  
    close(pipe_fds[1]);

    execvp(args1[0], args1);
    perror("execvp");
    exit(EXIT_FAILURE);
  }

  pid2 = fork();
  if (pid2 == 0) {
    close(pipe_fds[1]);
    dup2(
        pipe_fds[0],
        STDIN_FILENO); 
    close(pipe_fds[0]);

    execvp(args2[0], args2);
    perror("execvp");
    exit(EXIT_FAILURE);
  }

  close(pipe_fds[0]);
  close(pipe_fds[1]);
  waitpid(pid1, NULL, 0);
  waitpid(pid2, NULL, 0);
}

void executeCommand(char *argv[]) {
  static int success = 0;
  if (strcmp(argv[0], "then") == 0) {
    for (int i = 0; i < MAX_ARGS - 10; i++) {
      argv[i] = argv[i + 1];
    }

    if (success == -1) {
      printf("Error: Last command was failed\n");
      return;
    }
  }

  if (strcmp(argv[0], "else") == 0) {
    for (int i = 0; i < MAX_ARGS - 1; i++) {
      argv[i] = argv[i + 1];
    }

    if (success == 1) {
      printf("Error: Last command was not failed\n");
      return;
    }
  }

  if (strcmp(argv[0], "cd") == 0) {
    // cd command
    if (argv[1] == NULL || argv[2] != NULL) {
      fprintf(stderr, "cd: Error Number of Parameters\n");
      success = -1;
    } else if (chdir(argv[1]) == 0)
      success = 1;
    else {
      success = -1;
      perror("cd");
    }
  } else if (strcmp(argv[0], "pwd") == 0) {
    // pwd command
    char cwd[MAX_COMMAND_LENGTH];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
      printf("%s\n", cwd);
      success = 1;
    } else {
      perror("pwd");
      success = -1;
    }
  } else if (strcmp(argv[0], "which") == 0) {
    if (argv[1] == NULL) {
      success = -1;
      return;
    }

    char *path = malloc(15 + strlen(argv[1]) + 1);
    int found = 0;

    strcpy(path, "/usr/local/bin/");
    strcat(path, argv[1]);

    if (access(path, F_OK) == 0) {
      printf("%s\n", path);
      found = 1;
      success = 1;
    }

    strcpy(path, "/usr/bin/");
    strcat(path, argv[1]);

    if (access(path, F_OK) == 0 && found == 0) {
      printf("%s\n", path);
      found = 1;
      success = 1;
    }

    strcpy(path, "/bin/");
    strcat(path, argv[1]);

    if (access(path, F_OK) == 0 && found == 0) {
      printf("%s\n", path);
      found = 1;
      success = 1;
    }

    if (found == 0) success = -1;
  } else {
    pid_t pid = fork();
    if (pid == -1) {
      perror("fork");
    } else if (pid == 0) {
      if (execvp(argv[0], argv) == -1) {
        perror("execvp");
        exit(EXIT_FAILURE);
      }
    } else {
      int status;
      waitpid(pid, &status, 0);
    }
  }
}

void expand_wildcards(char *arg, char **argv, int *argc) {
  glob_t glob_result;
  int i;
  if (glob(arg, GLOB_NOCHECK | GLOB_TILDE, NULL, &glob_result) == 0) {
    for (i = 0; i < glob_result.gl_pathc && *argc < MAX_ARGS - 1; i++) {
      argv[(*argc)++] = strdup(glob_result.gl_pathv[i]);
    }
  }
  globfree(&glob_result);
}

void execute_redirection(char *file, char *args[], int key, int stdin,
                         int stdout) {
  int saved_stdin = stdin;
  int saved_stdout = stdout;
  if (key == -1) {
    int fd = open(file, O_RDONLY);
    if (fd == -1) {
      perror("open");
      return;
    }
    dup2(fd, STDIN_FILENO);
    close(fd);
  }
  if (key == 1) {
    int fd = open(file, O_WRONLY | O_CREAT | O_TRUNC,
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1) {
      perror("open");
      return;
    }
    dup2(fd, STDOUT_FILENO);
    close(fd);
  }
  executeCommand(args);
  dup2(saved_stdin, STDIN_FILENO);
  dup2(saved_stdout, STDOUT_FILENO);
}

void pipe_and_redirect(char *args[], char *input_file, char *output_file,
                       char *args_pipe[]) {
  int pipe_fds[2];
  pid_t pid1, pid2;

  if (args_pipe) {
    if (pipe(pipe_fds) == -1) {
      perror("pipe");
      exit(EXIT_FAILURE);
    }
  }

  pid1 = fork();
  if (pid1 == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
  } else if (pid1 == 0) {
    if (input_file) {
      int fd = open(input_file, O_RDONLY);
      if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
      }
      dup2(fd, STDIN_FILENO);
      close(fd);
    }

    if (args_pipe) {
      close(pipe_fds[0]);
      dup2(pipe_fds[1], STDOUT_FILENO);
      close(pipe_fds[1]);
    } else if (output_file) {
      int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC,
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
      if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
      }
      dup2(fd, STDOUT_FILENO);
      close(fd);
    }

    execvp(args[0], args);
    perror("execvp");
    exit(EXIT_FAILURE);
  }

  pid2 = fork();
  if (pid2 == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
  } else if (pid2 == 0) {
    if (input_file) {
      int fd = open(input_file, O_RDONLY);
      if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
      }
      dup2(fd, STDIN_FILENO);
      close(fd);
    }

    if (args_pipe) {
      close(pipe_fds[1]);
      dup2(pipe_fds[0], STDIN_FILENO);
      close(pipe_fds[0]);
    } else if (output_file) {
      int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC,
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
      if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
      }
      dup2(fd, STDOUT_FILENO);
      close(fd);
    }

    execvp(args_pipe[0], args_pipe);
    perror("execvp");
    exit(EXIT_FAILURE);
  }

  close(pipe_fds[0]);
  close(pipe_fds[1]);
  waitpid(pid1, NULL, 0);
  waitpid(pid2, NULL, 0);
}

void parse_and_execute(char *command, int saved_stdin, int saved_stdout) {
  char *args[MAX_ARGS];                
  char *args_pipe[MAX_ARGS];          
  char *token = strtok(command, " ");  
  int argc = 0, argc_pipe = 0;  
  int pipe_found = 0;           
  int redirect = 0;
  char *input_file = NULL;
  char *output_file = NULL;

  while (token != NULL) {
    if (strcmp(token, "<") == 0) {
      redirect = -1;
      if (token != NULL) {
        token = strtok(NULL, " ");  
        input_file = strdup(token);
      } else {
        fprintf(stderr, "Syntax error: Missing input file after '<'\n");
        return;
      }
    } else if (strcmp(token, ">") == 0) {
      redirect = 1;
      if (token != NULL) {
        token = strtok(NULL, " ");  
        output_file = strdup(token);
      } else {
        fprintf(stderr, "Syntax error: Missing output file after '>'\n");
        return;
      }
    } else if (strcmp(token, "|") == 0) {
      pipe_found = 1;  
    } else if (pipe_found) {
      args_pipe[argc_pipe++] = strdup(token);
    } else if (strcmp(token, ">") == 0) {
      redirect = 1;
      if (token != NULL) {
        token = strtok(NULL, " ");  
        output_file = strdup(token);
      } else {
        fprintf(stderr, "Syntax error: Missing output file after '>'\n");
        return;
      }
    } else if (strchr(token, '*')) {
      expand_wildcards(token, args, &argc);
    } else {
      args[argc++] = strdup(token);
    }
    token = strtok(NULL, " ");  
  }
  args[argc] = NULL;            
  args_pipe[argc_pipe] = NULL;  

  if (pipe_found) { // IF THERE IS PIPE
    if (input_file != NULL) { // IF THERE IS REDIRECTION
      pipe_and_redirect(args, input_file, output_file, args_pipe);
    } else if (output_file != NULL) { // IF THERE IS REDIRECTION
      pipe_and_redirect(args, input_file, output_file, args_pipe);

    } else {
      execute_pipe_command(args, args_pipe);  
    }
    for (int i = 0; i < argc_pipe; i++) {
      free(args_pipe[i]);
    }
  } else if (redirect != 0) {
    if (redirect == -1) { // IF THERE IS REDIRECTION NO PIPE
      execute_redirection(input_file, args, -1, saved_stdin, saved_stdout);
    } else if (redirect == 1) { // IF THERE IS REDIRECTION NO PIPE
      execute_redirection(output_file, args, 1, saved_stdin, saved_stdout);
    }
  } else {
    executeCommand(args); 
  }
  for (int i = 0; i < argc; i++) {
    free(args[i]);
  }
}

void interactive_mode() {
  char command[MAX_COMMAND_LENGTH];

  printf("Welcome to the shell！\n");
  while (1) {
    printf("mysh> ");
    if (!fgets(command, MAX_COMMAND_LENGTH, stdin)) {
      // if get wrong command
      break;
    }

    command[strcspn(command, "\n")] = 0;

    if (strcmp(command, "exit") == 0) {
      printf("mysh: exiting\n");
      break;
    }

    int saved_stdin = dup(STDIN_FILENO);
    int saved_stdout = dup(STDOUT_FILENO);

    parse_and_execute(command, saved_stdin, saved_stdout);
  }
}

int main(int argc, char *argv[]) {
  if (argc == 2) {
    batch_mode(argv[1]);
  } else {
    interactive_mode();
  }
  return 0;
}