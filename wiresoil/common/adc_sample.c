#include "pic24_all.h"
#include "adc_sample.h"
#include <stdio.h>

// Samples 1.250V Reference, Calculates Battery Voltage
float sampleRef(void)
{
    uint16 avgADC, i;
    float f_ref;
    avgADC = 0;

    for(i = 0; i < 32; i++)
    {
        avgADC = avgADC + convertADC1();
    }

    f_ref = (1.25 * 1024.0) / (avgADC / 32.0);
    printf("Ref: %f\n", (double)f_ref);
    return f_ref;
}

// Samples Soil Probe
float sampleSP(float f_bv)
{
    uint16 avgADC, i;
    float f_sp;
    avgADC = 0;

    for(i = 0; i < 32; i++)
    {
        avgADC = avgADC + convertADC1();
    }

    f_sp = ((f_bv / 1024.0 * (avgADC / 32.0)) - 1.250) * 1000.0;
    printf("Soil Probe: %f\n", (double)f_sp);
    return f_sp;
}

// Samples Temperature Indicator
float sampleTI(float f_bv)
{
    uint16 avgADC, i;
    float f_ti;
    avgADC = 0;

    for(i = 0; i < 32; i++)
    {
        avgADC = avgADC + convertADC1();
    }

    f_ti = ((((f_bv / 1024.0 * (avgADC / 32.0)) * 1000.0) - 424.0) / 6.25);
    printf("Temp: %f\n", (double)f_ti);
    return f_ti;
}

void sampleProbes(FLOAT *af_probeData)
{
    float f_bv;
    // Calculate Battery Voltage from 1.250V Reference
    configADC1_ManualCH0(ADC_CH0_POS_SAMPLEA_AN0, 31, ADC_10BIT_FLAG);
    af_probeData[0].f = f_bv = sampleRef();
    // Sample Soil Probe 1
    configADC1_ManualCH0(ADC_CH0_POS_SAMPLEA_AN9, 31, ADC_10BIT_FLAG);
    af_probeData[1].f = sampleSP(f_bv);
    // Sample Soil Probe 2
    configADC1_ManualCH0(ADC_CH0_POS_SAMPLEA_AN10, 31, ADC_10BIT_FLAG);
    af_probeData[2].f = sampleSP(f_bv);
    // Sample Soil Probe 3
    configADC1_ManualCH0(ADC_CH0_POS_SAMPLEA_AN11, 31, ADC_10BIT_FLAG);
    af_probeData[3].f = sampleSP(f_bv);
    // Sample Soil Probe 4
    configADC1_ManualCH0(ADC_CH0_POS_SAMPLEA_AN12, 31, ADC_10BIT_FLAG);
    af_probeData[4].f = sampleSP(f_bv);
    // Sample Temp Indicator 1
    configADC1_ManualCH0(ADC_CH0_POS_SAMPLEA_AN1, 31, ADC_10BIT_FLAG);
    af_probeData[5].f = sampleTI(f_bv);
    // Sample Temp Indicator 2
    configADC1_ManualCH0(ADC_CH0_POS_SAMPLEA_AN2, 31, ADC_10BIT_FLAG);
    af_probeData[6].f = sampleTI(f_bv);
    // Sample Temp Indicator 3
    configADC1_ManualCH0(ADC_CH0_POS_SAMPLEA_AN3, 31, ADC_10BIT_FLAG);
    af_probeData[7].f = sampleTI(f_bv);
    // Sample Temp Indicator 4
    configADC1_ManualCH0(ADC_CH0_POS_SAMPLEA_AN4, 31, ADC_10BIT_FLAG);
    af_probeData[8].f = sampleTI(f_bv);
    // Sample Temp Indicator 5
    configADC1_ManualCH0(ADC_CH0_POS_SAMPLEA_AN5, 31, ADC_10BIT_FLAG);
    af_probeData[9].f = sampleTI(f_bv);
}

/*void NormalMode(void) {
  float f_bv, f_sp1, f_sp2, f_sp3, f_sp4;
  float f_ti1, f_ti2, f_ti3, f_ti4, f_ti5;

  CONFIG_AN0_AS_ANALOG();
  CONFIG_AN1_AS_ANALOG();
  CONFIG_AN2_AS_ANALOG();
  CONFIG_AN3_AS_ANALOG();
  CONFIG_AN4_AS_ANALOG();
  CONFIG_AN5_AS_ANALOG();
  CONFIG_AN9_AS_ANALOG();
  CONFIG_AN10_AS_ANALOG();
  CONFIG_AN11_AS_ANALOG();
  CONFIG_AN12_AS_ANALOG();

  // Delay 1 Second
  DELAY_MS(1000);

  // Calculate Battery Voltage from 1.250V Reference
  configADC1_ManualCH0(ADC_CH0_POS_SAMPLEA_AN0, 31, ADC_10BIT_FLAG);
  f_bv = sampleRef();

  // Sample Soil Probe 1
  configADC1_ManualCH0(ADC_CH0_POS_SAMPLEA_AN9, 31, ADC_10BIT_FLAG);
  f_sp1 = sampleSP(f_bv);

  // Sample Soil Probe 2
  configADC1_ManualCH0( ADC_CH0_POS_SAMPLEA_AN10, 31, ADC_10BIT_FLAG );
  f_sp2 = sampleSP(f_bv);

  // Sample Soil Probe 3
  configADC1_ManualCH0( ADC_CH0_POS_SAMPLEA_AN11, 31, ADC_10BIT_FLAG );
  f_sp3 = sampleSP(f_bv);

  // Sample Soil Probe 4
  configADC1_ManualCH0( ADC_CH0_POS_SAMPLEA_AN12, 31, ADC_10BIT_FLAG );
  f_sp4 = sampleSP(f_bv);

  // Sample Temp Indicator 1
  configADC1_ManualCH0( ADC_CH0_POS_SAMPLEA_AN1, 31, ADC_10BIT_FLAG );
  f_ti1 = sampleTI(f_bv);

  // Sample Temp Indicator 2
  configADC1_ManualCH0( ADC_CH0_POS_SAMPLEA_AN2, 31, ADC_10BIT_FLAG );
  f_ti2 = sampleTI(f_bv);

  // Sample Temp Indicator 3
  configADC1_ManualCH0( ADC_CH0_POS_SAMPLEA_AN3, 31, ADC_10BIT_FLAG );
  f_ti3 = sampleTI(f_bv);

  // Sample Temp Indicator 4
  configADC1_ManualCH0( ADC_CH0_POS_SAMPLEA_AN4, 31, ADC_10BIT_FLAG );
  f_ti4 = sampleTI(f_bv);

  // Sample Temp Indicator 5
  configADC1_ManualCH0( ADC_CH0_POS_SAMPLEA_AN5, 31, ADC_10BIT_FLAG );
  f_ti5 = sampleTI(f_bv);

  //Write Data to VDIP1
  synchE();
  createData();
  outStringRTCC();
  write125();
  writeBV(f_bv);
  writeSP(f_sp1);
  writeSP(f_sp2);
  writeSP(f_sp3);
  writeSP(f_sp4);
  writeTemp(f_ti1);
  writeTemp(f_ti2);
  writeTemp(f_ti3);
  writeTemp(f_ti4);
  writeTemp(f_ti5);
  outStringNodeName();
  writeCR();
  closeData();

  //Write Debug File to VDIP1
  writeDebug();
}*/
