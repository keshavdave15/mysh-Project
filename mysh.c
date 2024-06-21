#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include "arraylist.h"
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_COMMAND_LENGTH 3000
#define MAX_TOKENS 200
#define MAX_TOKEN_LENGTH 200

char *search_program(char *program_name) {
    char *directories[] = {"/usr/local/bin/", "/usr/bin/", "/bin/"};
    char *full_path = malloc(MAX_TOKEN_LENGTH);
    if (full_path == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // Check if the program name contains a slash (indicating an absolute or relative path)
    if (strchr(program_name, '/') != NULL || access(program_name, X_OK) == 0) {
        // The program name already contains a slash or it's in the current directory
        strncpy(full_path, program_name, MAX_TOKEN_LENGTH);
        return full_path;
    }

    // Search in each directory
    for (int i = 0; i < sizeof(directories) / sizeof(directories[0]); i++) {
        snprintf(full_path, MAX_TOKEN_LENGTH, "%s%s", directories[i], program_name);
        if (access(full_path, X_OK) == 0) {
            // Found the program in this directory
            return full_path;
        }
    }

    // Program not found
    free(full_path);
    return NULL;
}

int cd(arraylist_t *arguments) {
    if (al_length(arguments) != 2) {
        printf("cd: wrong number of arguments\n");
        return 0;
    }

    char *directory = (char *)arguments->data[1];
    if (chdir(directory) != 0) {
        printf("cd: failed, are you sure this is a subdirectory?\n");
        return 0;
    }

    return 1;
}

int pwd(arraylist_t *arguments) {
    if (al_length(arguments) != 1) {
        printf("pwd: too many arguments\n");
        return 0;
    }

    char cwd[2000];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        printf("pwd: path too long to desplay\n");
        return 0;
    }

    printf("%s\n", cwd);
    return 1;
}

int which(arraylist_t *arguments) {
    if (al_length(arguments) != 2) {
        printf("which: wrong number of arguments\n");
        return 0;
    }

    char *program = (char *)arguments->data[1];
    if (strcmp(program, "cd") == 0 || strcmp(program, "pwd") == 0 || strcmp(program, "which") == 0 || strcmp(program, "exit") == 0) {
        printf("which: build it command\n");
        return 1;
    }

    char *temp = search_program((char *)arguments->data[1]);
    if (temp != NULL) {
        printf("%s\n", temp);
        free(temp);
        return 1;
    } 
    else {
        printf("which: program %s not found\n", (char *)arguments->data[1]);
        return 0;
    }
}


int my_exit(arraylist_t *arguments) {
    for (int i = 1; i < al_length(arguments); i++) {
        printf("%s ", (char *)arguments->data[i]);
    }
    if(al_length(arguments) != 1)
        printf("\n");

    return 1;
}

