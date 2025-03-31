#ifndef M24C0_H
#define M24C0_H
int writeDeviceSerial(uint16_t serialNumber, i2c_inst_t *i2c);
uint16_t readDeviceSerial(i2c_inst_t *i2c);


#endif