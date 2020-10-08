#define F_CPU 16000000 
#include <avr/io.h> 
#include <EEPROM.h>

float maxpower;// defines the maxium power in amperes
int savedState;//integral used to store the saved powerstate
int startPWM;
int maxPWM;// defines the maxium PWM for given powerstate, mainly to protect the device from shorting the mosfet...
int PWM;


float triggerlimitl = 0.9; //allowed drift upwards (percentage multiplier)
float triggerlimith = 1.1; //allowed drift downwards (percentage multiplier)
int updatepwm = 1; // on off state for updating the pwm, reduces flickering
int updatedelay = 0; // delay for pulling from the analog pin when measuring current
int updatefreq = 75; // amount of times the current value is sampled
int start; //amount of times current is sampled at startup, to get the current quickly to right value with updatedelay of "0" and the rise it to make it more stable (but also way slower)

float voltage;  //float variable for measuring voltage  


float lastpower; //float for the last power level (in amps)

int Vo;  //stuff for measuring temperature
float R1 = 10000; //resistance of the ntc thermistor while it is at around 25C
float logR2, R2, T;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;



void setup() {

  DDRB |=(1<<PB0)|(1<<PB1);
  // Enable PLL and async PCK for high-speed PWM
  PLLCSR |= (1 << PLLE) | (1 << PCKE);
  TCCR1 = 1<<PWM1A | 0<<COM1A1 | 1<<COM1A0 | 0<<CS13 | 0<<CS12 | 1<<CS11 | 1<<CS10;
  GTCCR = 0<<PWM1B | 0<<COM1B1 | 0<<COM1B0;
  OCR1C = 188;
  delay(10);
  OCR1A = 0;  // (103+1)/(205+1) = 0.50 = 50% duty cycle

  pinMode(A1, INPUT); //input for sensing battery voltage
  pinMode(A2, INPUT); //input for sensing current
  pinMode(A3, INPUT); //input for sensing temperature near Fet, Diode and Choce 
  readvoltage();
  powerstate();
//  EEPROM.write(0,0);
  

}

void loop() {

  
if (updatepwm == 1){
  OCR1A = PWM;
  updatepwm = 0;
}

readcurrent();
readTC();
if(start > 70){
readvoltage();
}
delay(1);



  
}






void powerstate() {
  savedState = EEPROM.read(0);


  switch (savedState) {
    case 0: maxpower = 0.1 ; startPWM = 10; maxPWM = 30; EEPROM.write(0, 1);  break;
    case 1: maxpower = 0.25; startPWM = 32; maxPWM = 59; EEPROM.write(0, 2); break;
    case 2: maxpower = 0.5;  startPWM = 60; maxPWM = 70; EEPROM.write(0, 3); break;
    case 3: maxpower = 1.0;  startPWM = 84; maxPWM = 98; EEPROM.write(0, 4); break;
    case 4: maxpower = 2.0;  startPWM = 108;maxPWM = 112;EEPROM.write(0, 0); break;
    }
    lastpower = maxpower;
    
    if(startPWM == 0){EEPROM.write(0,0);}
    for (PWM = 0; PWM < startPWM; PWM++){OCR1A = PWM; delay (5);}
}




void readcurrent(){
float AcsValue=0.0,Samples=0.0,AvgAcs=0.0,AcsValueF=0.0;

  for (int x = 0; x < updatefreq; x++){ //Get 150 samples
  AcsValue = analogRead(A2);     //Read current sensor values   
  Samples = Samples + AcsValue;  //Add samples together
  delay (updatedelay); // let ADC settle before next sample 3ms
  }start++;
  if(start > 50){
  updatefreq = 300;
  updatedelay = 10;
  }
AvgAcs=Samples/updatefreq;//Taking Average of Samples

AcsValueF = (2.57 - (AvgAcs * (5.06 / 1024.0)) )/-0.185;




  

if(AcsValueF*(triggerlimith) < maxpower){if(PWM < maxPWM){PWM++;} updatepwm = 1;} 
if(AcsValueF*(triggerlimitl) > maxpower){PWM--; updatepwm = 1;}

delay(10);
  
  
}

void readvoltage(){

  voltage = analogRead(A1);
  voltage = (voltage/1024)*5;
 if(voltage <= 3.30){maxPWM = 56;}
 if(voltage <= 3.15){maxPWM = 18;}
 if(voltage <= 3.12){maxPWM = 10;}
 if(voltage <= 3.08){maxPWM = 5;}

  
  if(PWM > maxPWM){PWM = maxPWM; updatepwm = 1;}
}

void readTC(){

  Vo = analogRead(A3);
  R2 = R1 * (1023.0 / (float)Vo - 1.0);
  logR2 = log(R2);
  T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
  T = T - 273.15;
  if(T > 50){if(lastpower > 1.01){maxpower = 1.1;}}
  else{ maxpower = lastpower;}
  
  


}
