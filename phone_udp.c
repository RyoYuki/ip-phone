#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>

#include <time.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/mount.h>
#include <linux/fs.h>

#define PORT (50000)
#define UDP_PORT (50001)

#define BUF_SIZE (1024)

#define NOISE_THRESHOLD (10)

int main(int argc, char** argv){
    int listen_fd, recv_fd, send_fd, audio_fd, audio_socket_fd, max_fd;
    int isCalling = 0;
    int isMyCalling = 0;
    fd_set fds, readfds;
    char buf[BUF_SIZE];
    int i, j, n;

    send_fd = socket(PF_INET, SOCK_STREAM, 0);
    listen_fd = socket(PF_INET, SOCK_STREAM, 0);

    audio_socket_fd = socket(PF_INET, SOCK_DGRAM, 0);

    socklen_t len = sizeof(struct sockaddr_in);
    socklen_t len_udp;

    struct sockaddr_in my_addr, other_recv_addr, other_send_addr, audio_addr;
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(PORT);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    if(bind(listen_fd, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1){
        fprintf(stderr, "Failed to bind\n");
        exit(1);
    }
    listen(listen_fd, 10);

    other_recv_addr.sin_family = AF_INET;
    other_recv_addr.sin_port = htons(PORT);

    other_send_addr.sin_family = AF_INET;
    other_send_addr.sin_port = htons(PORT);

    audio_addr.sin_family = AF_INET;
    audio_addr.sin_port = htons(UDP_PORT);

    bind(audio_socket_fd, (struct sockaddr*)&audio_addr, sizeof(audio_addr));

    if((audio_fd = open("/dev/dsp", O_RDWR, 0644)) == -1){
        fprintf(stderr, "Failed to open /dev/dsp\n");
        exit(1);
    }
   while(1){
        ioctl(audio_fd, BLKFLSBUF, 0);

        FD_ZERO(&readfds);
        FD_SET(0, &readfds);
        FD_SET(listen_fd, &readfds);
        if(isCalling){
            FD_SET(recv_fd, &readfds);
            FD_SET(audio_socket_fd, &readfds);
        }
        FD_SET(audio_fd, &readfds);

        max_fd = listen_fd > recv_fd ? listen_fd : recv_fd;
        max_fd = max_fd > audio_fd ? max_fd : audio_fd;

        memcpy(&fds, &readfds, sizeof(fd_set));
        select(max_fd+1, &fds, NULL, NULL, NULL);

        if(FD_ISSET(0, &fds)){
            n = read(0, buf, BUF_SIZE);
            if( n == -1 ){
                fprintf(stderr, "read stdin\n");
                exit(1);
            }
            if( n > 0 ){
                switch(buf[0]){
                    case 'q':
                        goto QUIT;
                        break;
                    default:
                        if(inet_aton(buf, &other_recv_addr.sin_addr) == 0){
                            fprintf(stderr, "Inavlid IP address\n");
                        }else{
                            if(connect(send_fd, (struct sockaddr*)&other_recv_addr, sizeof(other_recv_addr)) == -1){
                                fprintf(stderr, "Failed to connect to %s:%d\n", buf, PORT);
                                break;
                            }
                            if(inet_aton(buf, &audio_addr.sin_addr) == 0){
                                fprintf(stderr, "Invalid IP address\n");
                                break;
                            }
                            fprintf(stdout, "Calling to %s\n", buf);
                            isMyCalling = 1;
                        }
                }
            }
        }

        if(FD_ISSET(listen_fd, &fds)){
            if(isCalling){
                fprintf(stdout, "catch call\n");
            }else{
                recv_fd = accept(listen_fd, (struct sockaddr *)&other_send_addr, &len);
                if(!isMyCalling){
                    memcpy(&other_recv_addr.sin_addr, &other_send_addr.sin_addr, sizeof(struct in_addr));
                    memcpy(&audio_addr.sin_addr, &other_send_addr.sin_addr, sizeof(struct in_addr));
                    audio_addr.sin_port = htons(UDP_PORT);
                    len_udp = sizeof(audio_addr);
                    if(connect(send_fd, (struct sockaddr*)&other_recv_addr, sizeof(other_recv_addr)) == -1){
                        fprintf(stderr, "Failed to connect back to %s:%d\n", other_recv_addr.sin_addr.s_addr, PORT);
                    }
                }
                isCalling = 1;
            }
        }

        if(isCalling && FD_ISSET(recv_fd, &fds)){
            n = read(recv_fd, buf, BUF_SIZE);
            write(audio_fd, buf, n);
        }

        if(isCalling && FD_ISSET(audio_socket_fd, &fds)){
            n = recvfrom(audio_socket_fd, buf, BUF_SIZE, 0, (struct sockaddr*)&audio_addr, &len_udp);
            write(audio_fd, buf, n);
        }

        if(FD_ISSET(audio_fd, &fds)){
            if(isCalling){
                n = read(audio_fd, buf, BUF_SIZE);
                int max_v = 0;
                for( i = 0; i < n; i++){
                    if(buf[i]<NOISE_THRESHOLD) buf[i] = 0;
                    if(buf[i] > max_v) max_v = buf[i];
                }
                if(max_v != 0){
                    sendto(audio_socket_fd, buf, n, 0, (struct sockaddr*)&audio_addr, len_udp);
                }
            }
        }
    }
QUIT:
    close(recv_fd);
    close(send_fd);
    close(audio_fd);

    fprintf(stdout, "Good Bye\n");
    return 0;
}
