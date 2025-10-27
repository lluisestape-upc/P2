#include <math.h>
#include "pav_analysis.h"
#define M_PI 3.14159265358979323846

void hamming_window(float *w, unsigned int N) {
    for (unsigned int n = 0; n < N; ++n) {
        w[n] = 0.54f - 0.46f * cosf(2.0f * M_PI * n / (N - 1));
    }
}

float compute_power(const float *x, unsigned int N) {
    float sum = 0;

    for (int i = 0; i < N; i++) sum = sum + x[i]*x[i];
    
    float b = sum/N;
    float P = 10*log10(b);
    
    return P;
}

float compute_power_hamming(const float *x,  const float *w, unsigned int N) {
    float sum = 0;
    float window_energy = 0;

    for (int i = 0; i < N; i++) sum = sum + x[i]*x[i];
    for (int i = 0; i < N; i++) window_energy = window_energy + w[i]*w[i];
    
    float b = sum/window_energy;
    float P = 10*log10(b);
    
    return P;
}

float compute_am(const float *x, unsigned int N) {
    float sum = 0;

    for (int i = 0; i < N; i++) sum = sum + fabs(x[i]);

    float am = sum/N;
    
    return am;
}

float compute_zcr(const float *x, unsigned int N, float fm) {
    float sum = 0;

    for (int i = 1; i < N; i++){
        int boolean_sum = (x[i]>0) + (x[i-1]>0);
        
        if (boolean_sum == 1) sum += 1;    
    }

    float zcr = (fm/2)*(sum/(N-1));

    return zcr;
}
