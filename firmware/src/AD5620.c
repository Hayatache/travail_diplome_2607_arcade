// ==== Pilotage AD5620 (DAC 12 bits) sur SPI1 ====
// A ajouter dans Mc32SpiUtil.c / .h
#include "app.h"
#include "Mc32SpiUtil.h"
#include "stdbool.h"
#include "AD5620.h"

#include "peripheral/tmr/plib_tmr.h"


//----------------------------------------------------------------------------------//
//-- nom fct : dac_ad5620_write
//-- paramètre entrée : uint16_t mode_bits, uint16_t code12
//-- paramètre sortie : aucune
//-- description : construit et envoie un mot de 16 bits au DAC AD5620 via SPI
//----------------------------------------------------------------------------------//
void dac_ad5620_write(uint16_t mode_bits, uint16_t code12)
{
    uint16_t word = mode_bits | ((code12 & 0x0FFFu) << 2);  // construit le mot  16 bits a envoyer au DAC
    /*separe le mot en deux octets */
    uint8_t  msb  = (uint8_t)(word >> 8);
    uint8_t  lsb  = (uint8_t)(word & 0xFF);

    /* met la CS a 1 au cas ou il aurait été descendu precedement */
    PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_B, PORTS_BIT_POS_1,true);
    /* gestion du CS */
    PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_B, PORTS_BIT_POS_1,false);
    /*envoie des deux octets au dac */
    spi_write1(msb);         // D15..D8 (MSB en premier)
    spi_write1(lsb);         // D7..D0
    /* suite de la gestion du dac */
    PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_B, PORTS_BIT_POS_1,true);
}

//----------------------------------------------------------------------------------//
//-- nom fct : dac_ad5620_set_min
//-- paramètre entrée : aucun
//-- paramètre sortie : aucune
//-- description : configure la sortie du DAC à sa valeur minimale
//----------------------------------------------------------------------------------//
void dac_ad5620_set_min(void)
{
    dac_ad5620_write(AD5620_MODE_NORMAL, AD5620_CODE_MIN);
}

//----------------------------------------------------------------------------------//
//-- nom fct : dac_ad5620_set_max
//-- paramètre entrée : aucun
//-- paramètre sortie : aucune
//-- description : configure la sortie du DAC à sa valeur maximale
//----------------------------------------------------------------------------------//
void dac_ad5620_set_max(void)
{
    dac_ad5620_write(AD5620_MODE_NORMAL, AD5620_CODE_MAX);
}

//----------------------------------------------------------------------------------//
//-- nom fct : dac_ad5620_generate_pulse
//-- paramètre entrée : aucun
//-- paramètre sortie : aucune
//-- description : alterne entre la valeur minimale et maximale du DAC
//----------------------------------------------------------------------------------//
void dac_ad5620_generate_pulse()
{
    static bool minormax = true;
    
    if(minormax){
        minormax = false;
        dac_ad5620_set_max();
    }
    else{
        minormax = true;
        dac_ad5620_set_min();
    }

}

//----------------------------------------------------------------------------------//
//-- nom fct : Audio_SetFrequency
//-- paramètre entrée : float frequency
//-- paramètre sortie : aucune
//-- description : configure la période du Timer 3 pour générer la fréquence audio demandée
//----------------------------------------------------------------------------------//
void Audio_SetFrequency(float frequency)
{
    unsigned int timer_value;

    if (frequency <= 0)
    {
        return;
    }

    timer_value = (unsigned int)(5000000.0 / frequency);

     PLIB_TMR_Period16BitSet(TMR_ID_3,timer_value);
}

//----------------------------------------------------------------------------------//
//-- nom fct : Play_Sound_1_Tick
//-- paramètre entrée : SOUND_DATA *ptr_Sound_data
//-- paramètre sortie : aucune
//-- description : joue les différentes notes du son et gère leur enchaînement
//----------------------------------------------------------------------------------//
void Play_Sound_1_Tick(SOUND_DATA *ptr_Sound_data)
{
    /* variable pour memorisé la note actuelle a jouer*/
    static uint8_t current_note = 1;

    if(ptr_Sound_data->nb_note > 4)
    {
        ptr_Sound_data->nb_note = 4;
    }

    if(ptr_Sound_data->nb_note < 1)
    {
        ptr_Sound_data->nb_note = 1;
    }

    /* selectionne la frequence de la note parametrer au prealable*/
    switch(current_note)
    {
        case 1:
            Audio_SetFrequency(ptr_Sound_data->frequency_note_1);
            break;

        case 2:
            Audio_SetFrequency(ptr_Sound_data->frequency_note_2);
            break;

        case 3:
            Audio_SetFrequency(ptr_Sound_data->frequency_note_3);
            break;

        case 4:
            Audio_SetFrequency(ptr_Sound_data->frequency_note_4);
            break;
    }

    if(ptr_Sound_data->sound_for_a_tick)
    {
        PLIB_PORTS_PinWrite(PORTS_ID_0,PORT_CHANNEL_C,PORTS_BIT_POS_4,false);
        ptr_Sound_data->sound_for_a_tick = false;
        current_note = 1;
    }
    else
    {

        if(current_note > ptr_Sound_data->nb_note)
        {
            current_note = 1;
            PLIB_PORTS_PinWrite(PORTS_ID_0,PORT_CHANNEL_C,PORTS_BIT_POS_4, true);
        }
        else{
            current_note++;
        }
    }
}