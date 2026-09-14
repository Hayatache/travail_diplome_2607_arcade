// AD5620.h
// Driver DAC 12 bits AD5620CRMZ-1 (nanoDAC, ref interne 1.25V -> pleine echelle 2.5V)
//
//	Description : 	Pilotage du DAC AD5620 sur SPI1 (via Mc32SpiUtil)
//	Auteur 		: 	C. HUBER
//      Création	: 	13.09.2026
//
/*--------------------------------------------------------*/

#ifndef _AD5620_H
#define _AD5620_H

#include <stdint.h>

/* ---------------------------------------------------------------
 * Bits de mode (DB15-DB14 du registre d'entree 16 bits de l'AD5620)
 * --------------------------------------------------------------- */
#define AD5620_MODE_NORMAL         (0x0u << 14)   // fonctionnement normal
#define AD5620_MODE_PD_1K          (0x1u << 14)   // power-down, 1k vers GND
#define AD5620_MODE_PD_100K        (0x2u << 14)   // power-down, 100k vers GND
#define AD5620_MODE_PD_TRISTATE    (0x3u << 14)   // power-down, sortie haute impedance

/* ---------------------------------------------------------------
 * Codes utiles (12 bits, D11..D0)
 * --------------------------------------------------------------- */
#define AD5620_CODE_MIN             0x000u   // VOUT ~ 0 V
#define AD5620_CODE_MAX             0xFFFu   // VOUT ~ 2.5 V (version -1)
#define AD5620_CODE_MID             0x800u   // VOUT ~ 1.25 V


typedef struct
{
    float frequency_note_1;
    float frequency_note_2;
    float frequency_note_3;
    float frequency_note_4;
    uint8_t nb_note;
    bool sound_for_a_tick;
} SOUND_DATA;

/* ---------------------------------------------------------------
 * Prototypes
 * --------------------------------------------------------------- */

// Ecrit un mot complet (mode + code 12 bits) dans le registre du DAC
void dac_ad5620_write(uint16_t mode_bits, uint16_t code12);

// Positionne la sortie au minimum (~0 V)
void dac_ad5620_set_min(void);

// Positionne la sortie au maximum (~2.5 V)
void dac_ad5620_set_max(void);

// Positionne la sortie a une valeur 12 bits arbitraire (mode normal)
void dac_ad5620_set_code(uint16_t code12);

// Met le DAC en mode power-down (sortie a haute impedance, 1k ou 100k vers GND)
void dac_ad5620_power_down(uint16_t pd_mode);

// Genere une impulsion min <-> max en boucle bloquante
// half_period_us : demi-periode en microsecondes (>= ~20 us recommande)
void dac_ad5620_generate_pulse();

void Audio_SetFrequency(float frequency);
void Play_Sound_1_Tick(SOUND_DATA *ptr_Sound_data);


    
    
#endif /* _AD5620_H */