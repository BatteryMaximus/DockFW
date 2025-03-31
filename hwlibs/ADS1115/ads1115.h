#ifndef ADS1115_H
#define ADS1115_H

int getADS115AllValues(int * values, i2c_inst_t *i2c );
int ADS1115focusSingleValue(int ain, int numConversions, i2c_inst_t *i2c );

#endif