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
#define EN_CHARGE_PWR 14
#define LED0 16
#define LED1 17
#define LED2 18
#define LED3 19





int initSystems(){
	printf("Beginning initialization....\n");
	i2c_init(i2c1, 100*1000);
	gpio_set_function(SDA_PIN, GPIO_FUNC_I2C); //SDA
	gpio_set_function(SCL_PIN, GPIO_FUNC_I2C); //SCL
	sleep_ms(100);
	setMCP4725Voltage(i2c1, 1000, false);
	
	int outputPins[] = {EEPROM_WC, CHARGE, EN_CHARGE_PWR, LED0, LED1, LED2, LED3};
	int outputPinCount = sizeof(outputPins) / sizeof(outputPins[0]);
	for (int i = 0; i < outputPinCount; i++){
		gpio_init(outputPins[i]);
		gpio_set_dir(outputPins[i], GPIO_OUT);
	}

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
	analogValue = analogValue / 100;
	float percentage = analogValue / (float)32768;
	float voltage = percentage * analogMaxVoltage;
	float shuntCurrent = voltage / shuntResistance;
	return shuntCurrent;
}

int checkBatHealth(bool printStats){
	uint16_t serialNumber = 0x0000;
	serialNumber = readDeviceSerial(i2c1);
	float adcMaxVoltage = 4.096;
	float shuntResistance = 0.003;
	int values[4] = {0, 0, 0, 0};
	getADS115AllValues(values, i2c1);
	float batVoltage = calculateBatVoltage(values[0], adcMaxVoltage, 10, 100);
	float temperature = calculateTemperature((float)values[1], (float)50000, (float)3800);
	float chargeCurrent = calculateShuntCurrent(shuntResistance, (float)values[2], adcMaxVoltage);
	if (temperature > 50.0 && serialNumber > 0){
		gpio_put(CHARGE, 0);
		gpio_put(EN_CHARGE_PWR, 0);
		printf("Disabling charging, temperature too high \n");
	}
	if(printStats){
		printf("Serial: %04X: Bat Voltage: %.2fv, Temperature: %.2fC, Charge Current: %.2fA\n", serialNumber, batVoltage, temperature, chargeCurrent);
	}
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
			case 1: // Start Charging Power Supply
				printf("Starting Charge Power Supply... \n");
				gpio_put(LED0, 1);
				gpio_put(EN_CHARGE_PWR, 1);
				break;
			case 2: // Stop Charging Power Supply
				printf("Stopping Charge Power Supply... \n");
				gpio_put(LED0, 0);
				gpio_put(EN_CHARGE_PWR, 0);
				break;
			case 3: // Enable Charging Mosfet
				printf("Enabling Charge FET... \n");
				gpio_put(LED1, 1);
				gpio_put(CHARGE, 1);
				break;
			case 4:
				printf("Disabling Charge FET... \n");
				gpio_put(LED1, 0);
				gpio_put(CHARGE, 0);
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
				printf("Setting Max Current To %d/4096 \n", subCommand);
				setMCP4725Voltage(i2c1, subCommand, false);
				printf("Max Current Has been set \n");
				break;
			case 10: // Retreive Serial Number From Devicej
				uint16_t serialNumber = 0x0000;
				serialNumber = readDeviceSerial(i2c1);
				printf("%04X\n", serialNumber);
				break;
			case 11: // Set device Serial Number
				gpio_put(EEPROM_WC, 0);
				printf("Will assign %04X to this device. \n", subCommand);
				writeDeviceSerial(subCommand, i2c1);
				gpio_put(EEPROM_WC, 1);
				break;
			case 12: // Print out stats
				checkBatHealth(true);	
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

int ledStartUp(){
	int leds[] = {LED0, LED1, LED2, LED3};
	for(int i=0; i < 4; i++){
		gpio_put(leds[i], 1);
		sleep_ms(200);
		gpio_put(leds[i], 0);
	}
}


int main(){
	uint16_t serialNumber = 0x0000;
	int commandLength = 200;
	char command[200] = {};
	int maxCycles = 10000000;	
	int currentCycle = 0;
	stdio_init_all();
	initSystems();
	sleep_ms(1000);
	gpio_put(CHARGE, 0);
	gpio_put(EEPROM_WC, 1);
	gpio_put(EN_CHARGE_PWR, 0);
	gpio_put(LED0, 1);
	clearCharString(command, commandLength); 
	ledStartUp();
	//serialNumber =  readDeviceSerial(i2c1);
	printf("Starting Main Loop \n");	
	while(1){
		if(uart_is_readable(uart0)){
			char c = uart_getc(uart0);
			processCommandChar(command, commandLength, c);
			}		
		if(currentCycle >= maxCycles){
			checkBatHealth(false);
			currentCycle = 0;
		}
		currentCycle += 1;
			
	}



}

