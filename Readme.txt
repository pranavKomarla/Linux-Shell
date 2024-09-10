Vagish Murali vm595 Pranav Komarla pvk14

Design:
This code defines a custom shell program in C, featuring support for basic command execution, file redirection, piping between commands, and handling of built-in 
commands like cd, pwd, which, and exit. The parse_and_execute method is central, parsing user input to determine whether to execute a command directly, handle 
file redirections (<, >), or manage piping (|) between commands. The execute_command and execute_pipe_command functions are responsible for executing the parsed 
commands, either directly or through a pipe, using system calls like execv for external commands. File redirection and input/output handling for commands are 
managed by redirect and pipe_and_redirect functions, adjusting file descriptors as needed for proper execution. The shell can be run in interactive mode, 
presenting a prompt to the user and processing commands line by line, or in batch mode, executing commands from a provided file. As mentioned in the file, the code checks if 
the first argument contains a '/' and treats that argument as a full path.

Test Cases:
cd
This throws an error as there is not enough parameters printing out: cd: Error Number of Parameters

cd file_does_not_exist
This throws an error as the file doesn't exist printing out: cd: No such file or directory

which file_does_not_exist
This throws an error as the file again doesn't exist printing out: which: no such file or directory

ls > output.txt | output2.txt 
This throws a conflict because you cannot output to multiple places which the error message is: conflict: cannot output to multiple places at a time.execvp: No such
file or directory

echo Hello:
This test case show print out the word hello on the terminal using ./mysh

echo whatsup
Simmilarly, this test case prints out the word whatsup on the terminal using ./mysh

pwd
This prints out the current file directory we are at and for us it prints out /Desktop/myShell

which ls
This prints out the /usr/bin/ls, which shows us the absolute path of the ls command that we want to run

cd testDirectory
this goes into the test directory of which is a folder inside myShell which would be /Desktop/myShell/testDirectory

pwd
Again, this prints out the current file directory we are at and for us it prints out /Desktop/myShell/testDirectory

cat foo*.txt
This prints out the texts contained by foobar.txt and foobro.txt that since we are in the test directory we are able to access. Which are 
HelloOne
HelloTwo
HellThree
There 
HelloFour

cat foo*.txt | grep Hello
This prints out the texts contained by foobar.txt and foobro,txt that contain the word "Hello" again we can only access this because we are in the test directory.
These words are:
HelloOne
HelloTwo
HelloFour

cat foo*.txt | wc -l
This counts the total amount of lines that are contained in the foo*.txt files which are 5.

cd ..
This goes one directory backwards and takes us out of testDirectory and puts us into the myShell original folder

then ls
There are two arguements here. The then is succesfully run because the cd.. is succesfully run. Which allows the ls to run providing us with an output of
arraylist.h  file.txt  foobar.txt  input.txt  Makefile  mysh  mysh2.c  mysh.c  mysh.o  myshOne.txt  output2.txt  output.txt  test2.sh  testDirectory  test.txt

else pwd
Due to the fact that then ls ran, the else will not run and it provides an error message of:
Error: Last command was not failed

ls > output.txt
This puts the ls into our output file and this is inputted into output.txt:
arraylist.h  file.txt  foobar.txt  input.txt  Makefile  mysh  mysh2.c  mysh.c  mysh.o  myshOne.txt  output2.txt  output.txt  test2.sh  testDirectory  test.txt

cat input.txt | grep hello
This prints out the lines from input.txt that have the word hello in it which are:
hello there hi how are you
hello what is up

ps aux | grep mysh
This prints out:
rrp144   2678876  0.0  0.0   2780  1212 ?        Ss   19:20   0:00 /common/home/rrp144/Desktop/cs214/Assignment03/P3/src/mysh
nv274    2937330  0.0  0.0  17880  5592 pts/9    S+   21:14   0:00 nano mysh.c
jod17    3128716  0.0  0.0   2780   936 pts/59   S+   22:11   0:00 ./mysh
jjl330   3180131  0.0  0.0 21474908896 5804 pts/32 T  22:19   0:00 ./mysh
pvk14    3188846  0.0  0.0   2852  1620 pts/41   S+   22:21   0:00 ./mysh test.txt
pvk14    3188862  0.0  0.0   6800  2468 pts/41   S+   22:21   0:00 grep mysh

cat < input.txt | grep hello
This prints out which are the lines from the input.txt with the word hello in it
hello there hi how are you
hello what is up

cat input.txt | grep hello > output2.txt
This puts into the output2.txt the lines with the word hello in it so the following lines are written on output2.txt
hello there hi how are you
hello what is up

cat < input.txt > output.txt 
The input.txt file is fully printed out on output.txt.

exit hellOne hellTwo
This exits the process with the words HellOne and HellTwo and exits the shell.
