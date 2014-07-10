#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define PI 3.14159265358979323846

static void make_sintbl(int n, float sintbl[]){
    int i, n2, n4, n8;
    double c, s, dc, ds, t;
    n2 = n/2; n4 = n/4; n8 = n/8;
    t = sin(PI/n);
    dc = 2*t*t; ds = sqrt(dc*(2-dc));
    t = 2*dc; c=sintbl[n4] = 1; s = sintbl[0] = 0;
    for(i=1; i<n8; i++){
        c -= dc; dc += t*c;
        s += ds; ds -= t*s;
        sintbl[i] = s; sintbl[n4 - i] = c;
    }
    if(n8!=0) sintbl[n8] = sqrt(0.5);
    for(i=0; i<n4; i++) sintbl[n2 - i] = sintbl[i];
    for(i=0; i<n2+n4; i++) sintbl[i + n2] = -sintbl[i];
}

static void make_bitrev(int n, int bitrev[]){
    int i, j, k, n2;
    n2 = n/2; i = j = 0;
    for(;;){
        bitrev[i] = j;
        if(++i >= n)break;
        k= n2;
        while(k<=j){j-=k; k/=2;}
        j += k;
    }
}

int fft(int n, float x[], float y[]){
    static int last_n = 0;
    static int *bitrev = NULL;
    static float *sintbl = NULL;
    int i, j, k, ik, h, d, k2, n4, inverse;
    float t, s, c, dx, dy;

    if(n<0){
        n = -n; inverse = 1;
    }else{
        inverse = 0;
    }
    n4 = n/4;
    if(n!= last_n || n==0){
        last_n = n;
        if(sintbl != NULL) free(sintbl);
        if(bitrev != NULL) free(bitrev);
        if(n==0) return 0;
        sintbl = malloc((n+n4)*sizeof(float));
        bitrev = malloc(n*sizeof(int));
        if(sintbl == NULL || bitrev == NULL){
            fprintf(stderr, "Insufficient memory size\n");
            return 1;
        }
        make_sintbl(n, sintbl);
        make_bitrev(n, bitrev);
    }
    for(i=0; i<n; i++){
        j = bitrev[i];
        if(i<j){
            t = x[i]; x[i] = x[j]; y[j] = t;
            t = y[i]; y[i] = y[j]; y[j] = t;
        }
    }
    for(k=1; k<n; k=k2){
        h=0; k2=k+k; d=n/k2;
        for(j=0; j<k; j++){
            c=sintbl[h+n4];
            if(inverse) s = -sintbl[h];
            else s = sintbl[h];
            for(i=j; i<n; i+= k2){
                ik=i+k;
                dx=s*y[ik] + c*x[ik];
                dy=c*y[ik] - s*x[ik];
                x[ik] = x[i] - dx; x[i] += dx;
                y[ik] = y[i] - dy; y[i] += dy;
            }
            h+=d;
        }
    }
    if(!inverse){
        for(i=0; i<n; i++){
            x[i] /= n; y[i] /= n;
        }
    }
    return 0;
}

int ifft(int n, float x[], float y[]){
    return fft(-n, x, y);
}
