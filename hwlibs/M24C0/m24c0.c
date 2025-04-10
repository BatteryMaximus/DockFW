#include <stdio.h>
#include <stdlib.h>
#include "hardware/i2c.h"

int writeDeviceSerial(uint16_t serialNumber, i2c_inst_t *i2c){
    uint8_t deviceAddress = 0xD0;
    uint8_t upperByteAddress[] = {0xE1};
    uint8_t lowerByteAddress[] = {0xF2};
    uint8_t upperByte = (serialNumber >> 8) & 0xFF;
    uint8_t lowerByte = serialNumber & 0xFF;
    uint8_t devSerialDataFirst[2] = {upperByteAddress[0], upperByte};
    uint8_t devSerialDataLast[2] = {lowerByteAddress[0], lowerByte};
    i2c_write_blocking(i2c, deviceAddress, devSerialDataFirst, 2, false);
    i2c_write_blocking(i2c, deviceAddress, devSerialDataLast, 2, false);
	
    
    //now to confirm the data
    uint8_t response_data[] = {0x00};
    i2c_write_blocking(i2c, deviceAddress, upperByteAddress, 1, false);
    i2c_read_blocking(i2c, deviceAddress, response_data, 1, false);
    printf("Response 1/2: %02X, ", response_data[0]);
    response_data[0] = 0x00;
    i2c_write_blocking(i2c, deviceAddress, lowerByteAddress, 1, false);
    i2c_read_blocking(i2c, deviceAddress, response_data, 1, false);
    printf("Response 2/2: %02X \n", response_data[0]);
    
    return 0;
    
}

uint16_t readDeviceSerial(i2c_inst_t *i2c){
    uint8_t deviceAddress = 0xD0;
    uint8_t upperByteAddress[] = {0xE1};
    uint8_t lowerByteAddress[] = {0xF2};

    uint8_t response_data[] = {0x00};
    uint16_t serialNumberRead = 0x0000;
    i2c_write_blocking(i2c, deviceAddress, upperByteAddress, 1, false);
    i2c_read_blocking(i2c, deviceAddress, response_data, 1, false);
    serialNumberRead = response_data[0] << 8;
    response_data[0] = 0x00;
    i2c_write_blocking(i2c, deviceAddress, lowerByteAddress, 1, false);
    i2c_read_blocking(i2c, deviceAddress, response_data, 1, false);
    serialNumberRead = serialNumberRead | response_data[0];
    
    return serialNumberRead;
}
