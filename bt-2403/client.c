#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

int main()
{
    // open socket
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client == -1)
    {
        perror("Open socket error: ");
        exit(1);
    }

    // server address IPv4
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(9000);

    // connect to server
    if (connect(client, (struct sockaddr *)&server_addr, sizeof(server_addr)))
    {
        perror("Connect to server error: ");
        exit(1);
    }

    char message[1024];
    while (1)
    {
        printf("Message (Type): ");
        // enter message
        scanf("%s", message);
        // send message to server
        send(client, message, strlen(message), 0);
    }

    // close socket
    close(client);
}