## What the program does.

This is a simple network client-server implementation that allows a user to connect to a remote server and execute Linux shell commands on the remote server. The server (server_udp )listens for incoming connections and creates a new thread for each connected client. The client can execute the following commands:

~ connect [hostname]: connect to the remote server.
~ disconnect: disconnect from the remote server.
~ exec [Linux shell command]: execute a Linux shell command on the remote server.
~ help: show the commands supported in the client shell.
~ quit: quit the client shell.

This server uses UDP sockets for communication and sends the output of the executed command back to the client. IIt also limits the number of connected clients to four.

The program defines a **client_t** struct that stores the socket file descriptor, address, and client handle (a random number between 0 and MAX_CLIENTS). The **handle_client function** is the entry point for each client thread, which handles incoming commands from the client and executes them on the remote server.

The **handle_command_execution** function executes a Linux shell command using the *execvp* system call and sends the output of the command back to the client using the *sendto* function. The output of the command is read from a pipe that is created using the pipe system call, and the child process that is created using the *fork* system call writes to the *pipe* and the parent process reads from it.

The **handle_connect function** resolves the IP address of the remote server using the **gethostbyname** function and stores it in the **client_t** struct. The **handle_disconnect** function sets the handle field of the client_t struct to -1 to indicate that the client is not connected.

The **handle_help function** prints a list of supported commands to the console.

The **start_client_thread** function creates a new thread to handle a connected client and passes the socket file descriptor as an argument.

The main function creates a socket using the socket system call and binds it to a local address using the bind system call. It then listens for incoming connections using the listen system call and accepts incoming connections using the accept system call. For each accepted connection, it calls the **start_client_thread** function to create a new thread to handle the client. The main thread continues to listen for incoming connections until it is terminated.

## What functionalities the server implements

The server code is a concurrent UDP-based server that can handle up to 4 clients at the same time. If a 5th client tries to connect, it will be rejected.

When a client successfully connects, the server spawns a thread to service that client, and the client receives a handle from the server that it can use as a reference in all future requests.

When a connected client requests a command execution or disconnection, the server daemon checks whether the client's handle is consistent with its port. If a connected client requests a command that is not supported by Linux, the server daemon reports the error back to the client number and inet address associated. If not, the request will be rejected.

The server daemon executes client-requested Linux commands using fork() and execvp() system calls. It spawns a pthread to service the request, forks a new process, uses execvp() system calls to execute the client command, captures the result of the execution, and delivers it back to the client.

The client shell allows users to access the remote server using the commands "help", "connect [hostname]", "disconnect [hostname]", "[normal Linux shell command]", and "quit". The client shell will report an error message if the connection is not established at the time the user enters a command.

All data communications between the server and the client are based on UDP datagrams, and the implementation does not need to deal with transmission errors, assuming the server and the client are sitting on the same LAN.