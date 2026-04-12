#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{
    // open socket
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1)
    {
        perror("Open socket error: ");
        exit(1);
    }

    // address IPv4
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);

    // bind socket with address
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("Bind socket error: ");
        exit(1);
    }

    // max 5 connection
    listen(listener, 5);

    char *packet = (char *)malloc(4096);
    if (packet == NULL)
    {
        perror("Malloc error");
        exit(1);
    }

    int idx = 0;
    int client = accept(listener, NULL, NULL);
    if (client != -1)
    {
        int n = recv(client, packet, 4096, 0);
        if (n > 0)
        {
            // get path length
            int path_len;
            if (idx + sizeof(int) <= n)
            {
                memcpy(&path_len, packet + idx, sizeof(int));
                idx += sizeof(int);

                // get path
                char path[1024];
                if (idx + path_len <= n && path_len < 1024)
                {
                    memcpy(path, packet + idx, path_len);
                    idx += path_len;
                    path[path_len] = 0;

                    printf("%s\n", path);
                }
            }

            // get file count;
            int file_count = 0;
            if (idx + sizeof(int) <= n)
            {
                memcpy(&file_count, packet + idx, sizeof(int));
                idx += sizeof(int);
            }

            // get files
            for (int i = 0; i < file_count; i++)
            {
                // name length
                int name_len;
                if (idx + sizeof(int) > n)
                    break;
                memcpy(&name_len, packet + idx, sizeof(int));
                idx += sizeof(int);

                // file name
                char file_name[256];
                if (idx + name_len <= n && name_len < 256)
                {
                    memcpy(file_name, packet + idx, name_len);
                    idx += name_len;
                    file_name[name_len] = 0;
                }

                // file size
                long file_size;
                if (idx + sizeof(long) <= n)
                {
                    memcpy(&file_size, packet + idx, sizeof(long));
                    idx += sizeof(long);

                    printf("%s - %ld bytes\n", file_name, file_size);
                }
            }
        }
    }

    // free
    free(packet);
    close(listener);
    close(client);

    return 0;
}