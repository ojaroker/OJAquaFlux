// Define the Arduino analog pins that connect to the CH4 and pressure sensors
#define CH4sens A0 // CH4 sensor 
#define Vb A1 // CH4 sensor reference circuit
#define P A2 // differential pressure sensor
int CH4s = 0;
int Vbat = 0;
//int Press = 0;

float CH4smV = 0;
float VbatmV = 0;
//float PmV = 0;

float mV = 5000; // voltage (5V)
float steps = 1024; // steps for ADC
