Author: Keshav Dave
NetID: khd47

Important design notes
    This program is my custon created shell mysh.
    Batch mode may have some bugs in peculiar corner cases. In 99 precent of cases, it will work
    Interactive mode may have bugs when redicreting output or using grep only in the case that a few commands have been run already. In these cases, it will start printing unknown symbols
    Max command length: 3000 charecter
    Max tokens: 200 
    Max token length: 200 charecters
    dup2() is used for redirection in execution functions. pipe() is used for piping. execv() is used to execute programs
    arraylist.c and arraylist.h is used to generate the argument list. This code was adjusted from the code on canvas to accept strings instead of ints
    Using isatty(), mysh will determine whether to run in interactive mode or batch mode
    Welcome message: Welcome to my shell! Goodbye message: Exiting my shell.
    Interactive mode usses fflush(). I do not know if this is a restricted function.
    An empty command does not change the previous exit status. Conditional statements if or else rely on this.
    The first token after tokenization is always to executable program unless the first token is if or else. In this case, the second token is the program to execute.
    The execution functions return the exit status of the program. 0 for fail 1 for success
    Redirection for standard output can be redirected to an existing file or a new one. If an existing file is selected, everything in it will be overwritten with the new output. If a new file is selected, it will be created and written into in the working directory
    Use of conditionals after a pip is invalid
    Built in commands are cd, pwd, which, and exit
    exit always returns a successful status
    Files that start with a . will be hidden and not found be wildcards
    Pipes will take standard output of the first prgram and output it to the second program, which uses it as its input. There cannot be more than 1 pipe for my program
    Both child processes of the pipe run concurrectly. If there is an unsuccesful command in the beginning of the pipe, its error message will still be outputted to the second program. Be careful when using pipes.
    When redirection is present in a pipe, redirection will take precedence. In my code. I so not hard code any processes to destroy the pipe in the case of redirection. Be carful redirecting with a pipe
    Wildcards can only be present in command arguments and in the last / of a path
    A make file is not provided for this project

How to compile and run project files
    gcc -o mysh mysh.c arraylist.c
    Interactive mode
        ./mysh
    Batch mode
        ./mysh batchtest.txt

Test plan and design plan
    I approached designing this project by splitting work into 3 parts
        First, I built the loops for batch and interactive mode and created code that will collect each command and turn it into a string. I also set up the arraylist helper files
        Then, I used this command string to find : 1. The path to the executable file. 2. The list of argument strings. 3. Which files to use for standard input and output. Through the help of various functions and some control of conditionals, this was achieved and this information could be sent to execution functions
        Finally, I wrote execution functions which would preate child processes, utilize pipes, and redirect standard output as necessary. This was done using dup2(), execv() fork(), wait() and pipe()
    spchk.c puts together all three of these to create a spell-checking program
    Testing
        Feel free to test the code as you please in interactive or batch mode. Directories and files provided and involed for testing:
            test, test.txt, test2.txt
            testingResults.txt
                Results from my testing of mysh on interactive mode
                Shows usage on the mysh program in numerous possible situations
                Involves the other test files
            batchtest.txt
                a testing file that can be ran with mysh on batch mode

This project took me dozens of hours. Please recognize the attention to detail and if there are any bugs, they will be small and insignificant. Please take note of some of these bugs I mention in my design notes. Take consideration all of this information when grading the project. Take consideration that I completed this project all on my own. Enjoy use of my shell!