#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hwlibs/MCP4725/mcp4725.h"
#include "hwlibs/ADS1115/ads1115.h"
#include "hwlibs/M24C0/m24c0.h"

#define SDA_PIN 2
#define SCL_PIN 3
#define EEPROM_WC 5
#define CHARGE 6
#define EN_CHARGE 14

int initSystems(){
	printf("Beginning initialization....\n");
	i2c_init(i2c1, 100*1000);
	gpio_set_function(SDA_PIN, GPIO_FUNC_I2C); //SDA
	gpio_set_function(SCL_PIN, GPIO_FUNC_I2C); //SCL
	gpio_pull_up(SDA_PIN);
	gpio_pull_up(SCL_PIN);
	sleep_ms(100);
	
	setMCP4725Voltage(i2c1, 400, false);
	// Initialize GPIO Pins
	gpio_init(CHARGE);
	gpio_init(EEPROM_WC);
	gpio_init(EN_CHARGE);


	//Set GPIO Pin Direction
	gpio_set_dir(CHARGE, GPIO_OUT);
	gpio_set_dir(EEPROM_WC, GPIO_OUT);
	gpio_set_dir(EN_CHARGE, GPIO_OUT);
	printf("Finished initialization....\n");
	return 0;
}

float calculateBatVoltage(int analogValue, float analogMaxVoltage, int r1, int r2){
	float analogVoltage = ((float)analogValue / 32768.0) * analogMaxVoltage;
	float totalResistance = r1 + r2;
	float resistancePercentage = (float)r1 / totalResistance;
	float batVoltage = analogVoltage / resistancePercentage;
	return batVoltage;
}

float calculateTemperature(float analogValue, float ntcResistance, float betaNumber){
	float ntcDropPercentage = analogValue / (float)32768;
	float opposingPercentage = 1.0  - ntcDropPercentage;
	float systemResistance = ntcResistance / opposingPercentage;
	float ntcRealResistance = systemResistance - ntcResistance;
	float ntcResistancePercentage = ntcRealResistance / ntcResistance;
	float ntcLog = logf(ntcResistancePercentage);
	float reciprocalTemperature = 1.0 / 298.15;
	float betaCorection = ntcLog / betaNumber;
	float kelvinTemperature = 1.0 / (reciprocalTemperature - betaCorection);
	float celciusTemperature = kelvinTemperature - 273.15;
	return celciusTemperature;
}

float calculateShuntCurrent(float shuntResistance, float analogValue, float analogMaxVoltage){
	analogValue = analogValue - 500;
	analogValue = analogValue / 100;
	float percentage = analogValue / (float)32768;
	float voltage = percentage * analogMaxVoltage;
	float shuntCurrent = voltage / shuntResistance;
	return shuntCurrent;
}

int checkBatHealth(){
	uint16_t serialNumber = 0x0000;
	serialNumber = readDeviceSerial(i2c1);
	float adcMaxVoltage = 4.096;
	float shuntResistance = 0.003;
	int values[4] = {0, 0, 0, 0};
	getADS115AllValues(values, i2c1);
	float batVoltage = calculateBatVoltage(values[0], adcMaxVoltage, 10, 100);
	float temperature = calculateTemperature((float)values[1], (float)50000, (float)3800);
	float chargeCurrent = calculateShuntCurrent(shuntResistance, (float)values[2], adcMaxVoltage);
	if (temperature > 50.0){
		gpio_put(CHARGE, 0);
		gpio_put(EN_CHARGE, 0);
		printf("Disabling charging, temperature too high \n");
	}
	printf("Serial: %04X: Bat Voltage: %.2fv, Temperature: %.2fC, Charge Current: %.2fA\n", serialNumber, batVoltage, temperature, chargeCurrent);
}

int clearCharString(char *arr, int length){
	for (int i = 0; i < length; i++){
		arr[i] = 0;
	}
	return 0;
}

int charToInt(char c){
	int nubbin = 0;
	if(c >= 48 & c <= 57){
		nubbin = c - 48;	
	}else if(c >= 65 & c <= 70){
		nubbin = c - 55;
	}
	return nubbin;
}


uint16_t processCommand(char *command, int commandLength){
	uint16_t commandNumber = charToInt(command[1]) << 12 | charToInt(command[2]) << 8 | charToInt(command[3]) << 4 | charToInt(command[4]);
	uint16_t subCommand = charToInt(command[6]) << 12 | charToInt(command[7]) << 8 | charToInt(command[8]) << 4 | charToInt(command[9]);
	float adcMaxVoltage = 4.096;
	float shuntResistance = 0.003;
	int values[4] = {0, 0, 0, 0};
	getADS115AllValues(values, i2c1);
	if(command[0] != 'G'){
		printf("Unrecognized Machine Command \n");
		return 0;
	}else{
		switch (commandNumber) {
			case 1: // Start Charging 
				printf("Starting Charging... \n");
				gpio_put(CHARGE, 1);	
				gpio_put(EN_CHARGE, 1);
				break;
			case 2: // Stop Charging
				printf("Stopping Charging ... \n");
				gpio_put(CHARGE, 0);
				gpio_put(EN_CHARGE, 0);
				break;
			case 21: // get battery voltage
				float batVoltage = calculateBatVoltage(values[0], adcMaxVoltage, 10, 100);
				printf("%.2f\n", batVoltage);
				break;
			case 22: // get charge current
				float chargeCurrent = calculateShuntCurrent(shuntResistance, (float)values[2], adcMaxVoltage);
				printf("%.2f\n", chargeCurrent);
				break;
			case 23: // get battery temperature
				float temperature = calculateTemperature((float)values[1], (float)50000, (float)3800);
				printf("%.2f\n", temperature);
				break;
			case 33: // set max current limit
				setMCP4725Voltage(i2c1, subCommand, false);
				printf("Max Current Has been set \n");
				break;
			case 10: // Retreive Serial Number From Devicej
				uint16_t serialNumber = 0x0000;
				serialNumber = readDeviceSerial(i2c1);
				printf("%04X\n", serialNumber);
				break;
			case 11: // Set device Serial Number
				printf("Will assign %04X to this device. \n", subCommand);
				writeDeviceSerial(subCommand, i2c1);
				break;
			default:
				printf("No command recognized \n");

		}
	}
}

int processCommandChar(char *command, int commandLength, char newChar){
	if(newChar == ';'){
		//Process Command
		processCommand(command, commandLength);
		clearCharString(command, commandLength);
	}else{
		for(int i = 0; i < commandLength; i++){
			if(command[i] == 0){
				command[i] = newChar;	
				break;
			}
		}
	}
}


int main(){
	uint16_t serialNumber = 0x0000;
	int commandLength = 200;
	char command[commandLength] = {};
	int maxCycles = 100000000;	
	int currentCycle = 0;
	stdio_init_all();
	initSystems();
	sleep_ms(1000);
	gpio_put(CHARGE, 0);
	gpio_put(EEPROM_WC, 0);
	gpio_put(EN_CHARGE, 0);
	clearCharString(command, commandLength); 
	serialNumber =  readDeviceSerial(i2c1);

	
	while(1){
		if(uart_is_readable(uart0)){
			char c = uart_getc(uart0);
			processCommandChar(command, commandLength, c);
			}		
		if(currentCycle >= maxCycles){
			checkBatHealth();
			currentCycle = 0;
		}
		currentCycle += 1;
			
	}



}

