#include <stdio.h>

#include "Lepton_I2C.h"
#include "leptonSDKEmb32PUB/LEPTON_SDK.h"
#include "leptonSDKEmb32PUB/LEPTON_SYS.h"
#include "leptonSDKEmb32PUB/LEPTON_Types.h"

bool _connected;

LEP_CAMERA_PORT_DESC_T _port;
LEP_SYS_FPA_TEMPERATURE_KELVIN_T fpa_temp_kelvin;
LEP_SYS_FPA_TEMPERATURE_CELCIUS_T fpa_celsius;
LEP_SYS_AUX_TEMPERATURE_KELVIN_T aux_temp_kelvin;
LEP_RESULT result;


int lepton_connect() {
	LEP_OpenPort(1, LEP_CCI_TWI, 400, &_port);
	_connected = true;
	return 0;
}

void lepton_perform_ffc() {
	if(!_connected) {
		lepton_connect();
	}
	LEP_RunSysFFCNormalization(&_port);
}


double camera_tmpKelvin(){
    if(!_connected)
        lepton_connect();
    result = ((LEP_GetSysFpaTemperatureKelvin(&_port, &fpa_temp_kelvin)));
    //printf("FPA temp kelvin: %i, code %i\n", fpa_temp_kelvin, result);
    return (fpa_temp_kelvin);
}

double camera_tmpCelsius(){
    if(!_connected)
        lepton_connect();
    result = ((LEP_GetSysFpaTemperatureCelcius(&_port, &fpa_celsius)));
    //printf("FPA temp Celsius: %i, code %i\n", fpa_celsius, result);
    return (fpa_celsius);
}

double aux_tmpKelvin(){
    if(!_connected)
        lepton_connect();
    result = ((LEP_GetSysAuxTemperatureKelvin(&_port, &aux_temp_kelvin)));
    //printf("AUX temp kelvin: %i, code %i\n", aux_temp_kelvin, result);
    return (aux_temp_kelvin);
}

//Convert the Raw values in Celsius
float raw2Celsius(float raw){
    float ambientTemperature = 25.0;
    float slope = 0.0217;
    return slope*raw+ambientTemperature-177.77;
}
