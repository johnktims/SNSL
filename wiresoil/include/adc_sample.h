#ifndef ADC_SAMPLE_H
#define ADC_SAMPLE_H

#include "snsl.h"

// Config ADC Parameters
#define   ADC_LEN           10
#define   ADC_NSTEPS        1024
#define   ADC_10BIT_FLAG    0


/***********************************************************
 * Function Definitions
 **********************************************************/
float sampleRef(void);
float sampleSP(float);
float sampleTI(float);

void sampleProbes(FLOAT *);

#endif // ADC_SAMPLE_H
