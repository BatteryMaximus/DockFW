#include <stdio.h>
#include <stdlib.h>
#include "hardware/i2c.h"

int setMCP4725Voltage(i2c_inst_t *i2c, uint16_t voltage, bool addressPin){
	uint8_t mcp4725BaseAddress = 0x60;
    if(addressPin == 1){
        mcp4725BaseAddress += 1;
    }
	uint8_t programData[3] = {0x60, 0x00, 0x00};
	uint8_t rxdata[5] = {0, 0, 0, 0, 0};
	if(voltage < 0 || voltage > 4096){
		return -1;
	}
	uint16_t shiftedByte = voltage << 4;
	programData[1] = (shiftedByte >> 8) & 0xFF;
	programData[2] = shiftedByte & 0xFF;
	printf("Starting Write");
	i2c_write_blocking(i2c, mcp4725BaseAddress, programData, 3, false);
	i2c_read_blocking(i2c, mcp4725BaseAddress, rxdata, 5, false);
	if (rxdata[1] != programData[1] || rxdata[2] != programData[2]){
        return -1;
    }
    return 0;
}