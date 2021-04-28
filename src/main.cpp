#include <Arduino.h>
#include "signalgenerator.hpp"

void setup() {
  SignalGenerator sig;
  sig.populateBuffer(1000);
  sig.initializeDMAC();
}

void loop() {
  // put your main code here, to run repeatedly:
}