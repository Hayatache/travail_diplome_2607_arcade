#include "Ges_Menu.h"
#include "Ges_Adc.h"
#include "Mc32Max7219.h"
#include <stdbool.h>
#include "app.h"

MENU_STATES menu_state = MENU_DEFAULT;
SETTING_STATES setting_state = SET_DEFAULT;

void Gestion_Menu(JOYSTICK_DATA *ptr_joystickData){
    
    switch (menu_state)
    {
        case MENU_CALIBRATION:
            // Calibration
            break;

        case MENU_DEFAULT:
            // Menu principal
            PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_C, PORTS_BIT_POS_3,true);  
            if( ptr_joystickData->Joystick_are_LR){
                ptr_joystickData->Joystick_are_LR = false;
                PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_C, PORTS_BIT_POS_3,false);  
                PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_B, PORTS_BIT_POS_5,true); 
                menu_state = MENU_SETTING;
                MAX7219_DisplayDigitChar(0,'S',0);
            }   
            
            break;

        case MENU_GAME_IN_PROGRESS:
            // Jeu en cours
            break;

        case MENU_GAME_WON:
            // Victoire
            break;

        case MENU_GAME_LOST:
            // Défaite
            break;

        case MENU_GAME_STOPPED:
            // Jeu arrêté
            break;

        case MENU_SETTING:
            Gestion_Menu_Setting(ptr_joystickData);
            break;

        default:
            //cas invalide
            menu_state = MENU_DEFAULT;
            break;
    }   
    
}


static MODIFICATION_STATES modification_state = MOD_DEFAULT;
bool menu_plus = false;
bool menu_minus = false;
SYS_INFO systeme_info;
uint8_t value;
void Gestion_Menu_Setting(JOYSTICK_DATA *ptr_joystickData){
    if( ptr_joystickData->Joystick_are_RR && modification_state == MOD_DEFAULT ){
        ptr_joystickData->Joystick_are_RR = false;
        if(setting_state == SYS_RECALIBRATION){
            setting_state = SET_DEFAULT;
        }
        else{
            setting_state ++;
        }
    }  
    else if( ptr_joystickData->Joystick_are_RR && modification_state == MOD_ENTERING_SETTING  ){
        ptr_joystickData->Joystick_are_RR = false;
        menu_plus = true;
    }  
    
    if( ptr_joystickData->Joystick_are_LL && modification_state == MOD_DEFAULT ){
        ptr_joystickData->Joystick_are_LL = false;
        if(setting_state == SET_DEFAULT){
            setting_state = SYS_RECALIBRATION;
        }
        else{
            setting_state --;
        }
    }  
    else if( ptr_joystickData->Joystick_are_LL && modification_state == MOD_ENTERING_SETTING ){
        ptr_joystickData->Joystick_are_LL = false;
        menu_minus = true;
    }  
    
    if( ptr_joystickData->Joystick_are_LR){
        ptr_joystickData->Joystick_are_LR = false;
        modification_state = MOD_DEFAULT;
    }  
    
    if( ptr_joystickData->Joystick_are_RL){
        ptr_joystickData->Joystick_are_RL = false;
        modification_state = MOD_ENTERING_SETTING;
        PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_A, PORTS_BIT_POS_10,true);  
        PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_A, PORTS_BIT_POS_7,true); 
    }  
    
    switch (setting_state)
    {
        case SET_DEFAULT:
            MAX7219_DisplayDigitChar(2,'d',0);
            break;

        case SET_BRIGHTNESS:
            switch(modification_state){
                case MOD_DEFAULT : 
                    MAX7219_DisplayDigitChar(2,'b',0);
                    MAX7219_DisplayDigitChar(5,' ',0);
                    break;
                case MOD_ENTERING_SETTING :
                    value = systeme_info.sys_brightness & 0x0F;

                    MAX7219_DisplayDigitChar(5,
                         (value < 10) ? ('0' + value) : ('A' + value - 10),
                         1);
                    MAX7219_DisplayDigitChar(2,'b',1);
                    
                    if(menu_minus){
                        menu_minus = false;
                        if(systeme_info.sys_brightness <= MIN_BRIGHTNESS){
                            systeme_info.sys_brightness = MIN_BRIGHTNESS;
                        }
                        else {
                            systeme_info.sys_brightness --;
                        }
                    }
                    
                    if(menu_plus){
                        menu_plus = false;
                        if(systeme_info.sys_brightness >= MAX_BRIGHTNESS){
                            systeme_info.sys_brightness = MAX_BRIGHTNESS;
                        }
                        else {
                            systeme_info.sys_brightness ++;
                        }
                    }
                    MAX7219_SetIntensity(systeme_info.sys_brightness);
                    break;
                case MOD_EXITING_SETTING : 
                    break;
                    
            }
            break;
        case SET_TIME:
            MAX7219_DisplayDigitChar(2,'t',1);
            break;

        case SEE_TIME:
            MAX7219_DisplayDigitChar(2,'t',0);
            MAX7219_DisplayDigitChar(3,' ',0);
            break;

        case SEE_NBR_PLAYED_GAME_SESSION:
            MAX7219_DisplayDigitChar(2,'G',0);
            MAX7219_DisplayDigitChar(3,'S',0);
            break;

        case SEE_NBR_PLAYED_GAME_ALL_TIME:
            MAX7219_DisplayDigitChar(2,'G',0);
            MAX7219_DisplayDigitChar(3,'A',0);
            break;

        case SEE_SYS_TEMP:
            
            MAX7219_DisplayDigitChar(2,'t',0);
            MAX7219_DisplayDigitChar(3,'o',0);
            MAX7219_DisplayDigitChar(4,' ',0);
            break;
        case SEE_ALL_TIME_BEST_SCORE:
            
            MAX7219_DisplayDigitChar(2,'b',0);
            MAX7219_DisplayDigitChar(3,'s',0);
            MAX7219_DisplayDigitChar(4,'c',0);
            break;
        case SEE_LAST_SCORE:
            MAX7219_DisplayDigitChar(2,'l',0);
            MAX7219_DisplayDigitChar(3,'s',0);
            MAX7219_DisplayDigitChar(4,'c',0);
            break;

        case SYS_RECALIBRATION:
            MAX7219_DisplayDigitChar(2,'C',0);
            MAX7219_DisplayDigitChar(3,' ',0);
            MAX7219_DisplayDigitChar(4,' ',0);
            break;

        default:
            setting_state = SET_DEFAULT;
            break;
    }
}