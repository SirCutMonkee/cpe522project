/*
* file name: AnalogIn.cpp
* Created by Joshua Ryan on November 10, 2022.
*
* This class wraps the functionality of analog to digital converter on the BBB.
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include "Analogin.h"

using namespace std;

#define ADC_PATH "/sys/bus/iio/devices/iio:device0/in_voltage"

AnalogIn::AnalogIn(){
   number = 0;
   cout << "Creating an AnalogIn object with number = 0" << endl;
}

AnalogIn::AnalogIn(unsigned int n){
   number = n;
   cout << "Creating an AnalogIn object with number = " << number << endl;
}


AnalogIn::~AnalogIn(){
   cout << "Destroying the AnalogIn object." << endl;
}

unsigned int AnalogIn::getNumber(){
   cout << "Number = " << number << endl;
   return number;
}

void AnalogIn::setNumber(unsigned int n){
   number = n;
   cout << "Setting number = " << number << endl;
}

int AnalogIn::readADCSample(){
   stringstream ss;
   ss << ADC_PATH << number << "_raw";
   fstream fs;
   fs.open(ss.str().c_str(), fstream::in);
   fs >> number;
   fs.close();
   return number;
}
