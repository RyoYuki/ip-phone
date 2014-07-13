#include <stdio.h>
#define N 64

#include "fft.c"

int main(void){
    int i;
    static float x1[N], y1[N], x2[N], y2[N], x3[N], y3[N];
    for(i=0; i<N; i++){
        x1[i] = x2[i] = 6*cos(6*PI*i/N) + 4*sin(18*PI*i/N);
        y1[i] = y2[i] = 0;
    }
    fft(N, x2, y2);
    for(i=0; i<N; i++){
        x3[i] = x2[i]; y3[i] = y2[i];
    }
    ifft(N, x3, y3);
    printf("      元データ    フーリエ変換  逆変換\n");
    for(i=0; i<N; i++){
        printf("%4d | %6.3f %6.3f | %6.3f %6.3f | %6.3f %6.3f\n", i, x1[i], y1[i], x2[i], y2[i], x3[i], y3[i]);
    }
    return 0;
}
