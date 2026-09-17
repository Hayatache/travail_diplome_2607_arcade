#include "LM92.h"
#include "Mc32_I2cUtilCCS.h"

/*----------------------------------------------------------*/
/* Configuration LM92                                       */
/*----------------------------------------------------------*/

/*
Adresse I2C 7 bits du LM92.
Adresse = 0x40
 */
#define LM92_I2C_ADDRESS       0x49

/* Registre température */
#define LM92_TEMP_REGISTER     0x00


//----------------------------------------------------------------------------------//
//-- nom fct : LM92_GetTemperature
//-- paramètre entrée : aucun
//-- paramètre sortie : float temperature
//-- description : lit la température du LM92 via le bus I2C
//----------------------------------------------------------------------------------//

float LM92_GetTemperature(void)
{
    uint8_t msb;
    uint8_t lsb;
    uint16_t raw;
    int16_t temperature_raw;

    /*START*/
    i2c_start();

    /*Adresse LM92 + WRITE*/
    i2c_write(LM92_I2C_ADDRESS << 1);

    /*Sélection du registre température*/
    i2c_write(LM92_TEMP_REGISTER);

    /*RESTART*/
    i2c_reStart();

    /*Adresse LM92 + READ*/
    i2c_write((LM92_I2C_ADDRESS << 1) | 1);

    /* Lecture des deux octets. */
    msb = i2c_read(true);
    lsb = i2c_read(false);

    i2c_stop();

    /*Reconstruction des 16 bits*/
    raw = ((uint16_t)msb << 8) | lsb;

    /* calcule de la temperautre brut */
    temperature_raw = ((int16_t)raw) >> 3;

    /* calcule de la temperautre numerique */
    return temperature_raw * 0.0625f;
}