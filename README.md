# PipeFifoSocket
Minimal communication protocol implementing some basic Linux commands
--------------
Implementing a communication protocol that allows login, searching for files, checking file stats in Linux. The purpose of this project was exercising the communication between
processes using Pipes, Fifos and Socketpairs.

- the communication is done by executing commands read from the keyboard in the parent process and executed in child processes
- the commands are strings bounded by a new line
- the responses are series of bytes prefixed by the length of the response
- the result obtained from the execution of any command will be displayed on screen by the parent process
- the minimal protocol includes the following commands:
    - "login: username" - whose existence is validated by using a configuration file
    - "myfind file" - a command that allows finding a file and displaying information associated with that file; the displayed information will contain the creation date, date of change, file size, file access rights, etc.
    - "mystat file" - a command that allows you to view the attributes of a file
    - "quit"
- no function in the "exec()" family (or other similar) will be used to implement "myfind" or "mystat" commands in order to execute some system commands that offer the same functionalities
- the communication among processes will be done using all of the following communication mechanisms: pipes, fifos, and socketpairs.
