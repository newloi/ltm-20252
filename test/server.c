#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[])
{
    // create socket type TCP
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1)
    {
        perror("Create socket error: ");
        exit(EXIT_FAILURE);
    }

    // address IPv4
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9999);              // listen at port 9000, convert host (little-endian) to network (big-endian) short
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // listen at any address in this device (0.0.0.0), convert host (little-endian) to network (big-endian) long

    // bind socket with address
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("Bind socket error: ");
        exit(EXIT_FAILURE);
    }

    // waiting for connection (max 5 connections)
    if (listen(listener, 5))
    {
        perror("Transit socket to wait phase error: ");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for client\n");

    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    // accept client connect to server
    int client = accept(listener, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client == -1)
    {
        perror("Accept client error: ");
        exit(EXIT_FAILURE);
    }
    printf("New client connected: %d", client);
    printf("Client IP: %s\n", inet_ntoa(client_addr.sin_addr));
    printf("Client port: %d\n", ntohs(client_addr.sin_port));

    // send hello message
    char *message = "Hello Client!";
    int ret = send(client, message, strlen(message), 0);
    if (ret != -1)
    {
        printf("%d bytes are sent\n", ret);
    }

    // receive message
    char request[256];
    // while (1)
    // {
    int len = recv(client, request, sizeof(request), 0);
    if (len != -1)
    {
        request[len] = 0;
        printf("%d bytes received: %s\n", len, request);
    }
    else
    {
        printf("Disconnected\n");
        exit(1);
    }
    // }

    // close socket
    close(listener);
    close(client);
}