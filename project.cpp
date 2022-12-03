/* project.cpp
* by Joshua Ryan
* CPE 522 Team Project
* Adapted from subscribe.cpp and publish.cpp in ~/exploringBB/chp11/adafruit
* To call this program use: virtualLEDSwitch
* This program takes no arguments and will output the ambient temperature and
* take the state of a heater and air conditioner as inputs on:
*   https://io.adafruit.com/JoshuaRyan99/dashboards/beaglebone-temperature
*/

#include <iostream>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "MQTTClient.h"
using namespace std;

#define ADDRESS     "tcp://io.adafruit.com:1883"
#define CLIENTID    "Josh_Beagle"
#define TOPIC1      "JoshuaRyan99/feeds/weather.temperature"
#define TOPIC2      "JoshuaRyan99/feeds/weather.heater"
#define TOPIC3      "JoshuaRyan99/feeds/weather.air-conditioner"
#define QOS         1
#define TIMEOUT     10000L
#define LED_GPIO    "/sys/class/gpio/"
#define ADC_PATH    "/sys/bus/iio/devices/iio:device0/in_voltage"
#define ADC         0


// Use this function to control the LED
void writeGPIO(string gpio, string filename, string value){
   fstream fs;
   string path(LED_GPIO);
   fs.open((path + gpio + "/" + filename).c_str(), fstream::out);
   fs << value;
   fs.close();
}

float getTemperature(int adc_value) {     // from the TMP36 datasheet
   float cur_voltage = adc_value * (1.80f/4096.0f); // Vcc = 1.8V, 12-bit
   float diff_degreesC = (cur_voltage-0.75f)/0.01f;
   return (25.0f + diff_degreesC);
}

int readAnalog(int number){
   stringstream ss;
   ss << ADC_PATH << number << "_raw";
   fstream fs;
   fs.open(ss.str().c_str(), fstream::in);
   fs >> number;
   fs.close();
   return number;
}

volatile MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt) {
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    int i;
    char* payloadptr;
    payloadptr = (char*) message->payload;
    int switchState = atoi(payloadptr);
    if(strcmp(topicName, TOPIC2) == 0){
        switchState == 1 ? writeGPIO("gpio60", "value", "1") : writeGPIO("gpio60", "value", "0");
    }
    else{
        switchState == 1 ? writeGPIO("gpio48", "value", "1") : writeGPIO("gpio48", "value", "0");
    }
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause) {
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main(int argc, char* argv[]) {
    writeGPIO("gpio60", "direction", "out");
    writeGPIO("gpio48", "direction", "out");
    int rc;
    int ch;
    char str_payload[100];
    string tpc2 = TOPIC2;
    string tpc3 = TOPIC3;
    char *const topicArray[2] ={strcpy(new char[tpc2.length()+1], tpc2.c_str()),
     strcpy(new char[tpc3.length()+1], tpc3.c_str())};
    int qos2 = 1;
    int qos3 = 1;
    int qosArray[2] = {qos2, qos3};

    MQTTClient client;
    MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    opts.keepAliveInterval = 0;
    opts.cleansession = 1;
    opts.username = getenv("IO_USERNAME");
    opts.password = getenv("IO_KEY");
    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);   
    
    if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
        exit(-1);
    }
    
    MQTTClient_subscribeMany(client, 2, topicArray, qosArray);

    int cntr = 0;
    while(1){
        if(cntr >=100000){
           sprintf(str_payload, "%f", getTemperature(readAnalog(ADC)));
           pubmsg.payload = str_payload;
           pubmsg.payloadlen = strlen(str_payload);
           pubmsg.qos = QOS;
           pubmsg.retained = 0;
           MQTTClient_publishMessage(client, TOPIC1, &pubmsg, &token);
           rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
//           if(rc != MQTTCLIENT_SUCCESS) {
//               cout << "Did not complete with error code: " << rc << endl;
               // MQTTCLIENT_SUCCESS 0           MQTTCLIENT_FAILURE -1
               // MQTTCLIENT_DISCONNECTED -3     MQTTCLIENT_MAX_MESSAGES_INFLIGHT -4
               // MQTTCLIENT_BAD_UTF8_STRING -5  MQTTCLIENT_NULL_PARAMETER -6
               // MQTTCLIENT_TOPICNAME_TRUNCATED -7   MQTTCLIENT_BAD_STRUCTURE -8
               // MQTTCLIENT_BAD_QOS   -9        MQTTCLIENT_SSL_NOT_SUPPORTED   -10
//           }
           cntr = 0;
        }
        else{
            int numRead;
            char buf[1];
            cntr++;
            fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
            usleep(100);
            numRead = read(0, buf, 4);
            if(buf[0]=='Q' || buf[0]=='q'){
                break;
            }
        }
    }

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}

