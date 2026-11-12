#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAXLINE 65536
#define PORT 8080
#define SA struct sockaddr

int main() {
    int listenfd, connfd;
    ssize_t n;
    char buffer[MAXLINE];
    struct sockaddr_in servaddr, cli;
    socklen_t len;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    bind(listenfd, (SA*)&servaddr, sizeof(servaddr));
    listen(listenfd, 5);

    while (1) {
        len = sizeof(cli);
        connfd = accept(listenfd, (SA*)&cli, &len);
        while ((n = read(connfd, buffer, MAXLINE)) > 0) {
            write(connfd, buffer, n);
        }
        close(connfd);
    }
    close(listenfd);
    return 0;
}
