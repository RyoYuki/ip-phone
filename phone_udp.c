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

#define BUF_SIZE (2048)

#define NOISE_THRESHOLD (10)

int main(int argc, char** argv){
    int listen_fd, recv_fd=0, send_fd, audio_fd, audio_socket_fd, max_fd;
    int isCalling = 0;
    int isMyCalling = 0;
    fd_set fds, readfds;
    char buf[BUF_SIZE];
    float x[BUF_SIZE], y[BUF_SIZE];
    float xd[BUF_SIZE], yd[BUF_SIZE];
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

    char isLPFon = 0;
    char isVoiceChangerOn = 0;
    int LPF_THRESHOLD = 200;
    int VC_WIDTH = 200;

    FILE *freq_dat_fp;
    int count=0;

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
        max_fd = max_fd > audio_socket_fd ? max_fd : audio_socket_fd;

        memcpy(&fds, &readfds, sizeof(fd_set));
        select(max_fd+1, &fds, NULL, NULL, NULL);

        if(FD_ISSET(0, &fds)){
            n = read(0, buf, BUF_SIZE);
            if( n == -1 ){
                fprintf(stderr, "read stdin\n");
                exit(1);
            }
            if( n > 0 ){
                for(i=0; i<n; i++){
                    if(buf[i] == '\n'){
                        buf[i] = '\0';
                        break;
                    }
                }
                switch(buf[0]){
                    case 'q':
                        goto QUIT;
                        break;
                    default:
                        if(strncmp(buf, "set lpf th ", 11) == 0){
                            LPF_THRESHOLD = atoi(buf+11);
                            fprintf(stdout, "LPF THRESHOLD: %d\n", LPF_THRESHOLD);
                            break;
                        }
                        if(strncmp(buf, "set voicechange width ", 22) == 0){
                            VC_WIDTH = atoi(buf+22);
                            fprintf(stdout, "VC SHIFT N: %d\n", VC_WIDTH);
                            break;
                        }
                        if(strcmp(buf, "set lpf on") == 0){
                            isLPFon = 1;
                            fprintf(stdout, "LPF on\n");
                        }else if(strcmp(buf, "set lpf off") == 0){
                            isLPFon = 0;
                            fprintf(stdout, "LPF off\n");
                        }else if(strcmp(buf, "set voicechange on") == 0){
                            isVoiceChangerOn = 1;
                            fprintf(stdout, "Voice Changer on\n");
                        }else if(strcmp(buf, "set voicechange off") == 0){
                            isVoiceChangerOn = 0;
                            fprintf(stdout, "Voice Changer off\n");
                        }else{
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
            for(i=0; i<n; i++){
                x[i] = (float)buf[i] - 127;
                y[i] = 0;
            }
            char _f = 0;
            if(fft(n, x, y)){_f=1;}
            for(i=0; i<n; i++){
                xd[i] = x[i];
            }
            if(isLPFon){
                for(i=0; i<LPF_THRESHOLD; i++){
                    x[i] = 0;
                    y[i] = 0;
                }
                for(i=n-1; i>n-LPF_THRESHOLD; i--){
                    x[i] = 0;
                    y[i] = 0;
                }
            }
            if(isVoiceChangerOn){
                for(i=n-1; i>=n/2; i--){
                    x[i] = x[i-VC_WIDTH];
                    y[i] = y[i-VC_WIDTH];
                }
                for(i=0; i<n/2; i++){
                    x[i] = x[i+VC_WIDTH];
                    y[i] = y[i+VC_WIDTH];
                }
                for(i=n/2-VC_WIDTH; i<n/2+VC_WIDTH; i++){
                    x[i] = 0;
                    y[i] = 0;
                }
            }
            for(i=0; i<n; i++){
                if(x[i]*x[i] + y[i]*y[i] < 10){
                    x[i] = y[i] = 0;
                }
            }
            if(count%5==0){
                freq_dat_fp = fopen("freq.dat", "w");
                for(i=0; i<n; i++){
                    fprintf(freq_dat_fp, "%d %6.6f\n", i, sqrt(x[i]*x[i] + y[i]*y[i]));
                }
                fclose(freq_dat_fp);
            }
            count++;
            if(ifft(n, x, y)){_f=1;}
            for(i=0; i<n; i+=100){
                /* fprintf(stdout, "%d: %d %6.1f %6.1f\n", i, (int)buf[i], xd[i], x[i]); */
                buf[i] = (char)(x[i] + 127);
            }
            if(!_f){
                write(audio_fd, buf, n);
            }
        }

        if(FD_ISSET(audio_fd, &fds)){
            if(isCalling){
                n = read(audio_fd, buf, BUF_SIZE);
                for( i = 0; i < n; i++){
                    if(fabs(buf[i]-127) < NOISE_THRESHOLD) buf[i] = 127;
                }
                sendto(audio_socket_fd, buf, n, 0, (struct sockaddr*)&audio_addr, len_udp);
            }
        }
    }
QUIT:
    close(recv_fd);
    close(send_fd);
    close(audio_fd);
    close(audio_socket_fd);
    close(listen_fd);

    fprintf(stdout, "Good Bye\n");
    return 0;
}
