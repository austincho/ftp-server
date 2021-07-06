#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "helpers.h"
#include "dir.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "netdb.h"

int loggedin = 0;
int passiveclientd = -1;
int passive_status = 0;
int pasvd = -1;

int sendToClient(int clientd, char *msg)
{
    if (send(clientd, msg, strlen(msg), 0) == -1)
    {
        perror("Failed to send to the socket");
        return 0;
    }
    return 1;
}

int cmdUser(char *arg0, int clientd)
{
    char msg[1024];
    if (loggedin == 1)
    {
        strcpy(msg, "230 User logged in, proceed.\r\n");
    }
    else
    {
        if (arg0 == NULL){
            strcpy(msg, "530: Not logged in. Username not provided.\r\n");
        }
        else if (strcmp(arg0, "cs317") != 0)
        {
            strcpy(msg, "530: Not logged in. Incorrect username.\r\n");
        }
        else
        {
            loggedin = 1;
            printf("Have logged user in\r\n");
            strcpy(msg, "230: User logged in, proceed.\r\n");
        }
    }

    sendToClient(clientd, msg);
    return 1;
}

int cmdQuit(int clientd)
{
    if (loggedin == 1)
    {
        loggedin = 0;
        printf("Have logged user out\r\n");
    }
    char msg[1024];
    strcpy(msg, "221: Service closing control connection. User logged out.\r\n");
    sendToClient(clientd, msg);
    close(clientd);
    return 0;
}

int cmdInvalid(int clientd)
{
    char msg[1024];
    strcpy(msg, "500: Error. Command not supported.\r\n");
    sendToClient(clientd, msg);
    return 0;
}

int cmdInitialize(int clientd)
{
    char msg[1024];
    strcpy(msg, "220: Service ready for new user. Initiate login sequence.\r\n");
    sendToClient(clientd, msg);
    return 0;
}

int cmdType(char *arg0, int clientd)
{
    char msg[1024];

    if (loggedin == 0){
        strcpy(msg, "530 Error. User is not logged in, please login first.\r\n");
        sendToClient(clientd, msg);
        return 1;
    }

    if (arg0 == NULL)
    {
        strcpy(msg, "500 Error. TYPE command needs an argument.\r\n");
    }
    else if (strcasecmp(arg0, "I") == 0)
    {
        strcpy(msg, "200 Command okay. Switching to Binary mode.\r\n");
    }
    else if (strcasecmp(arg0, "A") == 0)
    {
        strcpy(msg, "200 Command okay. Switching to ASCII mode.\r\n");
    }
    else
    {
        strcpy(msg, "450 Error. Other data types not supported.\r\n");
    }

    sendToClient(clientd, msg);
    return 1;
}

_Bool startsWith(const char *string, const char *startString){
    if(strncmp(string, startString, strlen(startString)) == 0) return 1;
    return 0;
}

int cmdCWD(char* arg0, int clientd){
    char msg[1024];

    if (loggedin == 0){
        strcpy(msg, "530 Error. User is not logged in, please login first.\r\n");
        sendToClient(clientd, msg);
        return 1;
    }

    if (arg0 == NULL)
    {
        strcpy(msg, "500 Error. CWD command needs an argument.\r\n");
    } else {
        if (!(startsWith(arg0, "./") || startsWith(arg0, "../") ||  startsWith(arg0, "~/") || strstr(arg0, "..") != NULL)){
            if (chdir(arg0) == 0){
                strcpy(msg, "200 Changing directory.\r\n");
            } else {
                strcpy(msg, "Directory does not exist. \r\n");
            }
        } else {
            strcpy(msg, "550 Error. Parameter starts with ./ or ../ or contains '..' .\r\n");
        }
    sendToClient(clientd, msg);
    return 1;
    };
}

int cmdCDUP(char* arg0, int clientd) {
    char msg[1024];

    printf(arg0);

    if (loggedin == 0){
        strcpy(msg, "530 Error. User is not logged in, please login first.\r\n");
        sendToClient(clientd, msg);
        return 1;
    }

    char* buf = calloc(1024, sizeof(char));
    buf = getcwd(buf, 1024);

    // Check root directory
    if (strcmp(arg0, buf) == 0) {
        strcpy(msg, "550 Error. Failed to change directory.\r\n");
        free(buf);
        sendToClient(clientd, msg);
    } else {
    if (chdir("..") == -1){
        strcpy(msg, "550 Error. Failed to change directory.\r\n");
    } else {
        strcpy(msg, "250 CWD command successful.\r\n");
    }

    free(buf);
    sendToClient(clientd, msg);
    }
}

