// Mc32Max7219.C
// Utilitaire d'affichage MAX7219/MAX7221 (6 digits)
//
//  Description :  Pilotage d'un afficheur 7-segments via MAX7219,
//                  en mode "no-decode" : chaque caractere ASCII est
//                  decode en segments (table de police), puis envoye
//                  automatiquement en SPI dans le registre "Digit"
//                  correspondant a sa position sur l'afficheur.
//                  Inclut une fonction de defilement de texte.
//
//  Depend de   :   Mc32SpiUtil.c / Mc32SpiUtil.h (spi_write1 / spi_write2)
//  Compilateur :   XC32 + Harmony
//  Version     :   V1.0
/*--------------------------------------------------------*/

#include <string.h>
#include <stdio.h>

#include "app.h"
#include "Mc32SpiUtil.h"
#include "Mc32Max7219.h"
#include "Ges_Menu.h"


/* ==========================================================
 *  Table de correspondance position affichee -> registre digit
 * ========================================================== */
// position 0 = digit le plus a GAUCHE de l'afficheur
// position (MAX7219_NB_DIGITS-1) = digit le plus a DROITE
// -> a inverser si le cablage physique de la carte est different
static const uint8_t MAX7219_DigitRegTable[8] =
{
    MAX7219_REG_DIGIT0, MAX7219_REG_DIGIT1, MAX7219_REG_DIGIT2,
    MAX7219_REG_DIGIT3, MAX7219_REG_DIGIT4, MAX7219_REG_DIGIT5,
    MAX7219_REG_DIGIT6, MAX7219_REG_DIGIT7
};

static uint8_t MAX7219_PosToReg(uint8_t position)
{
    return MAX7219_DigitRegTable[(MAX7219_NB_DIGITS - 1) - position];
}


/* ==========================================================
 *  Table de police 7-segments (mode no-decode)
 *  Format d'un octet segment : DP A B C D E F G  (Table 6 datasheet)
 *  Certaines lettres (K,M,Q,V,W,X) sont difficilement
 *  representables sur 7 segments : approximation "au mieux".
 * ========================================================== */
typedef struct
{
    char    car;
    uint8_t seg;
} MAX7219_Font_t;

static const MAX7219_Font_t MAX7219_Font[] =
{
    {'0', 0x7E}, {'1', 0x30}, {'2', 0x6D}, {'3', 0x79},
    {'4', 0x33}, {'5', 0x5B}, {'6', 0x5F}, {'7', 0x70},
    {'8', 0x7F}, {'9', 0x7B},

    {'A', 0x77}, {'b', 0x1F}, {'B', 0x1F}, {'C', 0x4E}, {'c', 0x0D},
    {'d', 0x3D}, {'D', 0x3D}, {'E', 0x4F}, {'F', 0x47}, {'G', 0x5E},
    {'H', 0x37}, {'h', 0x17}, {'I', 0x30}, {'i', 0x10},
    {'J', 0x3C}, {'K', 0x37}, {'L', 0x0E}, {'M', 0x00},
    {'n', 0x15}, {'O', 0x7E}, {'o', 0x1D}, {'P', 0x67},
    {'Q', 0x73}, {'q', 0x73}, {'r', 0x05}, {'S', 0x5B},
    {'t', 0x0F}, {'U', 0x3E}, {'u', 0x1C}, {'V', 0x3E},
    {'W', 0x3E}, {'X', 0x37}, {'Y', 0x3B}, {'y', 0x3B},
    {'Z', 0x6D}, {'z', 0x6D},

    {'-', 0x01}, {'_', 0x08}, {' ', 0x00}, {'=', 0x09}
};
#define MAX7219_FONT_SIZE   (sizeof(MAX7219_Font) / sizeof(MAX7219_Font[0]))






uint8_t MAX7219_CharToSeg(char c)
{
    uint8_t i;
    for (i = 0; i < MAX7219_FONT_SIZE; i++)
    {
        if (MAX7219_Font[i].car == c)
        {
            return MAX7219_Font[i].seg;
        }
    }
    return 0x00;   // caractere non reconnu -> digit eteint
}


/* ==========================================================
 *  Ecriture bas niveau d'un registre (LOAD/CS + trame SPI 16 bits)
 * ========================================================== */
void MAX7219_WriteReg(uint8_t reg, uint8_t data)
{
    
    PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_B, PORTS_BIT_POS_0,false);      // LOAD/CS bas : debut de trame, shift register actif

#if (MAX7219_SPI_CHANNEL == 1)
    spi_write1(reg);
    spi_write1(data);
#else
    spi_write2(reg);
    spi_write2(data);
    
