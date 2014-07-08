#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>

#define buf (10000)


int main(int argc, char **argv)
{
  int s = socket(PF_INET, SOCK_DGRAM, 0);
  unsigned int host = 50000;
  char data[buf];
  fd_set read_fd;

  int fd = open ("/dev/dsp", O_WRONLY, 0644);
  if ( fd == -1) {
    printf("error(open)\n");
    return 0;
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(host);
  addr.sin_addr.s_addr = INADDR_ANY;

  unsigned int addrlen = sizeof(addr); 

  bind (s, (struct sockaddr *)&addr, sizeof(addr));

  FD_ZERO(&read_fd);

  FD_SET(s, &read_fd);
  FD_SET(0, &read_fd);

  int n = select(s+1, &read_fd, NULL, NULL, NULL);

  if (FD_ISSET(s, &read_fd)) {
    while(1) {
      n = read (fd, data, buf);
      if( sendto(s, data, n, 0, (struct sockaddr *)&addr, sizeof(addr)) == -1 ) {
	printf("error(sendto)\n");
        return 0;
      }
      
      n = recvfrom(s, data, buf, 0, (struct sockaddr *)&addr, &addrlen);
      if ( write(fd, data, n) == -1 ) {
	printf("error(write)\n");
	return 0;
      }
    }
  }

  if (FD_ISSET(0, &read_fd)) {
    memset(data, 0, sizeof(buf));
    if(read(0, data, buf) == -1) {
      printf("error(read)\n");
      return 0;
    }

    addr.sin_addr.s_addr = inet_addr(data);
    
    while(1) {
      n = read (fd, data, buf);
      if( sendto(s, data, n, 0, (struct sockaddr *)&addr, sizeof(addr)) == -1 ) {
	printf("error(sendto)\n");
        return 0;
      }

      n = recvfrom(s, data, buf, 0, (struct sockaddr *)&addr, &addrlen);
      if ( write(fd, data, n) == -1 ) {
	printf("error(write)\n");
	return 0;
      }
    }
  }


  //struct sockaddr_in client_addr;
  //socklen_t len = sizeof(struct sockaddr_in);
  //int s = accept(ss, (struct sockaddr *)&client_addr, &len);
  /*if (s == -1) {
    printf("error(accept)\n");
    return 0;
  }
  else printf("accepted!\n");

  if ((send(s, "a", 1, 0)) == -1) {
    printf("error(send)\n");
    return 0;
  }

  for (i = 0; i < N; i++) {
    int n = read(s, data, N);
    if (n == -1) {
      printf("error(read)\n");
      return 0;
    }

    if (n == SZ) {
      if ((send(s, "a", 1, 0)) == -1) {
	printf("error(send)\n");
	return 0;
      }
    }
    }*/
  close (s);
  close (fd);

  return 0;
}
      
  