void *pasvhelper(void *args)
{
    int pasvd = *(int *)args;
    char msg[1024];

    if (listen(pasvd, 4) != 0)
    {
        perror("Failed to listen for connections");

        return NULL;
    }

    // Accept the connection
    struct sockaddr_in clientAddress;

    socklen_t clientAddressLength = sizeof(struct sockaddr_in);

    printf("Client should be connecting to this passive port any second now\n");

    passiveclientd = accept(pasvd, (struct sockaddr*) &clientAddress, &clientAddressLength);

    if (passiveclientd < 0)
    {
        perror("Failed to accept the client connection");
        return NULL;
    }
    printf("Accepted the client connection from %s:%d.\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
    sendToClient(passiveclientd, msg);
}

int cmdMode(char *arg0, int clientd)
{
    char msg[1024];

    if (loggedin == 0)
    {
        strcpy(msg, "530 Error. User is not logged in, please login first.\r\n");
        sendToClient(clientd, msg);
        return 1;
    }

    if (arg0 == NULL)
    {
        strcpy(msg, "501 Argument error. MODE command needs an argument.\r\n");
    }
    else if (strcasecmp(arg0, "S") == 0)
    {
        strcpy(msg, "200 Command okay. Stream mode set - Data transmitted as a stream of Bytes.\r\nn");
    }
    else if (strcasecmp(arg0, "B") == 0 || strcasecmp(arg0, "C") == 0)
    {
        strcpy(msg, "500 Mode error. Block and Compressed data modes not supported.\r\n");
    }
    else
    {
        strcpy(msg, "504 Error. Data mode in argument does not exist.\r\n");
    }
    sendToClient(clientd, msg);
    return 1;
}

int cmdStru(char *arg0, int clientd)
{
    char msg[1024];

    if (loggedin == 0)
    {
        strcpy(msg, "530 Error. User is not logged in, please login first.\r\n");
        sendToClient(clientd, msg);
        return 1;
    }

    if (arg0 == NULL)
    {
        strcpy(msg, "501 Argument error. STRU command needs an argument.\r\n");
    }
    else if (strcasecmp(arg0, "F") == 0)
    {
        strcpy(msg, "200 Command okay. Data structure set to File structure.\r\n");
    }
    else if (strcasecmp(arg0, "R") == 0 || strcasecmp(arg0, "P") == 0)
    {
        strcpy(msg, "504 Mode error. Record and Page data structures not supported.\r\n");
    }
    else
    {
        strcpy(msg, "501 Error. Data structure in argument does not exist.\r\n");
    }
    sendToClient(clientd, msg);
    return 1;
}

int cmdPasv(char *arg0, int clientd)
{
    char msg[1024];

    if (loggedin == 0)
    {
        strcpy(msg, "530 Error. User is not logged in, please login first.\n");
        sendToClient(clientd, msg);
        return 1;
    }

    if (arg0 != NULL)
    {
        strcpy(msg, "501 Argument error. PASV command does not require arguments.\n");
        sendToClient(clientd, msg);
        return 1;
    }

    // Create a passive socket for client to connect to
    int pasvd = socket(PF_INET, SOCK_STREAM, 0);

    if (pasvd < 0)
    {
        perror("Failed to create the socket.");
        return -1;
    }

    // Reuse the address
    int value = 1;
    if (setsockopt(pasvd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(int)) != 0)
    {
        perror("Failed to set the socket option");

        return -1;
    }

    // Bind the socket to a port
    struct sockaddr_in address;
    bzero(&address, sizeof(struct sockaddr_in));
    address.sin_family = AF_INET;
    address.sin_port = 0;
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(pasvd, (const struct sockaddr *)&address, sizeof(struct sockaddr_in)) != 0)
    {
        perror("Failed to bind the socket");
        return -1;
    }

    // Get socket name to get port number
    struct sockaddr_in real_addr;
    socklen_t real_addr_len = sizeof(real_addr);
    getsockname(pasvd, (struct sockaddr *)&real_addr, &real_addr_len);
    int pasv_port = (int)ntohs(real_addr.sin_port);

    // Get host name for client to connect to
    char hostbuffer[1024];
    char *IPbuffer;
    struct hostent *host_entry;
    int hostname;

    // To retrieve hostname
    hostname = gethostname(hostbuffer, sizeof(hostbuffer));

    // To retrieve host information
    host_entry = gethostbyname(hostbuffer);

    // To convert an Internet network
    // address into ASCII string
    IPbuffer = inet_ntoa(*((struct in_addr *)host_entry->h_addr_list[0]));

    printf("Port for client to connect to: %d\r\n", pasv_port);
    printf("Host IP for client to connect to: %s\r\n", IPbuffer);

    char *a = strtok(IPbuffer, ".\r\n");
    char *b = strtok(NULL, ".\r\n");
    char *c = strtok(NULL, ".\r\n");
    char *d = strtok(NULL, ".\r\n");

    printf("6 numbers to be sent to client (%s,%s,%s,%s,%d,%d).\r\n", a, b, c, d, pasv_port / 256, pasv_port % 256);

    sprintf(msg, "227 Entering Passive Mode (%s,%s,%s,%s,%d,%d).\r\n", a, b, c, d, pasv_port / 256, pasv_port % 256);

    pthread_t pasvthread;

    if (pthread_create(&pasvthread, NULL, pasvhelper, &pasvd) != 0)
    {
        perror("Failed to create the thread");
        return -1;
    }
    passive_status = 1;
    sendToClient(clientd, msg);
    return 1;
}

