#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    // open socket UDP
    int server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // address IPv4
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9001);

    // bind socket with address
    if (bind(server_socket, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("Bind socket error: ");
        exit(1);
    }

    char message[2048];
    struct sockaddr_in sender_addr;
    int sender_addr_len = sizeof(sender_addr);
    while (1)
    {
        // received message from sender
        int n = recvfrom(server_socket, message, sizeof(message), 0, (struct sockaddr *)&sender_addr, &sender_addr_len);
        if (n > 0)
        {
            // turn back message to sender
            message[n] = 0;
            printf("Received: %s\n", message);
            sendto(server_socket, message, n, 0, (struct sockaddr *)&sender_addr, sender_addr_len);
        }
    }

    // close socket
    close(server_socket);
}