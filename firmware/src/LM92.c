#include "LM92.h"
#include "Mc32_I2cUtilCCS.h"

/*----------------------------------------------------------*/
/* Configuration LM92                                       */
/*----------------------------------------------------------*/

/*
 * Adresse I2C 7 bits du LM92.
 *
 * A1 = 0
 * A0 = 0
 *
 * Adresse = 0x48
 */
#define LM92_I2C_ADDRESS       0x49

/* Registre température */
#define LM92_TEMP_REGISTER     0x00

/*----------------------------------------------------------*/
/* Lecture température                                      */
/*----------------------------------------------------------*/

float LM92_GetTemperature(void)
{
    uint8_t msb;
    uint8_t lsb;
    uint16_t raw;
    int16_t temperature_raw;

    /*
     * START
     */
    i2c_start();

    /*
     * Adresse LM92 + WRITE
     */
    i2c_write(LM92_I2C_ADDRESS << 1);

    /*
     * Sélection du registre température
     */
    i2c_write(LM92_TEMP_REGISTER);

    /*
     * RESTART
     */
    i2c_reStart();

    /*
     * Adresse LM92 + READ
     */
    i2c_write((LM92_I2C_ADDRESS << 1) | 1);

    /*
     * Lecture des deux octets.
     *
     * ACK après le MSB :
     * on demande encore le LSB.
     *
     * NACK après le LSB :
     * c'est le dernier octet.
     */
    msb = i2c_read(true);
    lsb = i2c_read(false);

    /*
     * STOP
     */
    i2c_stop();

    /*
     * Reconstruction des 16 bits
     */
    raw = ((uint16_t)msb << 8) | lsb;

    /*
     * Les données température sont D15 à D3.
     *
     * D15 = bit de signe
     * D14..D3 = température
     *
     * On décale donc de 3 bits.
     *
     * Le cast en int16_t AVANT le décalage
     * permet de conserver le signe.
     */
    temperature_raw = ((int16_t)raw) >> 3;

    /*
     * Résolution LM92 :
     * 0,0625 °C / bit
     */
    return temperature_raw * 0.0625f;
}