int cmdRetr(char* arg0, int clientd)
{
    char msg[1024];
    FILE* file;
    int blockSize;
    int fileSize;
    char fileBuffer[1024 * 2];

    if (loggedin == 0)
    {
        strcpy(msg, "530 Error. User is not logged in, please login first.\r\n");
        sendToClient(clientd, msg);
        return 1;
    }
    else if (arg0 == NULL)
    {
        strcpy(msg, "501 Argument error. Provide a path to the file.\r\n");
        sendToClient(clientd, msg);
        return 1;
    }
    else
    {
        if (access(arg0, R_OK) != -1){
            file = fopen(arg0, "rb");
            fseek(file, 0L, SEEK_END);
            fileSize = ftell(file);
            fseek(file, 0L, SEEK_SET);

            bzero(msg, sizeof(msg));
            sprintf(msg, "150 File status okay; about to open data connection. Opening BINARY mode data connection for %s (%d bytes).\r\n", arg0, fileSize);
            sendToClient(clientd, msg);
            bzero(msg, sizeof(msg));

            bzero(fileBuffer, sizeof(fileBuffer));
    
                while((blockSize = fread(fileBuffer, sizeof(char), sizeof(fileBuffer), file)) > 0) 
                {
                    write(passiveclientd, fileBuffer, blockSize);
                    bzero(fileBuffer,sizeof(fileBuffer));
                }
            fclose(file);
            close(passiveclientd);
            passiveclientd = -1;

            bzero(msg, sizeof(msg));
            strcpy(msg, "226 Closing data connection. Transfer Complete. \r\n");
            sendToClient(clientd, msg);
        }
        else 
        {
            bzero(msg, sizeof(msg));
            strcpy(msg, "550 Requested action not taken, file not found or no access.\r\n");
            sendToClient(clientd, msg);
        }
    }
}

int cmdNlst(char *arg0, int clientd)
{
    char msg[1024];
    char path[1024];

    if (loggedin == 0)
    {
        strcpy(msg, "530 Error. User is not logged in, please login first.\n");
        sendToClient(clientd, msg);
        return 1;
    }

    if (arg0 != NULL)
    {
        strcpy(msg, "501 Syntax error. NLST command does not require arguments.\n");
        sendToClient(clientd, msg);
        return 1;
    }
    else
    {
        while(passiveclientd == -1){
        }
        getcwd(path, 1024);
        if (passiveclientd != -1)
        {
            strcpy(msg, "150 Here comes the directory listing.\r\n");
            sendToClient(clientd, msg);
            listFiles(passiveclientd, path);
            close(passiveclientd);
            close(pasvd);
            passiveclientd = -1;
            pasvd = -1;
            bzero(msg, sizeof(msg));
            strcpy(msg, "226 NLST successful. Closing data connection and exiting passive mode.\r\n");
        }
        else 
        {
            printf("File descriptor = -1 error");
            bzero(msg, sizeof(msg));
            strcpy(msg, "534 NLST unsuccessful. File descriptor error.\r\n");
        }
    }
    sendToClient(clientd, msg);
    return 1;
}
