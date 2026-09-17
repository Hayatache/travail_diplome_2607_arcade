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


//----------------------------------------------------------------------------------//
//-- Table de correspondance entre la position d'affichage et le registre du MAX7219
//-- position 0                  : digit le plus à gauche
//-- position MAX7219_NB_DIGITS-1 : digit le plus à droite
//----------------------------------------------------------------------------------//
static const uint8_t MAX7219_DigitRegTable[8] =
{
    MAX7219_REG_DIGIT0, MAX7219_REG_DIGIT1, MAX7219_REG_DIGIT2,
    MAX7219_REG_DIGIT3, MAX7219_REG_DIGIT4, MAX7219_REG_DIGIT5,
    MAX7219_REG_DIGIT6, MAX7219_REG_DIGIT7
};

//----------------------------------------------------------------------------------//
//-- nom fct : MAX7219_PosToReg
//-- paramètre entrée : uint8_t position
//-- paramètre sortie : uint8_t registre
//-- description : convertit la position d'affichage en registre du MAX7219
//----------------------------------------------------------------------------------//
static uint8_t MAX7219_PosToReg(uint8_t position)
{
    return MAX7219_DigitRegTable[(MAX7219_NB_DIGITS - 1) - position];
}


//----------------------------------------------------------------------------------//
//-- Table de correspondance des caractères avec les segments du digit
//-- Format : DP A B C D E F G
//-- Utilisée lorsque le MAX7219 est configuré en mode "no-decode"
//----------------------------------------------------------------------------------//
typedef struct
{
    char    car;
    uint8_t seg;
} MAX7219_Font_t;

//----------------------------------------------------------------------------------//
//-- Table des caractères disponibles sur l'afficheur 7 segments
//-- Les caractères ne pouvant pas être représentés exactement sont approximés
//----------------------------------------------------------------------------------//
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


//----------------------------------------------------------------------------------//
//-- nom fct : MAX7219_CharToSeg
//-- paramètre entrée : char c
//-- paramètre sortie : uint8_t segments
//-- description : convertit un caractère en code correspondant aux segments
//----------------------------------------------------------------------------------//
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


//----------------------------------------------------------------------------------//
//-- nom fct : MAX7219_WriteReg
//-- paramètre entrée : uint8_t reg, uint8_t data
//-- paramètre sortie : aucune
//-- description : écrit une donnée dans un registre du MAX7219 via SPI
//----------------------------------------------------------------------------------//
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


//----------------------------------------------------------------------------------//
//-- nom fct : MAX7219_Init
//-- paramètre entrée : SYS_INFO *ptr_systeme_info
//-- paramètre sortie : aucune
//-- description : initialise le MAX7219 avec les paramètres du système
//----------------------------------------------------------------------------------//
void MAX7219_Init(SYS_INFO *ptr_systeme_info)
{

	MAX7219_WriteReg(0x0c, 0x01);       //  power down =0?normal mode = 1
    MAX7219_WriteReg(0x09, 0x00);       //  no decoding
	MAX7219_WriteReg(0x0a, ptr_systeme_info->sys_brightness);       //  brightness intensity
	MAX7219_WriteReg(0x0b, 0x05);       //  scan limit = 8 LEDs
	MAX7219_WriteReg(0x0f, 0x00);       //  no test display
    
    MAX7219_Clear();
}

//----------------------------------------------------------------------------------//
//-- nom fct : MAX7219_Init
//-- paramètre entrée : SYS_INFO *ptr_systeme_info
//-- paramètre sortie : aucune
//-- description : initialise le MAX7219 avec les paramètres du système
//----------------------------------------------------------------------------------//
void MAX7219_Shutdown(uint8_t on)
{
    // Table 3 : D0 = 0 -> shutdown, D0 = 1 -> fonctionnement normal
    MAX7219_WriteReg(MAX7219_REG_SHUTDOWN, on);
}

//----------------------------------------------------------------------------------//
//-- nom fct : MAX7219_SetIntensity
//-- paramètre entrée : uint8_t intensity
//-- paramètre sortie : aucune
//-- description : règle la luminosité de l'afficheur
//----------------------------------------------------------------------------------//
void MAX7219_SetIntensity(uint8_t intensity)
{
    if (intensity > 0x0F)
    {
        intensity = 0x0F;
    }
    MAX7219_WriteReg(MAX7219_REG_INTENSITY, intensity);
}

//----------------------------------------------------------------------------------//
//-- nom fct : MAX7219_Clear
//-- paramètre entrée : aucun
//-- paramètre sortie : aucune
//-- description : efface tous les digits de l'afficheur
//----------------------------------------------------------------------------------//
void MAX7219_Clear(void)
{
    uint8_t i;
    for (i = 0; i < MAX7219_NB_DIGITS; i++)
    {
        MAX7219_WriteReg(MAX7219_PosToReg(i), 0x00);
    }
}


//----------------------------------------------------------------------------------//
//-- nom fct : MAX7219_DisplayDigitChar
//-- paramètre entrée : uint8_t position, char c, uint8_t dp
//-- paramètre sortie : aucune
//-- description : affiche un caractère à une position donnée
//----------------------------------------------------------------------------------//
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

//----------------------------------------------------------------------------------//
//-- nom fct : MAX7219_DisplayString
//-- paramètre entrée : const char *str
//-- paramètre sortie : aucune
//-- description : affiche une chaîne de caractères sur l'afficheur
//----------------------------------------------------------------------------------//
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


