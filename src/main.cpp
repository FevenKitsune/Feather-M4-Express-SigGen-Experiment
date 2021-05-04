#include <Arduino.h>
#include "signalgenerator.hpp"

void setup() {
  SignalGenerator sig(5, DAC0);
  SignalGenerator sig2(4, DAC1);
  sig.initializeDMAC();
  sig.populateBuffer(3000);
  sig.refreshDMAC();
}

void loop() {
  // put your main code here, to run repeatedly:
}