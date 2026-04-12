#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[])
{
    // open socket TCP
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // server address IPv4
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));

    // connect to server
    if (connect(client, (struct sockaddr *)&server_addr, sizeof(server_addr)))
    {
        perror("Connect to server error: ");
        exit(1);
    }

    // receive message from server
    char response[100];
    if (recv(client, response, sizeof(response), 0) > 0)
    {
        printf("Received: %s\n", response);
    }

    // enter the message
    char message[100];
    while (1)
    {
        printf("Message (type): ");
        fgets(message, sizeof(message), stdin);

        // send to server
        if (send(client, message, sizeof(message), 0) == -1)
        {
            printf("Cannot send message to server.\n");
        }
    }

    // close socket
    close(client);

    return 0;
}