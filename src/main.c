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


int initSystems(){
	printf("Beginning initialization....\n");
	i2c_init(i2c1, 100*1000);
	gpio_set_function(SDA_PIN, GPIO_FUNC_I2C); //SDA
	gpio_set_function(SCL_PIN, GPIO_FUNC_I2C); //SCL
	gpio_pull_up(SDA_PIN);
	gpio_pull_up(SCL_PIN);
	sleep_ms(100);
	
	setMCP4725Voltage(i2c1, 400, false);
	printf("Test\n");
	// Initialize GPIO Pins
	gpio_init(CHARGE);
	gpio_init(EEPROM_WC);


	//Set GPIO Pin Direction
	gpio_set_dir(CHARGE, GPIO_OUT);
	gpio_set_dir(EEPROM_WC, GPIO_OUT);
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


int main(){
	stdio_init_all();
	initSystems();
	sleep_ms(1000);
	gpio_put(CHARGE, 1);
	gpio_put(EEPROM_WC, 1);
	
	sleep_ms(1000);
	float adcMaxVoltage = 4.096;
	float shuntResistance = 0.001;

	
	while(1){
		uint16_t serialNumber =  readDeviceSerial(i2c1);

		int values[4] = {0, 0, 0, 0};
		getADS115AllValues(values, i2c1);
		printf("BatVoltage: %d, Temperature: %d, Charge Current: %d, DischargeCurrent: %d \n", values[0], values[1], values[2], values[3]);
		float batVoltage = calculateBatVoltage(values[0], adcMaxVoltage, 10, 100);
		float temperature = calculateTemperature((float)values[1], (float)50000, (float)3800);
		float chargeCurrent = calculateShuntCurrent(shuntResistance, (float)values[2], adcMaxVoltage);
		float dischargeCurrent = calculateShuntCurrent(shuntResistance, (float)values[3], adcMaxVoltage);
		printf("Serial: %04X: Bat Voltage: %.2fv, Temperature: %.2fC, Charge Current: %.2fA, Discharge Current: %.2fA \n",serialNumber, batVoltage, temperature, chargeCurrent, dischargeCurrent);
		sleep_ms(500);

	}



}

