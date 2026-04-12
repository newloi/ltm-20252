#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    // open socket
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // server address IPv4
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);

    int opt = 1;
    // Thiết lập SO_REUSEADDR để giải phóng cổng ngay khi server tắt
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // bind socket
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("Bind socket error: ");
        exit(1);
    }

    // max 5 connection
    listen(listener, 5);

    // accept client
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    int client = accept(listener, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client > -1)
    {
        int count = 0;
        const char *target = "0123456789";
        int target_len = strlen(target);
        char message[1024];
        char *buffer = NULL;
        int len = 0, search_idx = 0;
        while (1)
        {
            // receive message from client;
            int n = recv(client, message, sizeof(message), 0);
            if (n > 0)
            {
                // take more memory for buffer
                buffer = (char *)realloc(buffer, len + n + 1);
                // add message into buffer
                memcpy(buffer + len, message, n);
                // update buffer length
                len += n;
                buffer[len] = 0;

                // find target (start at previous search)
                char *tmp = buffer + search_idx;
                while ((tmp = strstr(tmp, target)) != NULL)
                {
                    count++;
                    tmp += target_len;
                    search_idx = tmp - buffer;
                }
                printf("Count = %d\n", count);
            }
        }
    }

    // close socket
    close(listener);
    close(client);
}