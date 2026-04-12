#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[])
{
    // open socket
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9999);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // connect to server
    if (connect(client, (struct sockaddr *)&server_addr, sizeof(server_addr)))
    {
        perror("Connect to server error: ");
        exit(1);
    }

    char message[256];
    int n = recv(client, message, sizeof(message), 0);
    if (n == -1)
    {
        printf("received message error\n");
    }
    else
    {
        message[n] = 0;
        printf("Received message: %s\n", message);
    }

    char *request = "Hello Server";
    send(client, request, strlen(request), 0);

    request = "Hi";
    send(client, request, strlen(request), 0);

    close(client);
}