#endif
    PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_B, PORTS_BIT_POS_0,true);

    delay_usCt(2);
    PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_B, PORTS_BIT_POS_0,false); 
    
   
}


/* ==========================================================
 *  Initialisation
 * ========================================================== */
void MAX7219_Init(SYS_INFO *ptr_systeme_info)
{

	MAX7219_WriteReg(0x0c, 0x01);       //  power down =0?normal mode = 1
    MAX7219_WriteReg(0x09, 0x00);       //  no decoding
	MAX7219_WriteReg(0x0a, ptr_systeme_info->sys_brightness);       //  brightness intensity
	MAX7219_WriteReg(0x0b, 0x05);       //  scan limit = 8 LEDs
	MAX7219_WriteReg(0x0f, 0x00);       //  no test display
    
    MAX7219_Clear();
}


void MAX7219_Shutdown(uint8_t on)
{
    // Table 3 : D0 = 0 -> shutdown, D0 = 1 -> fonctionnement normal
    MAX7219_WriteReg(MAX7219_REG_SHUTDOWN, on);
}

void MAX7219_SetIntensity(uint8_t intensity)
{
    if (intensity > 0x0F)
    {
        intensity = 0x0F;
    }
    MAX7219_WriteReg(MAX7219_REG_INTENSITY, intensity);
}

void MAX7219_Clear(void)
{
    uint8_t i;
    for (i = 0; i < MAX7219_NB_DIGITS; i++)
    {
        MAX7219_WriteReg(MAX7219_PosToReg(i), 0x00);
    }
}


/* ==========================================================
 *  Affichage caractere / chaine / nombre
 * ========================================================== */
void MAX7219_DisplayDigitChar(uint8_t position, char c, uint8_t dp)
{
    uint8_t seg;

    if (position >= MAX7219_NB_DIGITS)
    {
        return;
    }

    seg = MAX7219_CharToSeg(c);
    if (dp)
    {
        seg |= 0x80;    // bit DP (Table 6 : D7 = DP)
    }

    MAX7219_WriteReg(MAX7219_PosToReg(position), seg);

}

void MAX7219_DisplayString(const char *str)
{
    uint8_t position = 0;
    const char *p = str;

    while ((*p != '\0') && (position < MAX7219_NB_DIGITS))
    {
        char c = *p++;
        uint8_t dp = 0;

        // Un '.' juste apres un caractere allume le DP du meme digit
        // au lieu d'occuper une position supplementaire
        if (*p == '.')
        {
            dp = 1;
            p++;
        }

        MAX7219_DisplayDigitChar(position, c, dp);
        position++;
    }

    // Efface les digits restants si la chaine est plus courte que l'afficheur
    while (position < MAX7219_NB_DIGITS)
    {
        MAX7219_DisplayDigitChar(position, ' ', 0);
        position++;
    }
}

void MAX7219_DisplayInt(int32_t value)
{
    char buf[MAX7219_NB_DIGITS + 2];

    snprintf(buf, sizeof(buf), "%*ld", (int)MAX7219_NB_DIGITS, (long)value);
    MAX7219_DisplayString(buf);
}


/* ==========================================================
 *  Defilement de texte
 * ========================================================== */
void MAX7219_Scroll(const char *str, uint16_t delay_ms)
{
    char     padded[MAX7219_SCROLL_BUFFER_LEN];
    char     window[MAX7219_NB_DIGITS + 1];
    uint16_t len;
    uint16_t max_len;
    uint16_t total_len;
    uint16_t i, p;

    max_len = MAX7219_SCROLL_BUFFER_LEN - (2 * MAX7219_NB_DIGITS) - 1;

    len = (uint16_t)strlen(str);
    if (len > max_len)
    {
        len = max_len;
    }

    // Chaine paddee : N espaces (entree) + texte + N espaces (sortie)
    for (i = 0; i < MAX7219_NB_DIGITS; i++)
    {
        padded[i] = ' ';
    }
    for (i = 0; i < len; i++)
    {
        padded[MAX7219_NB_DIGITS + i] = str[i];
    }
    for (i = 0; i < MAX7219_NB_DIGITS; i++)
    {
        padded[MAX7219_NB_DIGITS + len + i] = ' ';
    }
    total_len = len + (2 * MAX7219_NB_DIGITS);
    padded[total_len] = '\0';

    // Fait glisser une fenetre de MAX7219_NB_DIGITS caracteres
    for (p = 0; p <= (total_len - MAX7219_NB_DIGITS); p++)
    {
        for (i = 0; i < MAX7219_NB_DIGITS; i++)
        {
            window[i] = padded[p + i];
        }
        window[MAX7219_NB_DIGITS] = '\0';

        MAX7219_DisplayString(window);
    }
}
