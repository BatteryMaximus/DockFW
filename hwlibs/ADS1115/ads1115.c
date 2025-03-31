#include <stdio.h>
#include <stdlib.h>
#include "hardware/i2c.h"

int getADS115AllValues(int * values, i2c_inst_t *i2c ){
	uint8_t i2cAddr = 0x48;
	uint8_t conversionRegister = 0x00;
    uint8_t configRegister = 0x01;

	
	for (int i = 4; i < 8; i++){
		uint8_t returnHolder[2] = {0, 0};
		uint8_t configInputFrame = (i << 4) | 0x83;
        //i << 4 says that we want to set the conversion target to single ended reading of 0->4 of the AIN
        // The | 3 set the bottom 4 bits of this config byte, The first 3 bits signify the gain amplifier,
        // The last bit sets continuous or single shot read mode.
		uint8_t configFrames[3] = {configRegister, configInputFrame, 0xE3};
		int x = i - 4;
        // send our compiled config to the device.
        // This config says we want to read i input, in single shot mode
		i2c_write_blocking(i2c, i2cAddr, configFrames, 3, false);
        uint8_t configRegisterStatus[2] = {0, 0};
        i2c_read_blocking(i2c, i2cAddr, configRegisterStatus, 2, false);

        //Now we send the address of the conversion register only, to tell the device we want a conversion.
		i2c_write_blocking(i2c, i2cAddr, 0x00, 1, false);
		
        //Wait just a bit to give the device time to calculate the value
		sleep_ms(5);

        //Now read 2 bytes from the device, representing the 16 bit value of the current AIN.
		i2c_read_blocking(i2c, i2cAddr, returnHolder, 2, false);
		int totalValue = (returnHolder[0] << 8) | returnHolder[1];
		totalValue = (totalValue > 0x7fff) ?  0 : totalValue;
		values[x] = totalValue;		
	}
	return 0;
}


int ADS1115focusSingleValue(int ain, int numConversions, i2c_inst_t *i2c ){
    if(ain > 4 || ain < 0){
        return -1;
    }
    ain = ain + 4;
	uint8_t i2cAddr = 0x48;
	uint8_t conversionRegister = 0x00;
    uint8_t configRegister = 0x01;
    
	
    uint8_t returnHolder[2] = {0, 0};
    uint8_t configInputFrame = (ain << 4) | 4;

    uint8_t configFrames[3] = {configRegister, configInputFrame, 0xE3};
    i2c_write_blocking(i2c, i2cAddr, configFrames, 3, false);
    //Now we send the address of the conversion register only, to tell the device we want a conversion.
    i2c_write_blocking(i2c, i2cAddr, &conversionRegister, 1, false);
    
    //Wait just a bit to give the device time to calculate the value
    sleep_ms(50);

    for(int i = 0; i < numConversions; i++){
        i2c_read_blocking(i2c, i2cAddr, returnHolder, 2, false);
        int totalValue = (returnHolder[0] << 8) | returnHolder[1];
        totalValue = (totalValue > 0x7fff) ?  0 : totalValue;
        sleep_ms(100);
        printf("Read Value: %d \n", totalValue);
    }

	return 0;
}