//--------------------------------------------------------
// Mc32gestI2cEEprom.C
//--------------------------------------------------------
// Gestion I2C de la SEEPROM du MCP79411 (Solution exercice)
//	Description :	Fonctions pour EEPROM MCP79411
//
//	Auteur 		: 	C. HUBER
//      Date            :       26.05.2014
//	Version		:	V1.0
//	Compilateur	:	XC32 V1.31
// Modifications :
//
/*--------------------------------------------------------*/



#include "Mc32gestI2cSeeprom.h"
#include "Mc32_I2cUtilCCS.h"
#include "Ges_Menu.h"



// Definitions du bus (pour mesures)
// #define I2C-SCK  SCL2/RA2      PORTAbits.RA2   pin 58
// #define I2C-SDA  SDa2/RA3      PORTAbits.RA3   pin 59




// Initialisation de la communication I2C et du MCP79411
// ------------------------------------------------

void I2C_InitMCP79411(void)
{
   bool Fast = true;
   i2c_init( Fast );
   
    i2c_start();

    i2c_write(MCP79411_RTCC_W);
    i2c_write(0x00);
    i2c_write(0x80);

    i2c_stop();
} //end I2C_InitMCP79411
// Ecriture d'un bloc de donnees dans la SEEPROM du MCP79411
// ---------------------------------------------------------

void I2C_WriteSEEPROM(void *SrcData, uint32_t EEpromAddr, uint16_t NbBytes)
{
    uint8_t pageIndex = 0;                 // Index des pages EEPROM
    uint8_t byteIndex = 0;                 // Index des bytes dans une page
    uint8_t *writeBuffer = SrcData;        // Pointeur sur les donnees a ecrire
    uint8_t bytesToWriteInPage = 0;        // Nombre de bytes a ecrire dans la page courante

    // Parcours de toutes les pages necessaires a l'ecriture
    for(pageIndex = 0; pageIndex <= (NbBytes / EEPROM_PAGE_SIZE); pageIndex++)
    {
        // Verification si on est sur la derniere page
        if(pageIndex == (NbBytes / EEPROM_PAGE_SIZE))
        {
            // Nombre restant de bytes a ecrire
            bytesToWriteInPage = NbBytes - EEPROM_PAGE_SIZE * pageIndex;
        }
        else
        {
            // Page complete
            bytesToWriteInPage = EEPROM_PAGE_SIZE;
        }

        // Attente de disponibilite du composant EEPROM (ACK)
        do
        {
            i2c_start();

        } while(!i2c_write(MCP79411_EEPROM_W));

        // Envoi de l'adresse de depart dans l'EEPROM
        i2c_write((uint8_t)EEpromAddr + (pageIndex * EEPROM_PAGE_SIZE));

        // Ecriture des donnees de la page
        for(byteIndex = 0; byteIndex < bytesToWriteInPage; byteIndex++)
        {
           i2c_write(writeBuffer[byteIndex + (pageIndex * EEPROM_PAGE_SIZE)]);
        }

        // Fin de transmission de la page
        i2c_stop();
    }

} // end I2C_WriteSEEPROM




// Lecture d'un bloc de donnees depuis la SEEPROM du MCP79411
// ----------------------------------------------------------

void I2C_ReadSEEPROM(void *DstData, uint32_t EEpromAddr, uint16_t NbBytes)
{
    uint8_t byteIndex = 0;
    uint8_t *readBuffer = DstData;

    // Attente de disponibilite du composant EEPROM (ACK)
    do
    {
        i2c_start();

    } while(!i2c_write(MCP79411_EEPROM_W));

    // Envoi de l'adresse de lecture
    i2c_write((uint8_t)EEpromAddr);

    // Redemarrage du bus I2C pour passer en mode lecture
    i2c_reStart();

    // Envoi de l'adresse du composant en lecture
    i2c_write(MCP79411_EEPROM_R);

    // Lecture des bytes avec ACK
    for(byteIndex = 0; byteIndex < NbBytes - 1; byteIndex++)
    {
        readBuffer[byteIndex] = i2c_read(1);
    }

    // Dernier byte lu sans ACK
    readBuffer[byteIndex] = i2c_read(0);

    // Arret de la communication I2C
    i2c_stop();

} // end I2C_ReadSEEPROM

SYS_INFO systeme_info_from_memory;

void Check_If_Memory_exist(SYS_INFO *ptr_systeme_info){
    /*lis la memoire de l'eeprom et la met dans une structure tempons*/
    I2C_ReadSEEPROM((uint32_t*)&systeme_info_from_memory,MCP79411_EEPROM_BEG, sizeof(systeme_info_from_memory));
    /* dans le cas ou la variable magic est dans la memoire, cela veut dire qu'une information a été sauvegarder */
    if(systeme_info_from_memory.Magic == MAGIC){
        /* recuperation de toutes les informations */
        *ptr_systeme_info = systeme_info_from_memory;
        /* reset de variable relative a la session */
        ptr_systeme_info->nbr_game_session = 0;
        ptr_systeme_info->last_time_score = 0;
    }
    else{
        /*si la valeur n'as pas été sauvegarder, met des valeurs de base dans toutes les variables de la structure*/
        ptr_systeme_info->Magic = MAGIC;
        ptr_systeme_info->best_time_score = 0;
        ptr_systeme_info->last_time_score = 0;
        ptr_systeme_info->nbr_game_all_time = 0;
        ptr_systeme_info->nbr_game_session = 0;
        ptr_systeme_info->sys_brightness = 3;
    }
}
 
