#ifndef Mc32Max7219_H
#define Mc32Max7219_H
/*--------------------------------------------------------*/
// Mc32Max7219.h
/*--------------------------------------------------------*/
//  Description :  Utilitaire de pilotage MAX7219/MAX7221
//                  Affichage 6 digits 7-segments, mode
//                  "no-decode" (chiffres + lettres), avec
//                  fonction de defilement (scroll) de texte.
//                  S'appuie sur Mc32SpiUtil.c/.h fourni.
//
//  Compilateur :   XC32 + Harmony
//  Version     :   V1.0
/*--------------------------------------------------------*/

#include <stdint.h>
#include "Ges_Menu.h"

/* ==========================================================
 *  CONFIGURATION MATERIELLE - A ADAPTER AU PROJET
 * ========================================================== */

// Canal SPI utilise pour dialoguer avec le MAX7219 (1 ou 2)
// -> correspond a spi_write1() ou spi_write2() de Mc32SpiUtil.c
#ifndef MAX7219_SPI_CHANNEL
#define MAX7219_SPI_CHANNEL     2
#endif

// Broche LOAD (MAX7219) / CS (MAX7221), generee par le MHC (Harmony)
// Remplacer "LOAD_CS_W" par le nom reel de la broche du projet
// (meme convention que LED3_W utilise dans Mc32SpiUtil.c)
#ifndef MAX7219_CS_W
#define MAX7219_CS_W             LOAD_CS_W   /* <-- A ADAPTER */
#endif

// Nombre de digits physiquement cables (1 a 8)
#ifndef MAX7219_NB_DIGITS
#define MAX7219_NB_DIGITS       6
#endif

// Longueur max d'un message pour la fonction de defilement
#ifndef MAX7219_SCROLL_BUFFER_LEN
#define MAX7219_SCROLL_BUFFER_LEN   64
#endif

/* ==========================================================
 *  Registres MAX7219 / MAX7221 (datasheet Table 2)
 * ========================================================== */
#define MAX7219_REG_NOOP         0x00
#define MAX7219_REG_DIGIT0       0x01
#define MAX7219_REG_DIGIT1       0x02
#define MAX7219_REG_DIGIT2       0x03
#define MAX7219_REG_DIGIT3       0x04
#define MAX7219_REG_DIGIT4       0x05
#define MAX7219_REG_DIGIT5       0x06
#define MAX7219_REG_DIGIT6       0x07
#define MAX7219_REG_DIGIT7       0x08
#define MAX7219_REG_DECODEMODE   0x09
#define MAX7219_REG_INTENSITY    0x0A
#define MAX7219_REG_SCANLIMIT    0x0B
#define MAX7219_REG_SHUTDOWN     0x0C
#define MAX7219_REG_DISPTEST     0x0F

/* ==========================================================
 *  Prototypes
 * ========================================================== */

// Initialise le MAX7219 (mode no-decode, scan-limit, intensite,
// efface l'ecran, sort du shutdown)
void MAX7219_Init(SYS_INFO *ptr_systeme_info);

// Ecrit un octet "data" dans le registre "reg" (gere LOAD/CS + SPI)
void MAX7219_WriteReg(uint8_t reg, uint8_t data);

// Met le composant en veille (1) ou en fonctionnement normal (0)
void MAX7219_Shutdown(uint8_t on);

// Regle la luminosite (0x00 a 0x0F)
void MAX7219_SetIntensity(uint8_t intensity);

// Eteint tous les digits
void MAX7219_Clear(void);

// Affiche un caractere decode sur un digit donne (position 0 = a gauche)
// dp = 1 pour allumer le point decimal de ce digit
void MAX7219_DisplayDigitChar(uint8_t position, char c, uint8_t dp);

// Decode une chaine et l'affiche directement (un '.' apres un caractere
// allume le point decimal du digit precedent au lieu de prendre une position)
void MAX7219_DisplayString(const char *str);

// Affiche un nombre entier, cadre a droite
void MAX7219_DisplayInt(int32_t value);

// Fait defiler un texte (plus long que l'affichage) de droite a gauche
// delay_ms = temps d'attente entre chaque pas de defilement
void MAX7219_Scroll(const char *str, uint16_t delay_ms);

uint8_t MAX7219_CharToSeg(char c);
#endif
