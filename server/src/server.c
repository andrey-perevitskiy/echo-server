#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "config.h"
#include "freectx.h"

struct free_context * freectx;

static void
sigchld_handler (int signo)
{
    (void) signo;
    int len;
    char buf [MAXLEN];

    while (waitpid(-1, NULL, WNOHANG) > 0) {
        len = snprintf(buf, MAXLEN, "Child process was exited\n");
        write(STDOUT_FILENO, buf, len);
    }
}

static void
sigint_handler (int signo)
{
    (void) signo;
    int len;
    char buf [MAXLEN];

    len = snprintf(buf, MAXLEN, "SIGINT catched\n");
    write(STDOUT_FILENO, buf, len);

    // By default, after SIGINT, -- all the resources are freeing by the kernel
    exit(EXIT_FAILURE);
}

int
main (void)
{
    int listenfd, udpfd, readyfdq, connfd;
    struct sockaddr_in servaddr = {0}, cliaddr;
    socklen_t cliaddr_len;
    fd_set rset;
    struct sigaction sigchld_action, sigint_action;
    char buf [MAXLEN], cliaddr_str [INET_ADDRSTRLEN + 1] = {0};
    ssize_t br, bw;
    pid_t pid;

    freectx = freectx_alloc();
    (void) freectx_append(freectx, &listenfd, IDFD);
    (void) freectx_append(freectx, &udpfd, IDFD);
    (void) freectx_append(freectx, &connfd, IDFD);

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
    listenfd = socket(servaddr.sin_family, SOCK_STREAM, 0);
    if (listenfd < 0) {
        fprintf(stderr, "socket(): %s\n", strerror(errno));
        return -1;
    }
    if (setsockopt(
            listenfd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &(int) {1},
            sizeof(int)) < 0)
        exit_free(freectx, "setsockopt(): %s\n", strerror(errno));
    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
        exit_free(freectx, "bind(): %s\n", strerror(errno));
    if (listen(listenfd, LISTENQ) < 0)
        exit_free(freectx, "listen(): %s\n", strerror(errno));

    // Create a UDP socket
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
    udpfd = socket(servaddr.sin_family, SOCK_DGRAM, 0);
    if (udpfd < 0)
        exit_free(freectx, "socket(): %s\n", strerror(errno));
    if (bind(udpfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
        exit_free(freectx, "bind(): %s\n", strerror(errno));

    sigchld_action.sa_handler = sigchld_handler;
    sigemptyset(&sigchld_action.sa_mask);
    sigaction(SIGCHLD, &sigchld_action, NULL);
    sigint_action.sa_handler = sigint_handler;
    sigemptyset(&sigint_action.sa_mask);
    sigaction(SIGINT, &sigint_action, NULL);
    FD_ZERO(&rset);
    while (1) {
        FD_SET(listenfd, &rset);
        FD_SET(udpfd, &rset);
        if ((readyfdq = select(udpfd + 1, &rset, NULL, NULL, NULL)) < 0) {
            if (errno == EINTR)
                continue;
            else
                exit_free(freectx, "select(): %s\n", strerror(errno));
        }
        if (readyfdq > 0)
            printf("%d FDs are available for reading\n", readyfdq);
        if (FD_ISSET(listenfd, &rset)) {
            cliaddr_len = sizeof(cliaddr);
            if ((connfd = accept(
                    listenfd,
                    (struct sockaddr *) &cliaddr,
                    &cliaddr_len)) < 0) {
                if (errno == EINTR)
                    continue;
                else
                    exit_free(freectx, "accept(): %s\n", strerror(errno));
            }
            if (inet_ntop(
                    servaddr.sin_family,
                    &cliaddr.sin_addr,
                    cliaddr_str,
                    INET_ADDRSTRLEN) < 0)
                exit_free(freectx, "inet_ntop(): %s\n", strerror(errno));
            printf("A new connection from %s was arrived\n", cliaddr_str);
            pid = fork();
            if (pid == 0)
                close(listenfd);
            while (pid == 0) {
                if ((br = read(connfd, buf, MAXLEN)) < 0)
                    exit_free(freectx, "[%s]: read(): %s\n", cliaddr_str,
                        strerror(errno));
                else if (br == 0)
                    exit_free(freectx,
                        "[%s]: The connection was closed prematurely\n",
                        cliaddr_str);
                else
                    printf("[%s]: (<--) %ld bytes were read\n", cliaddr_str,
                        br);
                if ((bw = write(connfd, buf, br)) < 0)
                    exit_free(freectx, "[%s]: write(): %s\n", cliaddr_str,
                        strerror(errno));
                else if (bw == 0)
                    exit_free(freectx,
                        "[%s]: The connection was closed prematurely\n",
                        cliaddr_str);
                else
                    printf("[%s]: (-->) %ld bytes were sent\n", cliaddr_str,
                        bw);
            }
            close(connfd);
        }
        if (FD_ISSET(udpfd, &rset)) {
            cliaddr_len = sizeof(cliaddr);
            if ((br = recvfrom(
                    udpfd,
                    buf,
                    MAXLEN,
                    0,
                    (struct sockaddr *) &cliaddr,
                    &cliaddr_len)) < 0)
                exit_free(freectx, "recvfrom(): %s\n", strerror(errno));
            if (inet_ntop(
                    servaddr.sin_family,
                    &cliaddr.sin_addr,
                    cliaddr_str,
                    INET_ADDRSTRLEN) < 0)
                exit_free(freectx, "inet_ntop(): %s\n", strerror(errno));
            printf("(<--) Received %ld bytes from %s\n", br, cliaddr_str);
            if ((bw = sendto(
                    udpfd,
                    buf,
                    br,
                    0,
                    (struct sockaddr *) &cliaddr,
                    cliaddr_len)) < 0)
                exit_free(freectx, "sendto(): %s\n", strerror(errno));
            printf("(-->) Sent %ld bytes to %s\n", bw, cliaddr_str);
        }
    }
    close(listenfd);
    close(udpfd);

    freectx_free(freectx);

    return 0;
}