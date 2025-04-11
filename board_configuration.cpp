#include "pch.h"
#include "mega-uaefi.h"
#include "hellen_meta.h"

// board-specific configuration setup
void setBoardDefaultConfiguration() {
    setMegaUaefiBoardDefaultConfiguration();

	engineConfiguration->injectionPins[0] = Gpio::MM100_MEGA_UAEFI_INJ1;
  	engineConfiguration->injectionPins[1] = Gpio::MM100_INJ2;
  	engineConfiguration->injectionPins[2] = Gpio::MM100_INJ3;
  	engineConfiguration->injectionPins[3] = Gpio::MM100_INJ4;

    engineConfiguration->ignitionPins[0] = Gpio::MM100_IGN7; // ICM Ignition control module
//	engineConfiguration->ignitionPins[0] = Gpio::MM100_IGN1;
//	engineConfiguration->ignitionPins[1] = Gpio::MM100_IGN2;
//	engineConfiguration->ignitionPins[2] = Gpio::MM100_IGN3;
//	engineConfiguration->ignitionPins[3] = Gpio::MM100_IGN4;

    engineConfiguration->fuelPumpPin = Gpio::MM100_OUT_PWM2;
    engineConfiguration->idle.solenoidPin = Gpio::MM100_INJ8;


    engineConfiguration->triggerInputPins[0] = Gpio::MM100_UART8_TX; // VR2 max9924 is the safer default

	engineConfiguration->tps1_1AdcChannel = MM100_IN_TPS_ANALOG;
	engineConfiguration->clt.adcChannel = MM100_IN_CLT_ANALOG;
	engineConfiguration->iat.adcChannel = MM100_IN_IAT_ANALOG;

}