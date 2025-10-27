#ifndef PAV_ANALYSIS_H
#define PAV_ANALYSIS_H

void hamming_window(float *w, unsigned int N);
float compute_power(const float *x, unsigned int N);
float compute_power_hamming(const float *x, const float *w, unsigned int N);
float compute_am(const float *x, unsigned int N);
float compute_zcr(const float *x, unsigned int N, float fm);

#endif	/* PAV_ANALYSIS_H	*/
