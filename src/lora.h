#ifndef LORA_H
#define LORA_H

int setLoraPins(int txd, int lprxd, int rst);
int passThroughLora();
int setLoraParams();
int encodeLocationMessage(char *output, float * positionLat, float * positionLng);
int loraSend(char * hexBytes, int len);
int activateLora();

#endif
