#include <sys/types.h>
#include "dir.h"
#include "usage.h"
#include "helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

// Here is an example of how to use the above function. It also shows
// one how to get the arguments passed on the command line.
int portNum;
char* startDirectory;
size_t startDirectorySize;

int execute(char* cmd, char* arg0, int clientd){

  printf("cmd: %s\n", cmd);
  printf("arg0: %s\n", arg0);

  
  if (cmd != NULL && strcasecmp(cmd, "USER") == 0){
    cmdUser(arg0, clientd);
    return 1;
  }

  if (cmd != NULL && strcasecmp(cmd, "QUIT") == 0){
    cmdQuit(clientd);
    return 1;
  }

  if (cmd != NULL && strcasecmp(cmd, "TYPE") == 0){
    cmdType(arg0, clientd);
    return 1;
  }

  if (cmd != NULL && strcasecmp(cmd, "MODE") == 0){
    cmdMode(arg0, clientd);
    return 1;
  }

  if (cmd != NULL && strcasecmp(cmd, "STRU") == 0){
    cmdStru(arg0, clientd);
    return 1;
  }

  if (cmd != NULL && strcasecmp(cmd, "CWD") == 0){
    cmdCWD(arg0, clientd);
    return 1;
  }
  if (cmd != NULL && strcasecmp(cmd, "CDUP")== 0){
    cmdCDUP(startDirectory, clientd);
    return 1;
  }
  if (cmd != NULL && strcasecmp(cmd, "PASV") == 0){
    cmdPasv(arg0, clientd);
    return 1;
  }
  if (cmd != NULL && strcasecmp(cmd, "RETR") == 0){
    cmdRetr(arg0, clientd);
    return 1;
  }
  if (cmd != NULL && strcasecmp(cmd, "NLST") == 0){
    cmdNlst(arg0, clientd);
    return 1;
  }
  
  cmdInvalid(clientd);
  return 0;
}

void* interact(void* args)
{
    int clientd = *(int*) args;
    
    // Interact with the client
    char buffer[1024];
    
    while (true)
    {
        bzero(buffer, 1024);
        
        // Receive the client message
        ssize_t length = recv(clientd, buffer, 1024, 0);
        
        if (length < 0)
        {
            perror("Failed to read from the socket");
            
            break;
        }
        
        if (length == 0)
        {
            printf("EOF\n");
            
            break;
        }
        
        char *cmd;
        char *arg0;
        // Extract cmd and it's arg 
        cmd = strtok(buffer, " \r\n");
        arg0 = strtok(NULL, " \r\n");

        execute(cmd, arg0, clientd);
    }
    cmdQuit(clientd);
    
    return NULL;
}

int main(int argc, char *argv[]) {
  // Find Root Directory for CDUP
  char* buf = calloc(8196, sizeof(char));
  buf = getcwd(buf, 8196);
  startDirectorySize = strlen(buf);
  startDirectory = malloc(sizeof(char) * startDirectorySize);
  startDirectory = strcpy(startDirectory, buf);
  free(buf);
    
    // Check the command line arguments
    if (argc != 2) {
      usage(argv[0]);
      return -1;
    }

    portNum = atoi(argv[1]);
    printf("portNum: %d\n", portNum);

    if (portNum < 1024 || portNum > 65535) {
      usage(argv[0]);
      return -1;
    }

    // Create a TCP socket
    int socketd = socket(PF_INET, SOCK_STREAM, 0);
    
    if (socketd < 0)
    {
        perror("Failed to create the socket.");
        exit(-1);
    }

    // Reuse the address
    int value = 1;
    if (setsockopt(socketd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(int)) != 0)
    {
        perror("Failed to set the socket option");
        
        exit(-1);
    }

    // Bind the socket to a port
    struct sockaddr_in address;
    
    bzero(&address, sizeof(struct sockaddr_in));
    
    address.sin_family = AF_INET;
    
    address.sin_port = htons(portNum);
    
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    
    if (bind(socketd, (const struct sockaddr*) &address, sizeof(struct sockaddr_in)) != 0)
    {
        perror("Failed to bind the socket");
        exit(-1);
    }
    
    // Set the socket to listen for connections
    if (listen(socketd, 4) != 0)
    {
        perror("Failed to listen for connections");
        
        exit(-1);
    }
    
    while (true)
    {
        // Accept the connection
        struct sockaddr_in clientAddress;
        
        socklen_t clientAddressLength = sizeof(struct sockaddr_in);
        
        printf("Waiting for incomming connections...\n");
        
        // int clientd = accept(socketd, (struct sockaddr*) &clientAddress, &clientAddressLength);
        int clientd = accept(socketd, NULL, NULL);
        
        if (clientd < 0)
        {
            perror("Failed to accept the client connection");
            
            continue;
        }
        
        printf("Accepted the client connection from %s:%d.\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
        
        // Create a separate thread to interact with the client
        pthread_t thread;

        cmdInitialize(clientd);
        
        if (pthread_create(&thread, NULL, interact, &clientd) != 0)
        {
            perror("Failed to create the thread");
            
            continue;
        }
        
        // The main thread just waits until the interaction is done
        pthread_join(thread, NULL);
        
        printf("Interaction thread has finished.\n");
    }
    free(startDirectory);
    return 0;
}
