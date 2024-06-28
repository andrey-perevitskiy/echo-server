#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "config.h"
#include "freectx.h"

enum con_type {
    CONTYPE_TCP = 0,
    CONTYPE_UDP = 1
};

struct free_context * freectx;

int
main (int argc, char ** argv)
{
    enum con_type contype;
    int sockfd;
    struct sockaddr_in servaddr = {0};
    char * servaddr_str, buf [MAXLEN];
    fd_set rset;
    ssize_t br, bw;

    if (argc == 3) {
        if (strcmp(*(argv + 1), "--tcp") == 0)
            contype = CONTYPE_TCP;
        else if (strcmp(*(argv + 1), "--udp") == 0)
            contype = CONTYPE_UDP;
        else
            return -1;
        servaddr_str = *(argv + 2);
    }
    else if (argc == 2) {
        contype = CONTYPE_TCP;
        servaddr_str = *(argv + 1);
    }
    else {
        fprintf(stderr,
            "Usage: %s [--tcp | --udp] ipaddr\n"
            "   --tcp   Create a TCP connection to ipaddr (default)\n"
            "   --udp   Create a UDP connection to ipaddr\n", *argv);
        return -1;
    }

    freectx = freectx_alloc();
    (void) freectx_append(freectx, &sockfd, IDFD);

    servaddr.sin_family = AF_INET;
    if (inet_pton(servaddr.sin_family, servaddr_str, &servaddr.sin_addr) < 0) {
        fprintf(stderr, "inet_pton(): %s\n", strerror(errno));
        return -1;
    }
    servaddr.sin_port = htons(PORT);
    if (contype == CONTYPE_TCP)
        sockfd = socket(servaddr.sin_family, SOCK_STREAM, 0);
    else if (contype == CONTYPE_UDP)
        sockfd = socket(servaddr.sin_family, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "socket(): %s\n", strerror(errno));
        return -1;
    }
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
        exit_free(freectx, "connect(): %s\n", strerror(errno));

    FD_ZERO(&rset);
    while (1) {
        FD_SET(STDIN_FILENO, &rset);
        FD_SET(sockfd, &rset);
        (void) select(sockfd + 1, &rset, NULL, NULL, NULL);
        if (FD_ISSET(STDIN_FILENO, &rset)) {
            if ((br = read(STDIN_FILENO, buf, MAXLEN)) < 0)
                exit_free(freectx, "read(): %s\n", strerror(errno));
            else if (br == 0) {
                printf("Quiting...\n");
                shutdown(sockfd, SHUT_WR);
                break;
            }
            if ((bw = write(sockfd, buf, br)) < 0)
                exit_free(freectx, "write(): %s\n", strerror(errno));
            else if (bw == 0 && contype == CONTYPE_TCP)
                exit_free(freectx, "The connection was closed prematurely\n",
                    NULL);
        }
        if (FD_ISSET(sockfd, &rset)) {
            if ((br = read(sockfd, buf, MAXLEN)) < 0)
                exit_free(freectx, "read(): %s\n", strerror(errno));
            else if (br == 0 && contype == CONTYPE_TCP)
                exit_free(freectx, "The connection was closed prematurely\n",
                    NULL);
            write(STDOUT_FILENO, buf, bw);
        }
    }

    freectx_free(freectx);

    return 0;
}