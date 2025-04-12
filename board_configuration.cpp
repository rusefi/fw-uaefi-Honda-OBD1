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

static Gpio OUTPUTS[] = {
	Gpio::MM100_MEGA_UAEFI_INJ1, // A1 INJ_1
	Gpio::MM100_INJ4, // A2 INJ_4
	Gpio::MM100_INJ2, // A3 INJ_2
	Gpio::MM100_INJ3, // A5 INJ_3
	Gpio::MM100_OUT_PWM2, // Fuel Pump Relay
	Gpio::MM100_INJ8, // A9 IAC
	Gpio::MM100_INJ7, // A13 MIL
	Gpio::MM100_IGN7, // A21 ICM Coil Control
	Gpio::MM100_IGN8, // Radiator Fan Control Module
	Gpio::MM100_OUT_PWM1, // A/C compressor clutch relay
	Gpio::MM100_INJ6, // IAB intake manifold butterflies solenoid
	Gpio::MM100_IGN6, // VTEC Solenoid Valve
};

int getBoardMetaOutputsCount() {
    return efi::size(OUTPUTS);
}

int getBoardMetaLowSideOutputsCount() {
    return getBoardMetaOutputsCount() - 1;
}

Gpio* getBoardMetaOutputs() {
    return OUTPUTS;
}
