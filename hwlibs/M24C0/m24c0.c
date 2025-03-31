#include <stdio.h>
#include <stdlib.h>
#include "hardware/i2c.h"

int writeDeviceSerial(uint16_t serialNumber, i2c_inst_t *i2c){
    uint8_t deviceAddress = 0xD0;
    uint8_t serialAddress = 0xEF;
    uint8_t devSerialDataFirst[2] = {serialAddress, serialNumber >> 8};
    uint8_t devSerialDataLast[2] = {serialAddress + 1, serialNumber | 0xFF};
    i2c_write_blocking(i2c, deviceAddress, devSerialDataFirst, 2, false);
    i2c_write_blocking(i2c, deviceAddress, devSerialDataLast, 2, false);

    //now to confirm the data
    uint8_t response_data = 0x00;
    uint16_t serialNumberRead = 0x0000;
    i2c_write_blocking(i2c, deviceAddress, &serialAddress, 1, false);
    i2c_read_blocking(i2c, deviceAddress, &response_data, 1, false);
    printf("Device Address: %02X - ", deviceAddress);
    printf("Response 1/2: %02X, ", response_data);

    serialNumberRead = response_data << 8;
    response_data = 0;
    i2c_read_blocking(i2c, deviceAddress, &response_data, 1, false);
    printf("Response 2/2: %02X \n", response_data);
    serialNumberRead = serialNumberRead | response_data;
    
    return 0;
    
}

uint16_t readDeviceSerial(i2c_inst_t *i2c){
    uint8_t deviceAddress = 0xD0;
    uint8_t serialAddress = 0xEF;
    //now to confirm the data
    uint8_t response_data = 0x00;
    uint16_t serialNumberRead = 0x0000;
    i2c_write_blocking(i2c, deviceAddress, &serialAddress, 1, false);
    i2c_read_blocking(i2c, deviceAddress, &response_data, 1, false);
    serialNumberRead = response_data << 8;
    response_data = 0;
    i2c_read_blocking(i2c, deviceAddress, &response_data, 1, false);
    serialNumberRead = serialNumberRead | response_data;
    
    return serialNumberRead;
}