int execute_nopipe(char *stdIn, char *stdOut, arraylist_t *arguments) {
    pid_t pid = fork(); // Fork a new process
    int status;
    int input_Stdinfd = 454540;
    int input_Stdoutfd = 454540;
    if (pid == 0) {
        char *executablePath = arguments->data[0];
    
        // Redirects 
        if (stdIn != NULL) {
            input_Stdinfd = open(stdIn, O_RDONLY);
            if (input_Stdinfd == -1) {
                printf("Cannot open file: %s\n", stdIn);
                exit(EXIT_FAILURE);
            }
            else 
                dup2(input_Stdinfd, STDIN_FILENO);
        }
        if (stdOut != NULL) {
            input_Stdoutfd = open(stdOut, O_CREAT | O_TRUNC | O_WRONLY, 0640);
            if (input_Stdoutfd == -1) {
                printf("Cannot open file: %s\n", stdOut);
                exit(EXIT_FAILURE);
            }
            else 
                dup2(input_Stdoutfd, STDOUT_FILENO);   
        }

        // Checks to see if command is built in
        if (strcmp(executablePath, "cd") == 0) {
            if (cd(arguments)){
                exit(EXIT_SUCCESS);
            }
            else {
                exit(EXIT_FAILURE);
            }
        }
        if (strcmp(executablePath, "pwd") == 0) {
            if (pwd(arguments)) {
                exit(EXIT_SUCCESS);
            }
            else {
                exit(EXIT_FAILURE);
            }
        }
        if (strcmp(executablePath, "which") == 0) {
            if (which(arguments)) {
                exit(EXIT_SUCCESS);
            }
            else {
                exit(EXIT_FAILURE);
            }
        }
        if (strcmp(executablePath, "exit") == 0) {
            my_exit(arguments);
            exit(EXIT_SUCCESS);
        }
        
        // Checks to see if command is a path
        if (strchr(executablePath, '/') != NULL){
            if (execv(executablePath, arguments->data) == -1) {
                printf("%s: could not execute\n", executablePath);
                exit(EXIT_FAILURE);
            }
        }
        else {    
            // Does path search
            char *full_path = search_program(executablePath);
            if (full_path != NULL) {
                if (execv(full_path, arguments->data) == -1) {
                    printf("%s: could not execute\n", executablePath);
                    exit(EXIT_FAILURE);
                }
            } 
            else {
                printf("Program %s not found\n", executablePath);
                exit(EXIT_FAILURE);
            }
        }

        exit(EXIT_SUCCESS);
    }
    wait(&status);

    if(input_Stdoutfd != STDOUT_FILENO){
            close(input_Stdoutfd);
        }
    if(input_Stdinfd != STDIN_FILENO){
        close(input_Stdinfd);
    }
    
    return (WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

int execute_pipe(char *stdIn, char *stdOut, arraylist_t *arguments, char *stdIn_, char *stdOut_, arraylist_t *arguments_) {
    // Makes pipe
    int p[2];
    if (pipe(p) == -1){
        printf("Pipe cannot be created.\n");
        return 0;
    }

    int statusA;
    int input_StdinfdA = 454540;
    int input_StdoutfdA = 454540;
    int statusB;
    int input_StdinfdB = 454540;
    int input_StdoutfdB = 454540;

    if (fork() == 0) {
        // Connect pipe
        close(p[0]);
        dup2(p[1], STDOUT_FILENO);
    
        char *executablePath = arguments->data[0];
    
        // Redirects 
        if (stdIn != NULL) {
            input_StdinfdA = open(stdIn, O_RDONLY);
            if (input_StdinfdA == -1) {
                printf("Cannot open file: %s\n", stdIn);
                exit(EXIT_FAILURE);
            }
            else 
                dup2(input_StdinfdA, STDIN_FILENO);
        }
        if (stdOut != NULL) {
            input_StdoutfdA = open(stdOut, O_CREAT | O_TRUNC | O_WRONLY, 0640);
            if (input_StdoutfdA == -1) {
                printf("Cannot open file: %s\n", stdOut);
                exit(EXIT_FAILURE);
            }
            else 
                dup2(input_StdoutfdA, STDOUT_FILENO);   
        }

        // Checks to see if command is built in
        if (strcmp(executablePath, "cd") == 0) {
            if (cd(arguments)){
                exit(EXIT_SUCCESS);
            }
            else {
                exit(EXIT_FAILURE);
            }
        }
        if (strcmp(executablePath, "pwd") == 0) {
            if (pwd(arguments)) {
                exit(EXIT_SUCCESS);
            }
            else {
                exit(EXIT_FAILURE);
            }
        }
        if (strcmp(executablePath, "which") == 0) {
            if (which(arguments)) {
                exit(EXIT_SUCCESS);
            }
            else {
                exit(EXIT_FAILURE);
            }
        }
        if (strcmp(executablePath, "exit") == 0) {
            my_exit(arguments);
            exit(EXIT_SUCCESS);
        }
        
        // Checks to see if command is a path
        if (strchr(executablePath, '/') != NULL){
            if (execv(executablePath, arguments->data) == -1) {
                printf("%s: could not execute\n", executablePath);
                exit(EXIT_FAILURE);
            }
        }
        else {    
            // Does path search
            char *full_path = search_program(executablePath);
            if (full_path != NULL) {
                if (execv(full_path, arguments->data) == -1) {
                    printf("%s: could not execute\n", executablePath);
                    exit(EXIT_FAILURE);
                }
            } 
            else {
                printf("Program %s not found\n", executablePath);
                exit(EXIT_FAILURE);
            }
            
            exit(EXIT_SUCCESS);
        }
    }

    // Second process
    if (fork() == 0) {
        // Connect pipe
        close(p[1]);
        dup2(p[0], STDIN_FILENO);
    
        char *executablePath_ = arguments_->data[0];
    
        // Redirects 
        if (stdIn_ != NULL) {
            input_StdinfdB = open(stdIn_, O_RDONLY);
            if (input_StdinfdB == -1) {
                printf("Cannot open file: %s\n", stdIn_);
                exit(EXIT_FAILURE);
            }
            else 
                dup2(input_StdinfdB, STDIN_FILENO);
        }
        if (stdOut_ != NULL) {
            input_StdoutfdB = open(stdOut_, O_CREAT | O_TRUNC | O_WRONLY, 0640);
            if (input_StdoutfdB == -1) {
                printf("Cannot open file: %s\n", stdOut_);
                exit(EXIT_FAILURE);
            }
            else 
                dup2(input_StdoutfdB, STDOUT_FILENO);   
        }

        // Checks to see if command is built in
        if (strcmp(executablePath_, "cd") == 0) {
            if (cd(arguments_)){
                exit(EXIT_SUCCESS);
            }
            else {
                exit(EXIT_FAILURE);
            }
        }
        if (strcmp(executablePath_, "pwd") == 0) {
            if (pwd(arguments_)) {
                exit(EXIT_SUCCESS);
            }
            else {
                exit(EXIT_FAILURE);
            }
        }
        if (strcmp(executablePath_, "which") == 0) {
            if (which(arguments_)) {
                exit(EXIT_SUCCESS);
            }
            else {
                exit(EXIT_FAILURE);
            }
        }
        if (strcmp(executablePath_, "exit") == 0) {
            my_exit(arguments_);
            exit(EXIT_SUCCESS);
        }
        
        // Checks to see if command is a path
        if (strchr(executablePath_, '/') != NULL){
            if (execv(executablePath_, arguments_->data) == -1) {
                printf("%s: could not execute\n", executablePath_);
                exit(EXIT_FAILURE);
            }
        }
        else {    
            // Does path search
            char *full_path = search_program(executablePath_);
            if (full_path != NULL) {
                if (execv(full_path, arguments_->data) == -1) {
                    printf("%s: could not execute\n", executablePath_);
                    exit(EXIT_FAILURE);
                }
            } 
            else {
                printf("Program %s not found\n", executablePath_);
                exit(EXIT_FAILURE);
            }
            
            exit(EXIT_SUCCESS);
        }
    }
    close(p[0]);
    close(p[1]);
    wait(&statusA);
    wait(&statusB);

    if(input_StdoutfdA != STDOUT_FILENO){
        close(input_StdoutfdA);
    }
    if(input_StdinfdA != STDIN_FILENO){
        close(input_StdinfdA);
    }
    if(input_StdoutfdB != STDOUT_FILENO){
        close(input_StdoutfdB);
    }
    if(input_StdinfdB != STDIN_FILENO){
        close(input_StdinfdB);
    }

    return ((WIFEXITED(statusA) && WEXITSTATUS(statusA) == 0) && (WIFEXITED(statusB) && WEXITSTATUS(statusB) == 0));
}

char **tokenize_input(char *input) {
    char **tokens = (char **)malloc(MAX_TOKENS * sizeof(char *));
    if (tokens == NULL) {
        printf("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    char *token = strtok(input, " \t\r\n");
    int token_count = 0;

    while (token != NULL) {
        tokens[token_count] = (char *)malloc(MAX_TOKEN_LENGTH * sizeof(char));
        if (tokens[token_count] == NULL) {
            printf("Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
        strcpy(tokens[token_count], token);
        token_count++;

        if (token_count >= MAX_TOKENS) {
            printf("Maximum number of tokens exceeded\n");
            exit(EXIT_FAILURE);
        }
        token = strtok(NULL, " \t\r\n");
    }

    tokens[token_count] = NULL;
    return tokens;
}

void free_tokens(char **tokens) {
    for (int i = 0; tokens[i] != NULL; i++) {
        free(tokens[i]);
    }
    free(tokens);
}

char *executablePath(char **tokens) {
    return tokens[0];
}

char *determineStdin(char **tokens, int token_count) {
    // Loop through the tokens to find '<' indicating input redirection
    for (int i = 1; i < token_count; i++) {
        if (strcmp(tokens[i], "<") == 0) {
            // Found input redirection
            if (i + 1 < token_count) {
                return tokens[i + 1];
            } 
            else {
                // Error: No file name provided after '<'
                // Ignores redirection token
                return NULL;
            }
        }
    }

    // No input redirection found
    return NULL;
}

char *determineStdout(char **tokens, int token_count) {
    // Loop through the tokens to find '>' indicating output redirection
    for (int i = 1; i < token_count; i++) {
        if (strcmp(tokens[i], ">") == 0) {
            // Found output redirection
            if (i + 1 < token_count) {
                return tokens[i + 1];
            } else {
                // Error: No file name provided after '>'
                // Ignores redirection token
                return NULL;
            }
        }
    }

    // No output redirection found
    return NULL;
}

void generate_argument_list(char **tokens, int token_count, arraylist_t *arguments) {
    // Pushes the first element onto the argument list
    al_push(arguments, executablePath(tokens));

    for (int i = 1; i < token_count; i++) {
        char *token = tokens[i];
        if (strcmp(token, "<") == 0 || strcmp(token, ">") == 0) {
            // Skip redirection tokens
            i = i + 1;
        } 
        else if (strchr(token, '*')) {
            if (strchr(token, '/') != NULL) {
                // Handle wildcard
                // Get the directory part before the '*' token
                char directory[MAX_TOKEN_LENGTH]; // Max size is 400 char
                strcpy(directory, token);
                char *pattern_part;
                char *last_slash = strrchr(directory, '/');
                pattern_part = last_slash + 1;
                *last_slash = '\0'; // Remove everything after the last '/'

                char before[MAX_TOKEN_LENGTH];  // Stores everything before the asterisk
                char after[MAX_TOKEN_LENGTH];    // Stores everything after the asterisk
                // Find the position of the asterisk
                char *asterisk_pos = strchr(pattern_part, '*');
                // Copy the directory part before the asterisk
                strncpy(before, pattern_part, asterisk_pos - pattern_part);
                before[asterisk_pos - pattern_part] = '\0';  // Null-terminate the string
                // Copy the filename part after the asterisk
                strcpy(after, asterisk_pos + 1);
                
                // Open the directory
                DIR *dir = opendir(directory);
                int argcount = 0;
                if (dir) {
                    struct dirent *entry;
                    while ((entry = readdir(dir)) != NULL) {
                        // Match files with the pattern
                        if (strncmp(entry->d_name, before, strlen(before)) == 0 &&
                            strcmp(entry->d_name + strlen(entry->d_name) - strlen(after), after) == 0) {
                            if (!(strcmp(before, "") == 0 && entry->d_name[0] == '.')) { // Accounts for hidden files
                                al_push(arguments, entry->d_name); // Add the matched file name
                                argcount++;
                            }
                        }
                    }
                    if(argcount == 0)
                        al_push(arguments, token);
                    closedir(dir);
                } 
                else {
                    al_push(arguments, token);
                }
            }
            else {
                char directory[MAX_TOKEN_LENGTH];
                if (getcwd(directory, sizeof(directory)) == NULL) {
                    perror("getcwd");
                    exit(EXIT_FAILURE);
                }
                // Handle wildcard
                // Get the directory part before the '*' token
                char pattern_part[MAX_TOKEN_LENGTH]; // Max size is 400 char
                strcpy(pattern_part, token);

                char before[MAX_TOKEN_LENGTH];  // Stores everything before the asterisk
                char after[MAX_TOKEN_LENGTH];    // Stores everything after the asterisk
                // Find the position of the asterisk
                char *asterisk_pos = strchr(pattern_part, '*');
                // Copy the directory part before the asterisk
                strncpy(before, pattern_part, asterisk_pos - pattern_part);
                before[asterisk_pos - pattern_part] = '\0';  // Null-terminate the string
                // Copy the filename part after the asterisk
                strcpy(after, asterisk_pos + 1);
                // printf("%s\n", pattern_part);
                // printf("%s\n", before);
                // printf("%s\n", after);
                
                // Open the directory
                DIR *dir = opendir(directory);
                int argcount = 0;
                if (dir) {
                    struct dirent *entry;
                    while ((entry = readdir(dir)) != NULL) {
                        // Match files with the pattern
                        if (strncmp(entry->d_name, before, strlen(before)) == 0 &&
                            strcmp(entry->d_name + strlen(entry->d_name) - strlen(after), after) == 0) {
                            if (!(strcmp(before, "") == 0 && entry->d_name[0] == '.')) { // Accounts for hidden files
                                al_push(arguments, entry->d_name); // Add the matched file name
                                argcount++;
                            }
                        }
                    }
                    if(argcount == 0)
                        al_push(arguments, token);                
                    closedir(dir);
                } 
                else {
                    al_push(arguments, token);
                }
            }
        } 
        else {
            // Regular token, add to the argument list
            al_push(arguments, token);
        }
    }
}

void read_commands_interactive() {
    char command[MAX_COMMAND_LENGTH];
    int bytes_read;
    int command_status = 1;
    int prev = 1;
    int toExit = 0;

    // Welcome message
    printf("Welcome to my shell!\n");

    while (command_status != 0) {
        printf("mysh> "); // Print prompt
        fflush(stdout); // Flush stdout to ensure prompt is displayed

        bytes_read = read(0, command, MAX_COMMAND_LENGTH); // Read input from stdin

        if (bytes_read == -1) {
            perror("read");
            command_status = 1;
            prev = 0;
            exit(EXIT_FAILURE);
        }

        // Remove trailing newline character
        if (bytes_read > 0 && command[bytes_read - 1] == '\n') {
            command[bytes_read - 1] = '\0';
        } 
        else {
            // Clear remaining characters from input buffer
            char c;
            while ((c = getchar()) != '\n' && c != EOF);
        }

        // Process the command here (e.g., tokenize and execute)
        command_status = 0;
        // Gets all tokens in a command
        char **tokens = tokenize_input(command);

        int tokenCount = 0;
        int tokenCount_before_pipe = 0;
        int tokenCount_after_pipe = 0;
        int hasPipe = 0;
        // Initialize the arraylist for arguments
        arraylist_t arguments;
        arraylist_t argumentsSecondary;
        al_init(&arguments, 10); // Initial size of 10, adjust as needed
        al_init(&argumentsSecondary, 10);

        for (int i = 0; tokens[i] != NULL; i++) {
            tokenCount++;
            if (strcmp(tokens[i], "|") == 0){
                hasPipe = i+1;
            }
        }
        if (tokenCount == 0) {
            command_status = 1;
            continue;
        }

        // Checks to see if conditionals are present
        if (strcmp(tokens[0], "then") == 0) {
            if (tokenCount == 1) {
                printf("No command Provided\n");
                command_status = 1;
                continue;
            }
            if (prev == 1) {
                for (int i = 1; tokens[i] != NULL; i++) {
                    tokens[i - 1] = strdup(tokens[i]);
                }
                tokens[tokenCount - 1] = NULL;
                tokenCount = tokenCount - 1;
                if (hasPipe != 0) {
                    hasPipe = hasPipe - 1;
                }
            }
            else {
                command_status = 1;
                continue;
            }
        }
        if (strcmp(tokens[0], "else") == 0) {
            if (tokenCount == 1) {
                printf("No command Provided\n");
                command_status = 1;
                continue;
            }
            if (prev == 0) {
                for (int i = 1; tokens[i] != NULL; i++) {
                    tokens[i - 1] = strdup(tokens[i]);
                }
                tokens[tokenCount - 1] = NULL;
                tokenCount = tokenCount - 1;
                if (hasPipe != 0) {
                    hasPipe = hasPipe - 1;
                }
            }
            else {
                command_status = 1;
                continue;
            }
        }

        // Starts evaluating the tokens
        if (!hasPipe){
            generate_argument_list(tokens, tokenCount, &arguments);

            // Calls execution function
            if (execute_nopipe(determineStdin(tokens, tokenCount), determineStdout(tokens, tokenCount), &arguments)) {
                prev = 1;
            }
            else {
                prev = 0;
            }

            // Checks to see if exit is needed
            if (strcmp(executablePath(tokens), "exit") == 0)
                toExit = 1;
        }
        else{
            // Gets token counts
            hasPipe = hasPipe - 1;
            tokenCount_before_pipe = hasPipe;
            tokenCount_after_pipe = tokenCount - hasPipe - 1;

            if (tokenCount_before_pipe == 0 || tokenCount_after_pipe == 0) {
                command_status = 1;
                prev = 0;
                printf("Pipe command entered incorrectly\n");
                continue;
            }

            // Tokens before the pipe
            char **tokens_before_pipe = (char **)malloc((hasPipe + 1) * sizeof(char *));
            for (int i = 0; i < hasPipe; i++) {
                tokens_before_pipe[i] = strdup(tokens[i]);
            }
            tokens_before_pipe[hasPipe] = NULL;

            // Tokens after the pipe
            char **tokens_after_pipe = (char **)malloc((tokenCount - hasPipe) * sizeof(char *));
            for (int i = hasPipe + 1, j = 0; tokens[i] != NULL; i++, j++) {
                tokens_after_pipe[j] = strdup(tokens[i]);
            }
            tokens_after_pipe[tokenCount - hasPipe - 1] = NULL;

            generate_argument_list(tokens_before_pipe, tokenCount_before_pipe, &arguments);
            generate_argument_list(tokens_after_pipe, tokenCount_after_pipe, &argumentsSecondary);
            
            // printf("Argument list:\n");
            // for (int i = 0; i < al_length(&arguments); i++) {
            //     printf("%s\n", (char *)arguments.data[i]);
            // }
            // Calls execution function
            if (execute_pipe(determineStdin(tokens_before_pipe, tokenCount_before_pipe), determineStdout(tokens_before_pipe, tokenCount_before_pipe), &arguments, determineStdin(tokens_after_pipe, tokenCount_after_pipe), determineStdout(tokens_after_pipe, tokenCount_after_pipe), &argumentsSecondary)) {
                prev = 1;
            }
            else {
                prev = 0;
            }

            // Checks to see if exit is needed
            if (strcmp(executablePath(tokens_before_pipe), "exit") == 0 || strcmp(executablePath(tokens_after_pipe), "exit") == 0)
                toExit = 1;

            // Clean up
            free_tokens(tokens_before_pipe);
            free_tokens(tokens_after_pipe);
        }
        
        // Clean up memory
        al_destroy(&arguments);
        al_destroy(&argumentsSecondary);
        free_tokens(tokens);

        // Exits 
        if (toExit) {
            // Exit command received
            printf("Exiting my shell.\n");
            command_status = 0;
            continue;
        }

        // Everything is finished
        command_status = 1;
    }
}

int process_batch_command(char *command, int prev) {
    int toExit = 0;

    // Gets all tokens in a command
    char **tokens = tokenize_input(command);

    int tokenCount = 0;
    int tokenCount_before_pipe = 0;
    int tokenCount_after_pipe = 0;
    int hasPipe = 0;
    // Initialize the arraylist for arguments
    arraylist_t arguments;
    arraylist_t argumentsSecondary;
    al_init(&arguments, 10); // Initial size of 10, adjust as needed
    al_init(&argumentsSecondary, 10);

    for (int i = 0; tokens[i] != NULL; i++) {
        tokenCount++;
        if (strcmp(tokens[i], "|") == 0){
            hasPipe = i+1;
        }
    }
    if (tokenCount == 0) {
        return prev;
    }

    // Checks to see if conditionals are present
    if (strcmp(tokens[0], "then") == 0) {
        if (tokenCount == 1) {
            printf("No command Provided\n");
            return prev;
        }
        if (prev == 1) {
            for (int i = 1; tokens[i] != NULL; i++) {
                tokens[i - 1] = strdup(tokens[i]);
            }
            tokens[tokenCount - 1] = NULL;
            tokenCount = tokenCount - 1;
            if (hasPipe != 0) {
                hasPipe = hasPipe - 1;
            }
        }
        else {
            return prev;
        }
    }
    if (strcmp(tokens[0], "else") == 0) {
        if (tokenCount == 1) {
            printf("No command Provided\n");
            return prev;
        }
        if (prev == 0) {
            for (int i = 1; tokens[i] != NULL; i++) {
                tokens[i - 1] = strdup(tokens[i]);
            }
            tokens[tokenCount - 1] = NULL;
            tokenCount = tokenCount - 1;
            if (hasPipe != 0) {
                hasPipe = hasPipe - 1;
            }
        }
        else {
            return prev;
        }
    }

    // Starts evaluating the tokens
    if (!hasPipe){
        generate_argument_list(tokens, tokenCount, &arguments);

        // Calls execution function
        if (execute_nopipe(determineStdin(tokens, tokenCount), determineStdout(tokens, tokenCount), &arguments)) {
            prev = 1;
        }
        else {
            prev = 0;
        }

        // Checks to see if exit is needed
        if (strcmp(executablePath(tokens), "exit") == 0)
            toExit = 1;
    }
    else{
        // Gets token counts
        hasPipe = hasPipe - 1;
        tokenCount_before_pipe = hasPipe;
        tokenCount_after_pipe = tokenCount - hasPipe - 1;

        if (tokenCount_before_pipe == 0 || tokenCount_after_pipe == 0) {
            prev = 0;
            printf("Pipe command entered incorrectly\n");
            return prev;
        }

        // Tokens before the pipe
        char **tokens_before_pipe = (char **)malloc((hasPipe + 1) * sizeof(char *));
        for (int i = 0; i < hasPipe; i++) {
            tokens_before_pipe[i] = strdup(tokens[i]);
        }
        tokens_before_pipe[hasPipe] = NULL;

        // Tokens after the pipe
        char **tokens_after_pipe = (char **)malloc((tokenCount - hasPipe) * sizeof(char *));
        for (int i = hasPipe + 1, j = 0; tokens[i] != NULL; i++, j++) {
            tokens_after_pipe[j] = strdup(tokens[i]);
        }
        tokens_after_pipe[tokenCount - hasPipe - 1] = NULL;

        generate_argument_list(tokens_before_pipe, tokenCount_before_pipe, &arguments);
        generate_argument_list(tokens_after_pipe, tokenCount_after_pipe, &argumentsSecondary);
        
        // printf("Argument list:\n");
        // for (int i = 0; i < al_length(&arguments); i++) {
        //     printf("%s\n", (char *)arguments.data[i]);
        // }
        // Calls execution function
        if (execute_pipe(determineStdin(tokens_before_pipe, tokenCount_before_pipe), determineStdout(tokens_before_pipe, tokenCount_before_pipe), &arguments, determineStdin(tokens_after_pipe, tokenCount_after_pipe), determineStdout(tokens_after_pipe, tokenCount_after_pipe), &argumentsSecondary)) {
            prev = 1;
        }
        else {
            prev = 0;
        }

        // Checks to see if exit is needed
        if (strcmp(executablePath(tokens_before_pipe), "exit") == 0 || strcmp(executablePath(tokens_after_pipe), "exit") == 0)
            toExit = 1;

        // Clean up
        free_tokens(tokens_before_pipe);
        free_tokens(tokens_after_pipe);
    }
    
    // Clean up memory
    al_destroy(&arguments);
    al_destroy(&argumentsSecondary);
    free_tokens(tokens);

    // Exits 
    if (toExit) {
        return prev;
    }

    // Everything is finished
}

void read_commands_batch(int fd) {
    char command[MAX_COMMAND_LENGTH];
    int i = 0;
    char c;
    int prev = 1;

    // Read characters until newline or EOF
    while (read(fd, &c, 1) > 0) {
        if (c == '\n') {
            command[i] = '\0'; // Null-terminate the command
            prev = process_batch_command(command, prev); // Process the command
            i = 0; // Reset index for the next command
        } else {
            // Append character to command
            command[i++] = c;
            if (i >= MAX_COMMAND_LENGTH) {
                printf("Command too long.\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    // Check for read error
    if (read(fd, &c, 1) == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }

    // Sends in last command after EOF
    if (i > 0) {
        command[i] = '\0'; // Null-terminate the command
        prev = process_batch_command(command, prev); // Process the command
    }
}

int main(int argc, char *argv[]) {
    int input_fd;
    if (argc >= 2) { // Checks to see if there is a filename argument
        input_fd = open(argv[1], O_RDONLY);
        if (input_fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
    } 
    else {
        input_fd = 0;
    }

    if (isatty(input_fd)) { // Finds out which mode to use based on isatty()
        printf("Interactive mode\n");
        read_commands_interactive();
    }
    else{
        read_commands_batch(input_fd);
    }

    if (input_fd != 0){
        close(input_fd); // Close the file after reading
    } 

    return EXIT_SUCCESS; // This project was hard please give good grade
}