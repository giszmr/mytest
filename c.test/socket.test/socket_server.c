#include <stdio.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>

int main(int argc, char* argv[])
{
    non_block_connect();
    return 0;
}

int non_block_connect()
{
    int retval = 0;
    int socketHandle = 0;
    fd_set writefds;
    struct timeval tv;
    struct sockaddr_in servaddr;
    int nonblockMode = 1;
    int flags;

    tv.tv_sec  = 5;
    tv.tv_usec = 0;

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port   = htons(8000);

    printf("zhumengri1\n");
    socketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    bind(socketHandle, )
    retval = connect(socketHandle, (struct sockaddr*)&servaddr, sizeof(servaddr));

    printf("zhumengri3\n");
    if(retval == -1){
        if(errno == EINPROGRESS)
            printf("connecting...\n");
        else
            printf("connection failed\n");
    }
    printf("retval = %d\n", retval);

    FD_ZERO(&writefds);
    FD_SET(socketHandle, &writefds);
    retval = select(socketHandle+1, 0, &writefds, 0, &tv);
    printf("retval=%d\n", retval);

}



int set_tcp_keep_alive(int fd, int start, int interval, int count)
{
  int keepAlive = 1;
  if (fd < 0 || start < 0 || interval < 0 || count < 0)
    return -1;

  //Start HeartBeat
  if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepAlive, sizeof(keepAlive)) == -1)
  {
    perror("setsockopt");
    return -1;
  }

  //Set the time between starting heartbeat and sending the first detecting package
  if (setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, (void*)&start, sizeof(start)) == -1)
  {
    perror("setsockopt");
    return -1;
  }


}
