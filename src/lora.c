#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "general.h"


int lora_txd;
int lora_lprxd;
int lora_rst;


char cdeveui[] = "AT+CDEVEUI=8a9c97514332a8d1\r";
char cappeui[] = "AT+CAPPEUI=1770af77581d9760\r";
char cappkey[] = "AT+CAPPKEY=40cecc3989a153df10b33416612aa36e\r";
char freqMask[] = "AT+CFREQBANDMASK=0002\r";
char deviceClass[] = "AT+CCLASS=0\r";

int setLoraPins(int txd, int lprxd, int rst){
    lora_txd = txd;
    lora_lprxd = lprxd;
    lora_rst = rst;
}
int activateLora(){
    gpio_put(lora_rst, 1);
    gpio_set_function(lora_txd, UART_FUNCSEL_NUM(uart1, lora_txd));
    gpio_set_function(lora_lprxd, UART_FUNCSEL_NUM(uart1, lora_lprxd));
    uart_init(uart1, 9600);
    char message[500];
    int charCount = 0;
    while(true){
        if(uart_is_readable(uart1)){
            char c = uart_getc(uart1);
            message[charCount] = c;
            charCount++;
            if(c == '\r'){
		for(int i = 0; i < charCount; i++){
			uart_putc_raw(uart0, message[i]);
		}
                printf("\n");
                char test[] = "\nJoined\n\r";
                if(strcmp(test, message) == 0){
                    printf("Join Accepted \n");
                    busy_wait_ms(500);
                    break;
                }
                charCount = 0;
                resetTxtArr(message, 500);
            }
            
        }
    }
    
    printf("Lora has been activated... \n");
}
int deactivateLora(){
    gpio_put(lora_rst, 0);
    uart_deinit(uart1);
    gpio_set_function(lora_txd, GPIO_FUNC_NULL);
	gpio_set_function(lora_lprxd, GPIO_FUNC_NULL);
    busy_wait_ms(500);
    printf("Lora has been deactivated... \n");
}

int passThroughLora(){
    activateLora();
	while(true){
		if(uart_is_readable(uart0)){
			char c = uart_getc(uart0);
			uart_putc_raw(uart1, c);
		}
		if(uart_is_readable(uart1)){
			char c = uart_getc(uart1);
			uart_putc_raw(uart0, c);
		}
	}
}


int setLoraParams(){
    activateLora();
	char c;
	uart_puts(uart1, cdeveui);
	while(c != '\r'){
		c = uart_getc(uart1);
		printf("%c", c);
	}
	c = 0;
	busy_wait_ms(100);
	uart_puts(uart1, cappeui);
	while(c != '\r'){
		c = uart_getc(uart1);
		printf("%c", c);
	}
	c = 0;
	busy_wait_ms(100);
	uart_puts(uart1, cappkey);
	while(c != '\r'){
		c = uart_getc(uart1);
		printf("%c", c);
	}
	c = 0;
	busy_wait_ms(100);
	uart_puts(uart1, freqMask);
	busy_wait_ms(100);
	printf("\n Lora Network Parameters Have Been Set \n");

    deactivateLora();
}

int loraJoin(){
	char cjoin[] = "AT+CJOIN=1,1,8,8\r";
	uart_puts(uart1, cjoin);
	printf("Sleeping for 5 Seconds to wait for join \n");
	busy_wait_ms(5000);

}


int encodeLocationMessage(char *output, float * positionLat, float * positionLng){
    printf("Encoding Lcoation Message \n");
	int totalMessageChars = 0;
	char version[] = "00";
	if(*positionLat == 0.0 && *positionLng == 0.0){
		version[1] = '1';
	}
	for(int i = 0; i < 2; i++){
		output[totalMessageChars] = version[i];
		totalMessageChars +=1;
	}

	int floatSize = sizeof(float) * 2;
    char lat_hex_string[floatSize];
    char lng_hex_string[floatSize];
    resetTxtArr(lat_hex_string, floatSize);
    resetTxtArr(lng_hex_string, floatSize);


	float_to_hex_string(positionLat, lat_hex_string);
	float_to_hex_string(positionLng, lng_hex_string);


	for(int i = 0; i < floatSize; i++){
        if(lat_hex_string[i] != 0){
            output[totalMessageChars] = lat_hex_string[i];
            printf("%c", lat_hex_string[i]);
		    totalMessageChars += 1;
        }
		
	}
    printf("\nLNG:");
	for (int i = 0; i < floatSize; i++){
        if(lng_hex_string[i] != 0){
    		output[totalMessageChars] = lng_hex_string[i];
            printf("%c", lng_hex_string[i]);
    		totalMessageChars += 1;
        }
	}
    printf("\n");

	
    printf("Encoded Message: ");
    printf("%s \n", output);

    return totalMessageChars;
}



int loraSend(char * hexBytes, int len){
    activateLora();
	char sendData[1000];
	resetTxtArr(sendData, 1000);
        snprintf(sendData, sizeof(sendData), "AT+DTRX=0,5,%d,", len);
	uart_puts(uart1, sendData);
	printf("%s", sendData);

	for (int i = 0; i < len; i++){
		uart_putc_raw(uart1, hexBytes[i]);
		uart_putc_raw(uart0, hexBytes[i]);
	}
	uart_putc_raw(uart1, '\r');
    char message[500];
    int charCount = 0;
	while(true){
        if(uart_is_readable(uart1)){
            char c = uart_getc(uart1);
            message[charCount] = c;
            charCount++;
            if(c == '\r'){
                printf(".\n");
                char testSent[] = "\nOK+SENT:01\r";
                char testFailed[] = "\nERR+SENT:05\r";
		for(int i = 0; i < charCount; i++){
			uart_putc_raw(uart0, message[i]);
		}
                if(strcmp(testSent, message) == 0){
                    printf("Messsage sent successfully. \n");
                    busy_wait_ms(500);
                    break;
                }else if (strcmp(testFailed, message) == 0){
                    printf("Messaged Failed to Send. \n");
                    busy_wait_ms(500);
                    break;
                }
                charCount = 0;
                resetTxtArr(message, 500);
            }
            
        }
    }

	deactivateLora();

}
