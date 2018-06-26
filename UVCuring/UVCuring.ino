/*
    UV Curing chamber code
    Christopher von Nagy and Ryan Bell
    Sage Ridge School
    Copyright 2018
    This code is intended to run our curing
    chamber for photopolymer prints
    Usage:
      Press the control button once to start the curing cycle. The UV lights will immediately turn on and
      the heating bed will start to warm. The target temperature for the curing oven is set to 60 C, the
      suggested temperature for curing FormLabs clear resin. Once the target temperature is reached, the
      heating plate should cycle on and off via one of the two relays.
      Press the control button twice in rapid succession and the curing process will immediately abort.
      While on the box will be red.
      While heating the box will be red with flashing.
      The large top button will illuminate if power is provided to the Arduino.
      Power must be provided to both the Arduino and to the heating bed / LED power supply
      for the system to operate.
      Normally open relays close when pulled low.
*/

// Define some constants
// NUMSAMPLES: Number of samples for averaged R reading from thermistor
// THERMISTORNOMINAL is used in the Steinhart-Hart equation to convert R to temp
// TEMPERATURENOMINAL  is used in the Steinhart-Hart equation to convert R to temp
// BCOEFFICIENT is used in the Steinhart-Hart equation to convert R to temp
// RESISTANCE is used in conjunction with the thermistor to convert R (Ohms) to V (Voltage). Install resistors are 100K Ohm.
#define NUMSAMPLES 5
#define THERMISTORNOMINAL 100000
#define TEMPERATURENOMINAL 25
#define BCOEFFICIENT 3950
#define RESISTANCE 100000

// Analog pin assignments for thermistors (100K)
const int box_thermistor = 0;   // Yellow wire 100K Ohm
const int plate_thermistor = 1; // White wire 100K Ohm

// Digital pin assignments
const int relay_one = 12; // Output. Orange wire. Contols heater bed.
const int relay_two = 11; // Output. Blue wire. Controls UV LEDs. Center positive.
const int go_button = 8; // Input. White wire. The top button is used to initiate the curing cycle.
const int LED01 = 3;      // First red LED
const int LED02 = 4;      // Second red LED
const int LED03 = 5;      // Power LED. Currently illuminates box in amber when power is on.
// Might be used to indicate when the main power supply is on.

// Targets
const float target_temp = 60;        // Target curing chamber temperature in degrees C
unsigned long target_time = 7200000; // Target curing time. 120 minutes in milliseconds is 7,200,000

// Control variables
int samples[NUMSAMPLES]; // Array to hold a set of resistance readings for averaging
float box_temp;            // The box temperature as read from the box thermistor
float plate_temp;          // The plate temperature as read from the plate thermistor
boolean heating;         // Heating element is on or off.

// Control button
boolean cycle_run = 0; // If true, the curing process is running
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 75;    // the debounce time; increase if bouncing still occurs
unsigned long start_time;
unsigned long finish_time;



// Setup
void setup() {

  // Input pins
  pinMode(box_thermistor, INPUT);
  pinMode(plate_thermistor, INPUT);
  pinMode(go_button, INPUT_PULLUP);

  // Output pins
  pinMode(relay_one, OUTPUT);
  pinMode(relay_two, OUTPUT);
  pinMode(LED01, OUTPUT);
  pinMode(LED02, OUTPUT);
  pinMode(LED03, OUTPUT);

  // Set initial states
  digitalWrite(relay_one, LOW); // The relay is NC. Open with LOW, closed with HIGH.
  digitalWrite(relay_two, LOW); // The relay is NC. Open with LOW, closed with HIGH.
  digitalWrite(LED01, LOW);     // Red LEDs are off. Amber on.
  digitalWrite(LED02, LOW);
  digitalWrite(LED03, HIGH);

  // Debugging information to serial monitor
  Serial.begin(9600);

  heating = 0;

}

