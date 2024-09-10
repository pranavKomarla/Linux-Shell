#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <glob.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
//#include "arraylist.h"

#define MAX_COMMAND_LENGTH 1024






typedef struct {
    char **data;
    int length;
    int capacity;
} arraylist_t;


void al_init(arraylist_t *, int);
void al_destroy(arraylist_t *);
int al_length(arraylist_t *);
void al_push(arraylist_t *, char *);
int al_pop(arraylist_t *, char *);



#ifndef DEBUG
#define DEBUG 0
#endif
void al_init(arraylist_t *L, int size)
{

    
    L->data = malloc(size * sizeof(char*));
    //printf("INITIALIZING\n");
    L->length = 0;
    L->capacity = size;
}
void al_destroy(arraylist_t *L)
{
    //free(L->data);
    int count = L->capacity;
    for (int i = 0; i < count; i++) {
        free(L->data[i]);
    }
}

int al_length(arraylist_t *L)
{
    return L->length;
}

void al_push(arraylist_t *L, char* item)
{
    //printf("%s\n", item); 
    if (L->length == L->capacity) {
        L->capacity *= 2;
        char** temp = realloc(L->data, L->capacity * sizeof(char*));
        if (!temp) {
            // for our own purposes, we can decide how to handle this error
            // for more general code, it would be better to indicate failure to our caller
            fprintf(stderr, "Out of memory!\n");
            exit(EXIT_FAILURE);
        }
        L->data = temp;
    if (DEBUG) printf("Resized array to %u\n", L->capacity);
    }
    //printf("length: %d\n", L->length); 
    L->data[L->length] = strdup(item);
    //printf("%s\n", L->data[0]);
    L->length++;
    //printf("length: %d\n", L->length); 
}

// returns 1 on success and writes popped item to dest
// returns 0 on failure (list empty)
int al_pop(arraylist_t *L, char *dest)
{
    if (L->length == 0) return 0;
    L->length--;
    *dest = L->data[L->length];
    return 1;
}


// void batch_mode(const char *filename) {
//   char line[500];
//   FILE *file = fopen(filename, "r");
//   if (!file) {
//     perror("open file error");
//     exit(EXIT_FAILURE);
//   }
//   while (fgets(line, 500, file)) {
//     line[strcspn(line, "\n")] = 0;
//     parse_and_execute(line);
//   }

//   fclose(file);
// }

