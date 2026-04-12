#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <sys/types.h> // định nghĩa các kiểu dữ liệu hệ thống
#include <arpa/inet.h> // các hàm chuyển đổi định dạng địa chỉ IP (số -> xâu, ...)
#include <netdb.h>     // DNS (getaddrinfo, struct addrinfo)

int main(int argc, char *argv[])
{
    // struct chứa thông tin về tên miền
    struct addrinfo *res, *head;
    // hàm phân giải tên miền -> return danh sách liên kết các địa chỉ IP ứng với tên miền, gán kết quả vào res
    int ret = getaddrinfo(argv[1], "http", NULL, &res); // ret = 0 -> thành công

    // kiểm tra kết quả
    if (ret != 0)
    {
        printf("getaddrinfo() failed: %s\n", gai_strerror(ret));
        exit(1);
    }

    // duyệt danh sách kết quả trả về
    head = res;
    while (head != NULL)
    {
        // họ IPv4
        if (head->ai_family == AF_INET)
        {
            printf("IPv4: ");
            // struct IPv4
            struct sockaddr_in addr;
            // sao chép dữ liệu tù vùng nhớ ai_addr (kiểu chung - sockaddr) sang addr (kiểu IPv4 - sockaddr_in)
            // tại sao dùng memcpy -> thực chất là một cách ép kiểu từ sockaddr (cha) sang sockaddr_in (con)
            memcpy(&addr, head->ai_addr, head->ai_addrlen);
            // chuyển IPv4 từ số nguyên sang xâu
            printf("%s\n", inet_ntoa(addr.sin_addr));
        }
        else if (head->ai_family == AF_INET6)
        {
            printf("IPv6: ");
            // struct IPv6
            struct sockaddr_in6 addr;
            // sao chép dữ liệu tù vùng nhớ ai_addr (kiểu chung - sockaddr) sang addr (kiểu IPv6 - sockaddr_in6)
            memcpy(&addr, head->ai_addr, head->ai_addrlen);
            // chuyển IPv6 từ số nguyên sang xâu
            char *text = malloc(INET6_ADDRSTRLEN);
            inet_ntop(AF_INET6, &addr.sin6_addr, text, INET6_ADDRSTRLEN);
            printf("%s\n", text);
            free(text);
        }

        head = head->ai_next;
    }

    // giải phóng (getaddrinfo tự động cấp phát bộ nhớ cho res)
    freeaddrinfo(res);
}