// ==== Pilotage AD5620 (DAC 12 bits) sur SPI1 ====
// A ajouter dans Mc32SpiUtil.c / .h
#include "app.h"
#include "Mc32SpiUtil.h"
#include "stdbool.h"
#include "AD5620.h"

#include "peripheral/tmr/plib_tmr.h"


// Ecrit un code 12 bits + mode dans le registre du DAC
void dac_ad5620_write(uint16_t mode_bits, uint16_t code12)
{
    uint16_t word = mode_bits | ((code12 & 0x0FFFu) << 2);  // bits 1-0 = don't care, mis à 0
    uint8_t  msb  = (uint8_t)(word >> 8);
    uint8_t  lsb  = (uint8_t)(word & 0xFF);

    PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_B, PORTS_BIT_POS_1,true);
    PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_B, PORTS_BIT_POS_1,false);
    spi_write1(msb);         // D15..D8 (MSB en premier)
    spi_write1(lsb);         // D7..D0
    PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_B, PORTS_BIT_POS_1,true);
}

void dac_ad5620_set_min(void)
{
    dac_ad5620_write(AD5620_MODE_NORMAL, AD5620_CODE_MIN);
}

void dac_ad5620_set_max(void)
{
    dac_ad5620_write(AD5620_MODE_NORMAL, AD5620_CODE_MAX);
}

// Génère une impulsion min <-> max en boucle
// half_period_us : demi-période en µs (voir contrainte plus bas)
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

void Play_Sound_1_Tick(SOUND_DATA *ptr_Sound_data)
{
    static uint8_t current_note = 1;

    /* Limitation du nombre de notes */
    if(ptr_Sound_data->nb_note > 4)
    {
        ptr_Sound_data->nb_note = 4;
    }

    if(ptr_Sound_data->nb_note < 1)
    {
        ptr_Sound_data->nb_note = 1;
    }

    /* Jouer la note actuelle */
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

    /* Gestion du son pendant un tick */
    if(ptr_Sound_data->sound_for_a_tick)
    {
        PLIB_PORTS_PinWrite(PORTS_ID_0,PORT_CHANNEL_C,PORTS_BIT_POS_4,false);
        ptr_Sound_data->sound_for_a_tick = false;
        current_note = 1;
    }
    else
    {
        /* Passer à la note suivante */
        

        /* Recommencer à la note 1 */
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