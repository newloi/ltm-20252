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
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // server address IPv4
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(9000);

    // connect to server
    if (connect(client, (struct sockaddr *)&server_addr, sizeof(server_addr)))
    {
        perror("Connect to server error: ");
        exit(1);
    }

    // packet to send to server (Sử dụng cấp phát động)
    char *packet = (char *)malloc(8192);
    if (packet == NULL)
        exit(1);
    int idx = 0;

    // get current path
    char path[512];
    getcwd(path, sizeof(path));
    int path_len = strlen(path);
    // copy path_len to packet
    memcpy(packet + idx, &path_len, sizeof(int));
    idx += sizeof(int);
    // copy path to packet
    memcpy(packet + idx, path, path_len);
    idx += path_len;

    // count file (placeholder)
    int file_count = 0;
    int count_idx = idx;
    idx += sizeof(int);

    // open current directory
    DIR *dir;
    dir = opendir(".");
    if (dir == NULL)
    {
        perror("Cannot open current directory: ");
        exit(1);
    }

    // get files in dir
    struct dirent *file;
    struct stat file_stat;
    while ((file = readdir(dir)) != NULL)
    {
        if (stat(file->d_name, &file_stat) == 0 && S_ISREG(file_stat.st_mode))
        {
            file_count++;
            int name_len = strlen(file->d_name);
            long file_size = file_stat.st_size;

            // copy name length to packet
            memcpy(packet + idx, &name_len, sizeof(int));
            idx += sizeof(int);
            // copy file name to packet
            memcpy(packet + idx, file->d_name, name_len);
            idx += name_len;
            // copy file size to packet
            memcpy(packet + idx, &file_size, sizeof(long));
            idx += sizeof(long);
        }
    }

    // close directory
    closedir(dir);

    // update file count
    memcpy(packet + count_idx, &file_count, sizeof(int));

    if (send(client, packet, idx, 0) <= 0)
    {
        printf("Send packet error: ");
        exit(1);
    }

    // free
    free(packet);
    close(client);

    return 0;
}