void batch_mode(const char *filename) {
    char line[MAX_COMMAND_LENGTH];
    char c;
    int bytes_read;
    int pos = 0;
    int batch_fd = open(filename, O_RDONLY);
    int original_stdin_fd = dup(STDIN_FILENO);
    int original_stdout_fd = dup(STDOUT_FILENO);

    while ((bytes_read = read(batch_fd, &c, 1)) > 0) {
        if (c == '\n') {
            line[pos] = '\0'; // Null-terminate the line string
            parse_and_execute(line);
            pos = 0; // Reset position for the next line
            dup2(original_stdin_fd, STDIN_FILENO);
            dup2(original_stdout_fd, STDOUT_FILENO);

            // Close the duplicate file descriptors
            close(original_stdin_fd);
            close(original_stdout_fd);
        } else if (bytes_read == -1) {
            perror("read error");
            exit(EXIT_FAILURE);
        } else {
            if (pos < MAX_COMMAND_LENGTH - 1) {
                line[pos++] = c; 
            } else {
                fprintf(stderr, "Command too long.\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    
    if(pos > 0){
        line[pos] = '\0'; 
        parse_and_execute(line);
    }
    if (bytes_read == -1) {
        perror("read error");
        exit(EXIT_FAILURE);
    }
    close(batch_fd);
}

void interactive_mode() {
    char line[MAX_COMMAND_LENGTH];
    int bytes_read;

    printf("Welcome to the shell！\n");
    while (1) {
        printf("mysh> ");
        fflush(stdout);

        // Read input from the user
        bytes_read = read(STDIN_FILENO, line, MAX_COMMAND_LENGTH);

        

        // Check for errors or end of file
        if (bytes_read == -1) {
            perror("read");
            break;
        } else if (bytes_read == 0) {
            // End of file reached (e.g., user pressed Ctrl+D)
            printf("mysh: exiting\n");
            break;
        }

        // Null-terminate the string
        line[bytes_read] = '\0';

        // Remove line break if present
        if (bytes_read > 0 && line[bytes_read - 1] == '\n') {
            line[bytes_read - 1] = '\0';
        }

        

        //printf("line is: %s\n", line);
        // Parse and execute the command
        parse_and_execute(line);
        
    }
}

void redirect(arraylist_t *args, char* input_file, char* output_file, int redirection) {

    
    int saved_stdin = dup(STDIN_FILENO);
    int saved_stdout = dup(STDOUT_FILENO);


    if(redirection == -1) {
            int redirect_input = open(input_file, O_RDONLY);
            if (redirect_input == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            // Redirect standard input to the file
            dup2(redirect_input, STDIN_FILENO);
            close(redirect_input);
        } 
        
        if(redirection == 1) {
            int redirect_output = open(output_file, O_WRONLY | O_CREAT | O_TRUNC,
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (redirect_output == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            // Redirect standard input to the file
            dup2(redirect_output, STDOUT_FILENO);
            close(redirect_output);

            
        } 

        execute_command(args); 
        dup2(saved_stdin, STDIN_FILENO);
        dup2(saved_stdout, STDOUT_FILENO);
        //exit(EXIT_SUCCESS);
    

}

void pipe_and_redirect(arraylist_t* args, char *input_file, char* input_file_pipe, char *output_file, char* output_file_pipe, arraylist_t *args_pipe) {
  
//   for(int i = 0; i < args->length; i++) {
//     printf("ARGS: %s\n", args->data[i]);
//   } 

//   for(int i = 0; i < args_pipe->length; i++) {
//     printf("ARGS_PIPE: %s\n", args_pipe->data[i]);
//   } 

//  for(int i = 0; i < args_pipe->length; i++) {
//             printf("ARGS_PIPE: %s\n", args_pipe->data[i]);
//         } 

    // char *envPaths = "/usr/local/bin:/usr/bin:/bin";
    // setenv("PATH", envPaths, 1);

    // int saved_stdin = dup(STDIN_FILENO);
    // int saved_stdout = dup(STDOUT_FILENO);

//   printf("INPUT FILE: %s\n", input_file);
//   printf("OUTPUT FILE: %s\n", output_file);
//   printf("INPUT_PIPE FILE: %s\n", input_file_pipe);
//   printf("OUTPUT_PIPE FILE: %s\n", output_file_pipe);
  
  input_file = args->data[1];
  int pipe_fds[2];
  pid_t pid1, pid2;

  if (args_pipe->data) {
    if (pipe(pipe_fds) == -1) {
      perror("pipe");
      exit(EXIT_FAILURE);
    }
  }
    //cat < input.txt | grep hello
    //printf("DATA0 %s\n:", args->data[0]);
    pid1 = fork();
    //printf("HERE!\n"); 
    if (pid1 == 0) {
         
        // first child
        close(pipe_fds[0]); 
        
        dup2(pipe_fds[1], STDOUT_FILENO); //Redirects standard input to the read side of the pipe
        //printf("HERE!\n");  
        close(pipe_fds[1]);
        

         

        if(input_file) {
            
            int fd = open(input_file, O_RDONLY);
            if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }


        
        // if(args->length == 1) {
            
        //     execlp("cat", "cat", "input.txt", NULL);
        // } else {
            execvp(args->data[0], args->data);
        //}
        
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    pid2 = fork();
    if (pid2 == 0) {
        // second child
        close(pipe_fds[1]); 
        dup2(pipe_fds[0], STDIN_FILENO);  //Redirects standard input to the read side of the pipe
        close(pipe_fds[0]);

          if(output_file_pipe) {
            int fd = open(output_file_pipe, O_WRONLY | O_CREAT | O_TRUNC, 0640);

            dup2(fd, STDOUT_FILENO);
            close(fd);    

          }

       
        //args_pipe->data[args->length] = NULL;
        execvp(args_pipe->data[0], args_pipe->data);
        perror("execvp");
        exit(EXIT_FAILURE);
    }

  close(pipe_fds[0]);
  close(pipe_fds[1]);
  waitpid(pid1, NULL, 0);
  waitpid(pid2, NULL, 0);

//   dup2(saved_stdin, STDIN_FILENO);
//   dup2(saved_stdout, STDOUT_FILENO);
}

void parse_and_execute(char *line) {

    //printf("PARSING!\n");

    arraylist_t *args;
    arraylist_t *args_pipe;

    args = malloc(sizeof(arraylist_t));
    args_pipe = malloc(sizeof(arraylist_t));

    al_init(args_pipe, 1);
    al_init(args, 1); 

    
    char *token = strtok(line, " ");
    int argCount = 0, arg_pipeCount = 0;

    char* input_file = NULL;
    char* output_file = NULL;
    char* input_file_pipe = NULL;
    char* output_file_pipe = NULL;


    int pipe_found = 0; 
    int redirection = 0;

    int conflict = 0;

    while(token != NULL) {
        
        if(strcmp(token, "<") == 0)
        {
            redirection = -1;
            token = strtok(NULL, " ");
            if(token != NULL) {
                input_file = strdup(token);
                al_push(args, token);
                

            } else {
                printf("Syntax error: Missing input file after '<'\n");
                return; 
            }

        } else if (strcmp(token, ">") == 0) {
            redirection = 1;
            conflict = 1;
            
            token = strtok(NULL, " ");
            if(token != NULL) {
                output_file = strdup(token);
                

            } else {
                printf("Syntax error: Missing output file after '>'\n");
                return; 
            }
        } else if (strcmp(token, "|") == 0) {
            //printf("PIPE FOUND!\n");
            pipe_found = 1;
            break;
        } else if (strchr(token, '*')) {
            expand_wildcards(token, args);
        }
        else {
            //printf("HERE");
            al_push(args, token); 
            //printf("%s\n", token); 

           
            //printf("length after: %d\n", args->length); 
        }
        token = strtok(NULL, " ");
    }

    args->data[args->length] = NULL;

    if(pipe_found == 1) {
      
        if(conflict == 1){
            fprintf(stderr, "conflict: cannot output to multiple places at a time.");
        }
        token = strtok(NULL, " ");
        //printf("token after pipe: %s\n", token); 
        while (token != NULL) {
            
        if(strcmp(token, "<") == 0)
        {
            redirection = -1;
            token = strtok(NULL, " ");
            if(token != NULL) {
                input_file_pipe = strdup(token);
                

            } else {
                printf("Syntax error: Missing input file after '<'\n");
                return; 
            }

        } else if (strcmp(token, ">") == 0) {
            redirection = 1;
            
            token = strtok(NULL, " ");
            if(token != NULL) {
                output_file_pipe = strdup(token);
                

            } else {
                printf("Syntax error: Missing output file after '>'\n");
                return; 
            }
        } else {
             al_push(args_pipe, token);
        }   
       
        token = strtok(NULL, " "); 

        }
        args_pipe->data[args_pipe->length] = NULL;

        if (input_file_pipe != NULL || input_file != NULL) { // IF THERE IS REDIRECTION
            
            pipe_and_redirect(args, input_file, input_file_pipe, output_file, output_file_pipe, args_pipe);
        } else if (output_file_pipe != NULL || output_file != NULL) { // IF THERE IS REDIRECTION
            //printf("PIPE AND REDIRECT Output\n"); 
            pipe_and_redirect(args, input_file, input_file_pipe, output_file, output_file_pipe, args_pipe);

        } else {
            execute_pipe_command(args, args_pipe);  
            al_destroy(args_pipe);
        
        }

        //execute_pipe_command(args, args_pipe);
       
        
    } else if(redirection != 0) {
        if(redirection == -1) {
            redirect(args, input_file, output_file, -1);
        } else {
            redirect(args, input_file, output_file, 1);
        }
    } else {
        //printf("Pipe not found\n");
        //printf("before execute: %s\n", args->data[0]);
        execute_command(args);  
        
    }
    al_destroy(args); 

    free(args);
    free(args_pipe); 

}

char* find_path(char* filename) {
    const char *directories[] = {"/usr/local/bin", "/usr/bin", "/bin"};

    // Iterate over each directory
    for (int i = 0; i < 3; i++) {
        // Construct the full path by concatenating the directory path and filename
        char full_path[1024]; // Adjust the size as needed
        snprintf(full_path, sizeof(full_path), "%s/%s", directories[i], filename);

        // Check if the file exists in the current directory
        if (access(full_path, F_OK) == 0) {
            //printf("File %s exists in %s\n", filename, directories[i]);
            char *result_path = malloc(strlen(full_path) + 1);
            if (result_path == NULL) {
                perror("malloc");
                exit(EXIT_FAILURE);
            }
            strcpy(result_path, full_path);
            return result_path; 
        } 
    }

    return "hello";

    
}

void execute_command(arraylist_t *args) {

    //printf("Executing command\n");
    //printf("%s\n", args->data[0]);
    static int success = 0; 
    

    if (strcmp(args->data[0], "then") == 0) {
        for (int i = 0; i < args->length; i++) {
            args->data[i] = args->data[i + 1];
        }

        if (success == -1) {
            printf("Error: Last command was failed\n");
            return;
        }
    }

    if (strcmp(args->data[0], "else") == 0) {
        for (int i = 0; i < args->length; i++) {
            args->data[i] = args->data[i + 1];
        }

        if (success == 1) {
            printf("Error: Last command was not failed\n");
            return;
        }
    }

    // for(int i = 0; i < args->length; i++) {
    //     printf("ARGS: %s", args->data[i]); 
    // }
    const char *builtin_functions[] = {"cd", "pwd", "which", "exit"};

    if (strcmp(args->data[0], "cd") == 0) {
      
        if (args->data[1] == NULL || args->data[2] != NULL) {
            fprintf(stderr, "cd: Error Number of Parameters\n");
            success = -1;
        } else if(strcmp(args->data[1], "..") == 0) {
            char current_directory[1024];

            // Get the current working directory
            if (getcwd(current_directory, sizeof(current_directory)) != NULL) {
                //printf("Current working directory: %s\n", current_directory);

                // Find the last occurrence of '/' to get the parent directory
                char *last_slash = strrchr(current_directory, '/');
                if (last_slash != NULL) {
                    *last_slash = '\0'; // Set the last slash to null character to truncate the string

                
                    if (chdir(current_directory) == 0) {
                        //printf("Changed to the parent directory.\n");
                        success = 1;
                    } else {
                        success = -1;
                        perror("chdir");
                        return 1;
                    }
                } else {
                    success = -1;
                    printf("Unable to determine the parent directory.\n");
                    return 1;
                }
            } else {
                perror("getcwd");
                return; 
            }


        } else if (chdir(args->data[1]) != 0) {
            success = -1;
            perror("cd: No such file or directory\n");
        } else {
            success = 1;
        }
    } 
    else if (strcmp(args->data[0], "pwd") == 0) {
        // pwd command
        //printf("IN PWD\n");
        // for(int i = 0; i < args->length; i ++) {
        //     printf("ARGS: %s\n", args->data[i]);
        // }
        char cwd[MAX_COMMAND_LENGTH];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
            success = 1;
        } 
        else {
            success = -1;
            perror("pwd");
        }
    } else if(strcmp(args->data[0], "which") == 0) {
        if (args->data[1] == NULL) {
            success = -1;
            printf("Wrong number of arguments\n");
            return;
        }

        for(int i = 0; i < 4; i++) {
            if(strcmp(args->data[1], builtin_functions[i]) == 0) {
                success = -1;
                printf("Failure due to built in function\n");
                return;
            }
        }

        char* full_path = find_path(args->data[1]);

        if(strcmp(full_path, "hello") == 0) {
            success = -1;
            printf("File does not exist\n"); 
            return; 
        } else {
            printf("%s\n", full_path);
            success = 1;
        }


        
    } else if(strcmp(args->data[0], "exit") == 0) {
        //printf("HERE!\n");

        if(args->data[1] != NULL) {
            for(int i = 1; i < args->length; i ++) {
                printf("%s ", args->data[i]);
            }
        }   
        
        //printf("THIRD ARG%s", args->data[3]);
        printf("\n");
        printf("Exiting My Shell!\n");
        exit(EXIT_SUCCESS);
    
    }

    else {
        // external command
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
        }
        else if (pid == 0) {
            // child progress

            char* full_path = NULL;
            if(strchr(args->data[0], '/') == NULL) {
                full_path = find_path(args->data[0]);
            } else {
                full_path = args->data[0];
            }
            
            
            //printf("THIS IS THE FULL PATH: %s", full_path); 

            if (execv(full_path, args->data) == -1) {
                perror("exec");
                exit(EXIT_FAILURE);
            }
        }
        else {
            // father progress
            int status;
            waitpid(pid, &status, 0);
        }
    }

    
}

void execute_pipe_command(arraylist_t *args, arraylist_t *args_pipe) {
    int pipe_fds[2];
    pid_t pid1, pid2;

    if (pipe(pipe_fds) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid1 = fork();
    if (pid1 == 0) {
        // first child
        close(pipe_fds[0]); 
        dup2(pipe_fds[1], STDOUT_FILENO);   //Redirects standard input to the read side of the pipe
        close(pipe_fds[1]);

        execvp(args->data[0], args->data);
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    pid2 = fork();
    if (pid2 == 0) {
        // second child
        close(pipe_fds[1]); 
        dup2(pipe_fds[0], STDIN_FILENO);  //Redirects standard input to the read side of the pipe
        close(pipe_fds[0]);

        execvp(args_pipe->data[0], args_pipe->data);
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    // father
    //printf("HERE! 324\n");
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

void expand_wildcards(char *arg, arraylist_t *args) {
    glob_t glob_result;
    int i;

    if (glob(arg, GLOB_NOCHECK | GLOB_TILDE, NULL, &glob_result) == 0) {
        for (i = 0; i < glob_result.gl_pathc; i++) {
            //args[(*argc)++] = strdup(glob_result.gl_pathv[i]);
            al_push(args, glob_result.gl_pathv[i]);


        }
    }
    globfree(&glob_result);
}



int BMCheck(int argc, char *argv[]) {
    // Check if there are command-line arguments
    if (argc > 1) {
        return 1; // Running in batch mode
    }

    // Check if input is being piped
    if (!isatty(fileno(stdin))) {
        return 1; // Running in batch mode
    }

    return 0; // Running in interactive mode
}

int main(int argc, char **argv) {
    // Parsing commands Interactive mode or Script Mode
    if (BMCheck(argc, argv)) {
        if (argc > 1) {

            batch_mode(argv[1]);

        } 
        
    } else {
   
            interactive_mode(); 

    }
    
    
    // Exit the Shell
    return EXIT_SUCCESS;
}


// int main(int argc, char *argv[]) {

//     // if (argc == 2) {
//     //     batch_mode(argv[1]);
//     // } else {
//     //     interactive_mode();
//     // }

//      if (isatty(STDIN_FILENO)) {
//         batch_mode(argv[1]);
        
//     } else {
//         interactive_mode(); 
//     }
    
    


//     return 0;
// }