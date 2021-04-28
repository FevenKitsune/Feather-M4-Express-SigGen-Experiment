#include <Arduino.h>
#include "signalgenerator.hpp"

void setup() {
  SignalGenerator sig(5);
  sig.populateBuffer(1000);
  sig.initializeDMAC();
}

void loop() {
  // put your main code here, to run repeatedly:
}