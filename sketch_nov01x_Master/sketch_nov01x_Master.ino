#include <aREST.h>
#include <SPI.h>

// Create aREST instance
aREST rest = aREST();

const int ledPin = 13; // the pin that the LED is attached to
int incomingByte;      // a variable to read incoming serial data into
const String xbee_id = "MASTER";

void setup()
{
  // initialize serial communication:
  Serial.begin(9600);

  // Give name and ID to device
  rest.set_id(xbee_id);

  // initialize the LED pin as an output:
  pinMode(ledPin, OUTPUT);
}

void loop()
{
  // see if there's incoming serial data:
  if (Serial.available() > 0)
  {
    // read the oldest byte in the serial buffer:
    incomingByte = Serial.read();
    // if it's a capital H (ASCII 72), turn on the LED:
    if (incomingByte == 'H')
    {
      digitalWrite(ledPin, HIGH);
    }
    // if it's an L (ASCII 76) turn off the LED:
    if (incomingByte == 'L')
    {
      digitalWrite(ledPin, LOW);
    }
  }

  rest.handle(Serial);
}
