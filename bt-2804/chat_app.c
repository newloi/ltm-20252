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
    if (argc < 4)
    {
        fprintf(stderr, "Usage: %s <port> <remote_ip> <remote_port>\n", argv[0]);
        exit(1);
    }

    // open socket UDP
    int chat_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (setsockopt(chat_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)))
    {
        perror("setsockopt() failed");
        close(chat_socket);
        exit(1);
    }

    // address IPv4 to receive message
    struct sockaddr_in src_addr;
    src_addr.sin_family = AF_INET;
    src_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    src_addr.sin_port = htons(atoi(argv[1]));
    int src_addr_len = sizeof(src_addr);

    // bind socket with address
    if (bind(chat_socket, (struct sockaddr *)&src_addr, sizeof(src_addr)))
    {
        perror("Bind socket error: ");
        exit(1);
    }

    // address IPv4 to send message
    struct sockaddr_in dst_addr;
    dst_addr.sin_family = AF_INET;
    dst_addr.sin_addr.s_addr = inet_addr(argv[2]);
    dst_addr.sin_port = htons(atoi(argv[3]));

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
                       (struct sockaddr *)&dst_addr, sizeof(dst_addr));
                fflush(stdin);
            }
        }
    }

    // close socket
    close(chat_socket);

    return 0;
}