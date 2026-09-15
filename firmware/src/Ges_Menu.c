#include "Ges_Menu.h"
#include "Ges_Adc.h"
#include "Ges_Stepper.h"
#include "Mc32Max7219.h"
#include <stdbool.h>
#include <stdio.h>
#include "app.h"
#include "LM92.h"
#include <stdio.h>
#include "AD5620.h"
#include "Mc32gestI2cSeeprom.h"

MENU_STATES menu_state = MENU_DEFAULT;
SETTING_STATES setting_state = SET_DEFAULT;
extern APP_DATA appData;

extern STEPPER_DATA stepper_1_Data;
extern STEPPER_DATA stepper_2_Data;
extern SOUND_DATA Sound_Data;
SYS_INFO systeme_info;
bool do_this_once;
void Gestion_Menu(JOYSTICK_DATA *ptr_joystickData){
    static float current_time_s = 0;
    static uint8_t current_time_m = 0;
    char string_7seg[7];
    static bool blinking = false;
    
    
    switch (menu_state)
    {
        case MENU_CALIBRATION:
            // Calibration
            break;

        case MENU_DEFAULT:
            // Menu principal
            MAX7219_DisplayString("StArt");
            PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_C, PORTS_BIT_POS_3,true); 
            do_this_once = true;
            if( ptr_joystickData->Joystick_are_LR){
                ptr_joystickData->Joystick_are_LR = false;
                menu_state = MENU_SETTING;
                
                MAX7219_DisplayDigitChar(0,'S',0);
                MAX7219_DisplayDigitChar(1,' ',0);
                MAX7219_DisplayDigitChar(2,' ',0);
                MAX7219_DisplayDigitChar(3,' ',0);
                MAX7219_DisplayDigitChar(4,' ',0);
                MAX7219_DisplayDigitChar(5,' ',0);
                
                Sound_Data.frequency_note_1 = 20;
                Sound_Data.frequency_note_2 = 40;
                Sound_Data.frequency_note_3 = 60;
                Sound_Data.nb_note = 3;
                Sound_Data.sound_for_a_tick = true;

            }   
            
            if( ptr_joystickData->Joystick_are_RR){
                ptr_joystickData->Joystick_are_RR = false;
                menu_state = MENU_GAME_IN_PROGRESS;
                current_time_s = 0;
                current_time_m = 0;
                appData.in_game = true;
                Sound_Data.frequency_note_1 = 392;
                Sound_Data.frequency_note_2 = 440;
                Sound_Data.frequency_note_3 = 493.88;
                Sound_Data.frequency_note_4 = 523.25;
                Sound_Data.nb_note = 4;
                Sound_Data.sound_for_a_tick = true;
            }   
            
            break;

        case MENU_GAME_IN_PROGRESS:
            current_time_s = current_time_s + 0.2;
            if(current_time_s >= 60){
                current_time_m ++;
                current_time_s = 0;
            }
            sprintf(string_7seg,"t%02d=%02d",current_time_m,(int)current_time_s);
            
            MAX7219_DisplayString(string_7seg);
            
            if( ptr_joystickData->Joystick_are_LL){
                ptr_joystickData->Joystick_are_LL = false;
                menu_state = MENU_GAME_STOPPED;
                current_time_s = 0;
                current_time_m = 0;
                appData.in_game = false;
                Sound_Data.frequency_note_1 = 261.63;
                Sound_Data.frequency_note_2 = 246.94;
                Sound_Data.frequency_note_3 = 220;
                Sound_Data.frequency_note_4 = 196;
                Sound_Data.nb_note = 4;
                Sound_Data.sound_for_a_tick = true;
            }   
            
            
            PLIB_PORTS_PinWrite(PORTS_ID_0,PORT_CHANNEL_A,PORTS_BIT_POS_10,true);
            if(!Capteur_billeStateGet()){
                menu_state = MENU_GAME_LOST;
            }
            
            break;

        case MENU_GAME_WON:
            Sound_Data.frequency_note_1 = 523.25;
            Sound_Data.frequency_note_2 = 659.25;
            Sound_Data.frequency_note_3 = 783.99;
            Sound_Data.frequency_note_4 = 1046.5;
            Sound_Data.nb_note = 4;
            Sound_Data.sound_for_a_tick = true;
            systeme_info.last_time_score = (60*current_time_m)+current_time_s;
            if(systeme_info.last_time_score < systeme_info.best_time_score){
                systeme_info.best_time_score = systeme_info.last_time_score;
            }
            menu_state = MENU_END_GAME;
            break;

        case MENU_GAME_LOST:
            Sound_Data.frequency_note_1 = 65.406;
            Sound_Data.frequency_note_2 = 41.203;
            Sound_Data.nb_note = 2;
            Sound_Data.sound_for_a_tick = true;
            menu_state = MENU_END_GAME;
            break;        
        case MENU_END_GAME:
            appData.in_game = false;

            if(blinking){
                blinking = false;
                MAX7219_DisplayString("      ");
            }
            else if(!blinking){
                blinking = true;
                sprintf(string_7seg,"t%02d=%02d",current_time_m,(int)current_time_s);
                MAX7219_DisplayString(string_7seg);
            }

            
            if( ptr_joystickData->Joystick_are_LL){
                ptr_joystickData->Joystick_are_LL = false;
                APP_Move_To(&stepper_1_Data,1000);
                APP_Move_To(&stepper_2_Data,1000);
                menu_state = MENU_DEFAULT;
                systeme_info.nbr_game_session++;
                systeme_info.nbr_game_all_time++;
                I2C_WriteSEEPROM((uint32_t*)&systeme_info,MCP79411_EEPROM_BEG, sizeof(systeme_info));
                PLIB_PORTS_PinWrite(PORTS_ID_0,PORT_CHANNEL_A,PORTS_BIT_POS_7,true);
            }  
            
            break;

        case MENU_GAME_STOPPED:
            // Jeu arr�t�s
            APP_Stepper_Stop(&stepper_1_Data);
            APP_Stepper_Stop(&stepper_2_Data);
            APP_Move_To(&stepper_1_Data,2500);
            APP_Move_To(&stepper_2_Data,1000);
            menu_state = MENU_END_GAME;
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
uint8_t value;




void Gestion_Menu_Setting(JOYSTICK_DATA *ptr_joystickData){
    char string_7seg[7];
    if( ptr_joystickData->Joystick_are_RR && modification_state == MOD_DEFAULT ){
        ptr_joystickData->Joystick_are_RR = false;
        
        Sound_Data.frequency_note_1 = 150;
        Sound_Data.nb_note = 1;
        Sound_Data.sound_for_a_tick = true;
        
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
        
        Sound_Data.frequency_note_1 = 50;
        Sound_Data.nb_note = 1;
        Sound_Data.sound_for_a_tick = true;
        
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
        if(modification_state == MOD_DEFAULT){
            menu_state = MENU_DEFAULT;
            I2C_WriteSEEPROM((uint32_t*)&systeme_info,MCP79411_EEPROM_BEG, sizeof(systeme_info));
        }
        else{
            modification_state = MOD_DEFAULT;
        }
    }  
    
    if( ptr_joystickData->Joystick_are_RL){
        ptr_joystickData->Joystick_are_RL = false;
        modification_state = MOD_ENTERING_SETTING;
    }  
    
    switch (setting_state)
    {
        case SET_DEFAULT:
            MAX7219_DisplayDigitChar(2,'d',0);
            break;

        case SET_BRIGHTNESS:
            switch(modification_state){
                case MOD_DEFAULT : 
                    MAX7219_DisplayDigitChar(0,' ',0);
                    MAX7219_DisplayDigitChar(1,'b',0);
                    MAX7219_DisplayDigitChar(2,'r',0);
                    MAX7219_DisplayDigitChar(3,' ',0);
                    MAX7219_DisplayDigitChar(4,' ',0);
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
            MAX7219_DisplayDigitChar(3,' ',0);
            break;

        case SEE_TIME:
            MAX7219_DisplayDigitChar(2,'t',0);
            MAX7219_DisplayDigitChar(3,' ',0);
            break;

        case SEE_NBR_PLAYED_GAME_SESSION:
            switch(modification_state){
                case MOD_DEFAULT : 
                    MAX7219_DisplayDigitChar(0,' ',0);
                    MAX7219_DisplayDigitChar(1,'S',0);
                    MAX7219_DisplayDigitChar(2,'G',0);
                    MAX7219_DisplayDigitChar(3,'S',0);
                    MAX7219_DisplayDigitChar(4,' ',0);
                    MAX7219_DisplayDigitChar(5,' ',0);
                    break;
                case MOD_ENTERING_SETTING :
                    sprintf(string_7seg,"%06d", systeme_info.nbr_game_session);
                    MAX7219_DisplayString(string_7seg);
                    break;
                case MOD_EXITING_SETTING : 
                    break;
            }
            break;

        case SEE_NBR_PLAYED_GAME_ALL_TIME:
            switch(modification_state){
                case MOD_DEFAULT : 
                    MAX7219_DisplayDigitChar(0,' ',0);
                    MAX7219_DisplayDigitChar(1,'S',0);
                    MAX7219_DisplayDigitChar(2,'G',0);
                    MAX7219_DisplayDigitChar(3,'A',0);
                    MAX7219_DisplayDigitChar(4,' ',0);
                    MAX7219_DisplayDigitChar(5,' ',0);
                    break;
                case MOD_ENTERING_SETTING :
                    sprintf(string_7seg,"%06d", systeme_info.nbr_game_all_time);
                    MAX7219_DisplayString(string_7seg);
                    break;
                case MOD_EXITING_SETTING : 
                    break;
            }
            break;

        case SEE_SYS_TEMP:
            switch(modification_state){
                case MOD_DEFAULT : 
                    MAX7219_DisplayDigitChar(0,' ',0);
                    MAX7219_DisplayDigitChar(1,' ',0);
                    MAX7219_DisplayDigitChar(2,'t',0);
                    MAX7219_DisplayDigitChar(3,'o',0);
                    MAX7219_DisplayDigitChar(4,' ',0);
                    MAX7219_DisplayDigitChar(5,' ',0);
                    break;
                case MOD_ENTERING_SETTING :
                    sprintf(string_7seg,"  %03.2f", LM92_GetTemperature());
                    MAX7219_DisplayString(string_7seg);
                    break;
                case MOD_EXITING_SETTING : 
                    break;
            }
            break;
        case SEE_ALL_TIME_BEST_SCORE:
            switch(modification_state){
                case MOD_DEFAULT : 
                    MAX7219_DisplayDigitChar(0,' ',0);
                    MAX7219_DisplayDigitChar(1,'S',0);
                    MAX7219_DisplayDigitChar(2,'b',0);
                    MAX7219_DisplayDigitChar(3,'s',0);
                    MAX7219_DisplayDigitChar(4,'c',0);
                    MAX7219_DisplayDigitChar(5,' ',0);
                    break;
                case MOD_ENTERING_SETTING :
                    sprintf(string_7seg,"%03d=%02d",(int)systeme_info.best_time_score/60,(int)systeme_info.best_time_score%60);
                    MAX7219_DisplayString(string_7seg);
                    break;
                case MOD_EXITING_SETTING : 
                    break;
            }
            break;
        case SEE_LAST_SCORE:
            switch(modification_state){
                case MOD_DEFAULT : 
                    MAX7219_DisplayDigitChar(0,' ',0);
                    MAX7219_DisplayDigitChar(1,'S',0);
                    MAX7219_DisplayDigitChar(2,'L',0);
                    MAX7219_DisplayDigitChar(3,'s',0);
                    MAX7219_DisplayDigitChar(4,'c',0);
                    MAX7219_DisplayDigitChar(5,' ',0);
                    break;
                case MOD_ENTERING_SETTING :
                    sprintf(string_7seg,"%03d=%02d",(int)systeme_info.last_time_score/60,(int)systeme_info.last_time_score%60);
                    MAX7219_DisplayString(string_7seg);
                    break;
                case MOD_EXITING_SETTING : 
                    break;
            }
            break;

        case SYS_RECALIBRATION:
            MAX7219_DisplayDigitChar(0,' ',0);
            MAX7219_DisplayDigitChar(1,' ',0);
            MAX7219_DisplayDigitChar(2,'C',0);
            MAX7219_DisplayDigitChar(3,' ',0);
            MAX7219_DisplayDigitChar(4,' ',0);
            MAX7219_DisplayDigitChar(5,' ',0);
            break;

        default:
            setting_state = SET_DEFAULT;
            break;
    }
}