//Run
void loop() {

  // Check ambient box temperature
  box_temp = check_Temperature(box_thermistor);
  plate_temp = check_Temperature(plate_thermistor);

  // Check the start button using a debounce algorithm
  // Start cycle with a single push. Abort with a double push.
  // We use millis() to detect a double push, control for bounce, and
  // keep track of the time for the 120 minute curing cycle.
  // There is a slight chance that the millis() counter would roll over
  // back to zero during a cycle (this happens every 50 days or so),
  // so we try to account for that. (Currently a FIXME)


  int reading = digitalRead(go_button);
  Serial.println(reading);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        cycle_run = !cycle_run;
        finish_time = millis() + target_time;
      }
    }
  }

  Serial.print("Button state: ");
  Serial.print(buttonState);
  Serial.print(" ");

  if (cycle_run) {
    start_time = millis();
  } else {
    finish_time = 0;
    start_time = 0;
  }

  // Run or terminate curing process
  if      (!cycle_run) {
    heating = 0;
    digitalWrite(relay_one, LOW);
    digitalWrite(relay_two, LOW);
    digitalWrite(LED01, LOW);
    digitalWrite(LED02, LOW);
    digitalWrite(LED03, HIGH);
  }
  else if (cycle_run && ( finish_time <= millis() )) {
    cycle_run = 0;
    heating = 0;
    digitalWrite(relay_one, LOW);
    digitalWrite(relay_two, LOW);
    digitalWrite(LED01, LOW);
    digitalWrite(LED02, LOW);
    digitalWrite(LED03, HIGH);
  }
  else if (cycle_run && ( finish_time > millis() ) && ( plate_temp < target_temp ) ) {
    heating = 1;
    digitalWrite(relay_one, HIGH);
    digitalWrite(relay_two, HIGH);
    digitalWrite(LED01, HIGH);
    digitalWrite(LED02, HIGH);
    digitalWrite(LED03, LOW);
  }
  else if (cycle_run && ( finish_time > millis() ) && ( plate_temp >= target_temp ) ) {
    heating = 0;
    digitalWrite(relay_one, HIGH);
    digitalWrite(relay_two, LOW);
    digitalWrite(LED01, HIGH);
    digitalWrite(LED02, LOW);
    digitalWrite(LED03, LOW);
  }
  else    {
    heating = 0;
    digitalWrite(relay_one, LOW);
    digitalWrite(relay_two, LOW);
    digitalWrite(LED01, LOW);
    digitalWrite(LED02, LOW);
    digitalWrite(LED03, HIGH);
  }

  // Calculate time remaining
  float minutes = (finish_time - start_time) / 60000;

  // Print status and temperature to serial monitor
  Serial.print(" Box temperature: ");
  Serial.print(box_temp);
  Serial.print(" C");
  Serial.print(" Plate temperature: ");
  Serial.print(plate_temp);
  Serial.print(" C");
  Serial.print(" Run: ");
  Serial.print(cycle_run);
  Serial.print(" Heat: ");
  Serial.print(heating);
  Serial.print(" Time remaining: ");
  Serial.println(minutes, 2);

  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonState = reading;


  // delay(10);

}


// Functions


// Measure thermisistor resistance and convert to degrees C
// Adafruit as a good tutorial on this,and most of this code
// is drawn from their tutorial.
// We use 5v which should be less noisy as normally the power is
// from a power supply and not USB. When connected to a computer
// to read serial output the temperature values will
// fluctuate a bit do to circuit noise

int check_Temperature(int p) {

  int tpin = p;    // Thermistor ADC value

  uint8_t i;
  float res;       // Measured resistance
  float average;   // ADC average
  float steinhart; // Temperature via Steinhard-Hart equation

  // Convert ADC value to resistance using an average
  // ADC value = R/(R + resistor_value) * Vcc * 1023/Varef
  // Board and, therefore, Varef is 5v
  // So, R = resistor_value/(1023/ADC - 1)
  // R = 100K / (1023/ADC - 1)

  // Take samples
  for (i = 0; i < NUMSAMPLES; i++) {
    samples[i] = analogRead(tpin);
    delay(10);
  }

  // Calculate average
  average = 0;
  for (i = 0; i < NUMSAMPLES; i++) {
    average += samples[i];
  }
  average /= NUMSAMPLES;

  // Convert to R in Ohms
  res = RESISTANCE / (1023 / average - 1);
  // Serial.print("Thermistor resistance ");
  // Serial.println(res, 0);

  // Use R to convert to C via the Steinhart-Hart equation
  // We use the suggested simplified B parameter equation and
  // estimate parameters.
  // 1/T = 1/T0 + 1/Bln(R/R0)
  // Where, T0 is 298.15K at 25 decrees C (room temperature)
  // and B, the coefficient of the thermistor, is 3950

  steinhart = res / THERMISTORNOMINAL;         // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert to C

  // Return the temperature value
  return (steinhart);
}
