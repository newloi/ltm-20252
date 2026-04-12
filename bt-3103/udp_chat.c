#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/select.h>

int main(int argc, char *argv[])
{
    // open socket UDP
    int chat_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (setsockopt(chat_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)))
    {
        perror("setsockopt() failed");
        close(chat_socket);
        exit(1);
    }

    // address IPv4 to receive message
    struct sockaddr_in recv_addr;
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    recv_addr.sin_port = htons(atoi(argv[1]));
    int recv_addr_len = sizeof(recv_addr);

    // bind socket with address
    if (bind(chat_socket, (struct sockaddr *)&recv_addr, sizeof(recv_addr)))
    {
        perror("Bind socket error: ");
        exit(1);
    }

    // address IPv4 to send message
    struct sockaddr_in send_addr;
    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = inet_addr(argv[2]);
    send_addr.sin_port = htons(atoi(argv[3]));

    char message[2048];
    char buf[2048];
    fd_set readfds;
    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(chat_socket, &readfds);

        if (select(chat_socket + 1, &readfds, NULL, NULL, NULL) < 0)
        {
            perror("select() error");
            break;
        }

        // receive message
        if (FD_ISSET(chat_socket, &readfds))
        {
            int n = recvfrom(chat_socket, message, sizeof(message), 0, NULL, NULL);
            if (n > 0)
            {
                message[n] = 0;
                printf("Received: %s", message);
                fflush(stdout);
            }
        }

        // send message
        if (FD_ISSET(STDIN_FILENO, &readfds))
        {
            if (fgets(buf, sizeof(buf), stdin))
            {
                sendto(chat_socket, buf, strlen(buf), 0,
                       (struct sockaddr *)&send_addr, sizeof(send_addr));
                fflush(stdin);
            }
        }
    }

    // close socket
    close(chat_socket);

    return 0;
}