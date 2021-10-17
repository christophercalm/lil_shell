#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DEFAULT_SHELL_INPUT_BUFFER_SIZE 1024
#define DEFAULT_INPUT_BUFFER_REALLOC_SIZE 512
#define DEFAULT_STROK_BUFFER_SIZE 64
#define TOKEN_DELIMETER " \t\r\n\a"


int shell_cd(char ** args);
int shell_help(char ** args);
int shell_exit(char ** args);

char *builtin_functions[] = {
    "cd",
    "help",
    "exit"
};


int (*builtin_jmp_table[])(char **) = {
    &shell_cd,
    &shell_help,
    &shell_exit
};

int shell_num_builtins() {
    return sizeof(builtin_functions) / sizeof(char *);
}

int shell_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Expected argument to \"cd\"\n");
    } else {
        if(chdir(args[1]) != 0) {
            //perror adds a special message before the actual error message from stderr
            perror("shell");
        }
    }
    return 1;
}

int shell_help(char **args) {
    printf("*****************************\n");
    printf("* Christopher's Toy Shell\n");
    printf("* Number of builtin functions: %d\n", shell_num_builtins());
    for (int i = 0; i < shell_num_builtins(); i++) {
        printf("* %s\n", builtin_functions[i]);
    }
    printf("*****************************\n");
    return 1;
}

int shell_exit(char ** args) {
    return 0;
}

char * shell_read_line() {
    int buffer_size = DEFAULT_SHELL_INPUT_BUFFER_SIZE;
    int input_position = 0;
    char *buffer = malloc(sizeof(char) * buffer_size);

    int character;

    if (!buffer) {
        fprintf(stderr, "Could not allocate input buffer\n");
        exit(EXIT_FAILURE);
    }

    while(1) {
        //make buffer size bigger if we don't have enough room
        if (input_position > buffer_size) {
            buffer_size = buffer_size + DEFAULT_INPUT_BUFFER_REALLOC_SIZE;
            buffer = (char *)realloc(buffer, buffer_size);
            if (!buffer) {
                fprintf(stderr, "Could not allocate input buffer\n");
                exit(EXIT_FAILURE);
            }
        }

        character = getchar();

        if (character == EOF || character == '\n') {
            buffer[input_position] = '\0';
            return buffer;
        } else {
            buffer[input_position] = character;
        }
        input_position++;
    }
}

//todo make this work for non space identifiers
char ** shell_split_line(char * line) {
    int buffer_size = DEFAULT_STROK_BUFFER_SIZE;
    int line_position = 0;
    //allocate space for with size of char pointers
    char ** delim_tokens = malloc(buffer_size * sizeof(char*));
    char * token;
    if (!delim_tokens) {
        fprintf(stderr, "Could not allocate token buffer");
        exit(EXIT_FAILURE);
    }

    //char *strtok(char *str, const char *delim)
    //if character is delimiter (here "") then add a pointer to the list

    token = strtok(line, TOKEN_DELIMETER);
    while(token != NULL) {

        //reallocate memory in buffer if not enought space
        if (line_position > buffer_size) {
            buffer_size += DEFAULT_STROK_BUFFER_SIZE;
            delim_tokens = realloc(delim_tokens, buffer_size * sizeof(char *));
            if (!delim_tokens) {
                fprintf(stderr, "Could not allocate token buffer");
                exit(EXIT_FAILURE);
            }
        }
        delim_tokens[line_position] = token;
        line_position++;
        token = strtok(NULL, TOKEN_DELIMETER);
    }

    //add null pointer to indicate the end
    delim_tokens[line_position] = NULL;
    return delim_tokens;
}

int shell_launch(char ** args) {
    pid_t process_id;
    pid_t worker_process_id;
    int status;

    //we create a new process in unix by forking the current process and then using the exec() call
    process_id = fork();
    if (process_id == 0) {
        //We are a child process. If we return any value there is an error.
        if (execvp(args[0], args) == -1) {
            perror("Shell");
        }
        exit(EXIT_FAILURE);

    } else if (process_id < 0) {
        //error spawning
        perror("shell");

    } else {
        //parent success
        do {
            //The waitpid() system call suspends execution of the calling process until a child specified by pid argument has changed state.
            worker_process_id = waitpid(process_id, &status, WUNTRACED);
        }
        //WIFSIGNALED determines if the child process exited because it raised a signal that caused it to exit.
        while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;

}

int shell_execute(char ** args) {
    if (args[0] == 0) {
        //empty command was entered
        return 1;
    }

    for(int i = 0; i < shell_num_builtins(); ++i) {
        if(strcmp(args[0], builtin_functions[i]) == 0) {
            //execute function from table
            return (*builtin_jmp_table[i])(args);
        }
    }

    return shell_launch(args);
}

void shell_loop() {
    char *line;
    char **args;
    int status = 1;

    while (status) {
        printf("~$ ");
        //read
        line = shell_read_line();
        //parse
        args = shell_split_line(line);
        //execute
        status = shell_execute(args);

        free(line);
        free(args);
    }

}

int main(int argc, char **argv) {
    // Load config files, if any.

    // Run command loop.
    shell_loop();

    // Perform any shutdown/cleanup.

    return EXIT_SUCCESS;
}
