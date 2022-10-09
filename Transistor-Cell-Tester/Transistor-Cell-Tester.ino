/*
    This sketch demonstrates how we can output a value in both channels of MCP4822 or MCP4812 or MCP4802.
*/

#include <MCP48xx.h>

// Define the MCP4822 instance, giving it the SS (Slave Select) pin
// The constructor will also initialize the SPI library
// We can also define a MCP4812 or MCP4802
MCP4822 dac(10);

// We define an int variable to store the voltage in mV so 100mV = 0.1V
int dac_voltage = 0;

int v_sensorPin = A0;
int v_sensorValue = 0;
int c_sensorPin = A1;
int c_sensorValue = 0;
float solar_voltage = 0.0;
float solar_current = 0.0;

int single_shot_samples = 100;
int dac_start = 0;
int dac_end = 0;
int dac_steps = 1;
int dac_increment = 0;
int dac_delay = 100;

void(* resetFunc) (void) = 0;

void setup() {

    Serial.begin(9600);
    while (!Serial) {
      ; // wait for serial port to connect. Needed for native USB port only
    }
  
    // We call the init() method to initialize the instance
    dac.init();

    // The channels are turned off at startup so we need to turn the channel we need on
    dac.turnOnChannelA();
    //dac.turnOnChannelB();

    // We configure the channels in High gain
    // It is also the default value so it is not really needed
    dac.setGainA(MCP4822::High);
    //dac.setGainB(MCP4822::High);

    set_dac();
    Serial.println("# READY");
}


void loop() {
    char data [21];
    int number_of_bytes_received;
    if(Serial.available() > 0) {
      delay(40);
      number_of_bytes_received = Serial.readBytesUntil (32,data,20); // read bytes (max. 20) from buffer, untill <CR> (13) (or <SPACE> (32) ). store bytes in data. count the bytes recieved.
      data[number_of_bytes_received] = 0; // add a 0 terminator to the char array
      //Serial.println('\n'); 
      //Serial.println(data);

      if(strstr(data,"*RST")) { // Reset
         Serial.println("Resetting..");
         delay(1000);
         resetFunc();
      }
      else if(strstr(data,":READ?")) { // measure single shot
        measure();
      }
      else if(strstr(data,":SWEEP?")) { // measure sweep
        dac_start = 2700;
        dac_end = 3400;
        dac_steps = 100;
        sweep();
      }
      else if(strstr(data,":SETV?")) { // set voltage, only for debugging
        dac_voltage = 4000;
        set_dac();
      }
      else if(strstr(data,":SWEEPUI?")) { // measure sweep adaptive
        dac_start = 0;
        dac_end = 4000;
        sweep_ui();
      }
      else {}
    }
    else {
      Serial.flush();  // in case of garbage serial data, flush the buffer
    }
    
    delay(100);

}

void set_dac() {
    // We set channel A to output
    dac.setVoltageA(dac_voltage);
    // We set channel B to output
    //dac.setVoltageB(dac_voltage);
    
    // We send the command to the MCP4822
    // This is needed every time we make any change
    dac.updateDAC();
    delay(dac_delay);
}

void sweep() {
  Serial.println("# SWEEP START");
  if (dac_steps <= 1) {
    dac_increment = dac_end - dac_start;
  }
  else {
    dac_increment = (dac_end - dac_start) / dac_steps; 
  }
  dac_voltage = dac_start;
  while (dac_voltage <= dac_end) {
    //Serial.println(dac_voltage);
    set_dac();
    measure();
    print_data();
    dac_voltage += dac_increment;
  }
  dac_voltage = 0;
  set_dac();
  Serial.println("# SWEEP END");
}

void sweep_ui() {
  Serial.println("# SWEEP START");
  dac_increment = 50;
  // Nehme Start-Wert auf
  dac_voltage = dac_start;
  set_dac();
  measure();
  print_data();
  int v_sensorValue_old = v_sensorValue;
  int c_sensorValue_old = c_sensorValue;

  int dac_voltage_less = dac_voltage;

  // Versuche Wert zu finden, der zwischen 10 und 15 von altem Sensor-Wert abweicht
  while (dac_voltage < dac_end-dac_increment) {
    set_dac();
    measure();
    int diff_v = abs(v_sensorValue_old-v_sensorValue);
    int diff_c = abs(c_sensorValue_old-c_sensorValue);
    if (diff_v<10 && diff_c<10) { // muss weiter laufen
      //Serial.println(String(diff_v)+" "+String(diff_c)+" increase");
      dac_voltage_less = dac_voltage;
      dac_voltage += dac_increment;
    }
    else if (diff_v>15 || diff_c>15) { // Zu weit gelaufen
      // setze auf Mitte
      //Serial.println(String(diff_v)+" "+String(diff_c)+" decrease");
      dac_voltage = dac_voltage_less + (dac_voltage-dac_voltage_less)/2;
    }
    else { //Fenster erwischt
      print_data();
      v_sensorValue_old = v_sensorValue;
      c_sensorValue_old = c_sensorValue;
      int diff_v = dac_voltage - dac_voltage_less;
      dac_voltage_less = dac_voltage;
      dac_voltage += diff_v;
    }
  }

  // Nehme End-Wert auf
  dac_voltage = dac_end;
  set_dac();
  measure();
  print_data();
  dac_voltage = 0;
  set_dac();
  Serial.println("# SWEEP END");
}

void measure() {
  solar_voltage = 0;
  solar_current = 0;
  for (int i = 0; i<single_shot_samples; i++) { // Measure multiple times and print average to Serial
      v_sensorValue = analogRead(v_sensorPin);
      solar_voltage += (1.0/single_shot_samples) * v_sensorValue * (5.0/1023.0);
      c_sensorValue = analogRead(c_sensorPin);
      solar_current += (1.0/single_shot_samples) * c_sensorValue * (5.0/1023.0);
    } 
}

void print_data() {
    //Serial.print("V:");
    Serial.print(solar_voltage, 4);
    Serial.print("\t");
    //Serial.print("I:");
    Serial.println(solar_current, 4);  
}
