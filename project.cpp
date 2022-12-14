/* project.cpp
* by Joshua Ryan
* CPE 522 Team Project
* Adapted from subscribe.cpp and publish.cpp in ~/exploringBB/chp11/adafruit
* To call this program use: project
* This program takes no arguments and will output the ambient temperature and state of a pushbutton, and
* take the state of a heater and momentary button as inputs on:
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
#include "GPIO.h"
#include "Analogin.h"
using namespace std;
using namespace exploringBB;

#define ADDRESS     "tcp://io.adafruit.com:1883"
#define CLIENTID    "Josh_Beagle"
#define TOPIC1      "JoshuaRyan99/feeds/weather.temperature"
#define TOPIC2      "JoshuaRyan99/feeds/weather.heater"
#define TOPIC3      "JoshuaRyan99/feeds/weather.led"
#define TOPIC4      "JoshuaRyan99/feeds/weather.button"
#define TOPIC5      "bareleus/feeds/weather.joshuatemp"
#define TOPIC6      "EmbeddedSp3zz/feeds/cpe-project.joshuatemp"
#define TOPIC7      "Bentan01/feeds/project-group.joshuatemp"
#define TOPIC8      "anhprotien588/feeds/project.joshuatemperature"
#define QOS         1
#define TIMEOUT     10000L

volatile MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt) {
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    static GPIO led(60);
    static GPIO heater(48);
    led.setDirection(OUTPUT);
    heater.setDirection(OUTPUT);    
    int i;
    char* payloadptr;
    payloadptr = (char*) message->payload;
    int switchState = atoi(payloadptr);
    if(strcmp(topicName, TOPIC3) == 0){
        switchState == 1 ? led.setValue(HIGH) : led.setValue(LOW);
    }
    else{
        switchState == 1 ? heater.setValue(HIGH) : heater.setValue(LOW);
    }
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause) {
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

void publish(MQTTClient *client, const char *topicName, float val){
    int rc;
    char str_payload[100];
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;    
    sprintf(str_payload, "%f", val);    
    pubmsg.payload = str_payload;
    pubmsg.payloadlen = strlen(str_payload);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    MQTTClient_publishMessage(*client, topicName, &pubmsg, &token);
    if(rc = MQTTClient_waitForCompletion(*client, token, TIMEOUT) != MQTTCLIENT_SUCCESS){
        exit(-1);
    }
}

int main(int argc, char* argv[]) {    
    int cntr = 0;
    float temp;
    
    AnalogIn tempSensor(0);
    
    GPIO button(46);
    button.setDirection(INPUT);
    GPIO_VALUE lastState;
    GPIO_VALUE currentState;
    currentState = button.getValue();
    if(currentState == LOW) lastState = HIGH;
    else lastState = LOW;

    int rc;
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
    
    while(1){
        if(currentState != lastState){
            if(currentState == HIGH) publish(&client, TOPIC4, 0);
            else publish(&client, TOPIC4, 1);
            lastState = currentState;
        }
        currentState = button.getValue();
        
        if(cntr >=100000){
            temp = 25.0f+((tempSensor.readADCSample()*(1.80f/4096.0f)-0.75f)/0.01f); 
            publish(&client, TOPIC1, temp);
            publish(&client, TOPIC5, temp);
            publish(&client, TOPIC6, temp);
            publish(&client, TOPIC7, temp);
            publish(&client, TOPIC8, temp);
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
    return 0;
}

