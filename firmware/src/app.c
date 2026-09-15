/*******************************************************************************
  MPLAB Harmony Application Source File
  
  Company:
    Microchip Technology Inc.
  
  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It 
    implements the logic of the application's state machine and it may call 
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2013-2014 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 *******************************************************************************/
// DOM-IGNORE-END


// *****************************************************************************
// *****************************************************************************
// Section: Included Files 
// *****************************************************************************
// *****************************************************************************

#include "app.h"
#include "Mc32SpiUtil.h"
#include "Mc32Max7219.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "Ges_Menu.h"
#include "Mc32DriverAdc.h"
#include "Ges_Adc.h"
#include "Ges_Stepper.h"
#include "Mc32Delays.h"
#include "Mc32CoreTimer.h"
#include "Mc32gestI2cSeeprom.h"
#include "system_config.h"
#include "AD5620.h"
// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.
    
    Application strings and buffers are be defined outside this structure.
*/

APP_DATA appData;
SOUND_DATA Sound_Data;
static S_ADCResults adcRes ;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary callback functions.
*/

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


/* TODO:  Add any necessary local functions.
*/


// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;

    
    /* TODO: Initialize your application's state machine and other
     * parameters.
     */
}

uint32_t ctr_calibration= 0;

static void Stepper_RunCalibration(STEPPER_DATA *stepperData, bool btmPressed, bool topPressed)
{
    switch(stepperData->calib_state)
    {
        case STEPPER_CALIB_FIND_BTM:
        {
            if(btmPressed)
            {
                APP_Stepper_Stop(stepperData);
                stepperData->Position_X   = 0;
                stepperData->Position_FC1 = 0;
                stepperData->calib_state  = STEPPER_CALIB_FIND_TOP;
            }
            else if(stepperData->state == STEPPER_STBY)
            {

                APP_Move_Calibration(stepperData, STEPPER_DIR_DOWN, STEPPER_CALIB_SEARCH_STEPS);

            }
            break;
        }

        case STEPPER_CALIB_FIND_TOP:
        {
            if(topPressed)
            {
                APP_Stepper_Stop(stepperData);
                stepperData->Position_FC2 = stepperData->Position_X;
                stepperData->calib_state  = STEPPER_CALIB_DONE;
            }
            else if(stepperData->state == STEPPER_STBY)
            {

                APP_Move_Calibration(stepperData, STEPPER_DIR_UP, STEPPER_CALIB_SEARCH_STEPS);

            }
            break;
        }

        case STEPPER_CALIB_DONE:
            
        default:
            break;
    }
}

/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */
int32_t cpt = 0;
int32_t cpt_aff = 0;
char string[6];
int bruh = 0;
int bruh2 = 0;
int jesus = 0;
int jesus1= 0;

JOYSTICK_DATA Joystick_Data;
STEPPER_DATA stepper_1_Data;
STEPPER_DATA stepper_2_Data;

bool calibration_ADC_status = false;
bool calibration_Moteur_status = false;
bool calibration_status_test = false;

bool do_this_once = false;
int calibration_status = 2;
int cpt_sound= 100;


extern SYS_INFO systeme_info;



void APP_Tasks ( void )
{
    
    static bool game_won = false;
    static uint8_t cpt_start_game = 0;
    static uint8_t cpt_reset_game = 0;

        
    
    /* Surveillance des fins de course a chaque passage de boucle (pas
       seulement au tick ~210 ms) pour arreter le moteur au plus vite. */
    
    if(!calibration_Moteur_status)
    {
        bool btm2 = !Fin_de_course_1_BTM_StateGet();
        bool top2 = !Fin_de_course_1_TOP_StateGet();
        bool btm1 = !Fin_de_course_2_BTM_StateGet();
        bool top1 = !Fin_de_course_2_TOP_StateGet(); //a reinvers� 
        Stepper_RunCalibration(&stepper_1_Data, btm1, top1);
        Stepper_RunCalibration(&stepper_2_Data, btm2, top2);


        calibration_Moteur_status = (stepper_1_Data.calib_state == STEPPER_CALIB_DONE) &&
                                     (stepper_2_Data.calib_state == STEPPER_CALIB_DONE);
    }

    if(calibration_status_test && !do_this_once){
        do_this_once = true;        
        Sound_Data.frequency_note_1 = 523.251;
        Sound_Data.frequency_note_2 = 783.991;
        Sound_Data.frequency_note_3 = 932.327;
        Sound_Data.nb_note = 3;
        Sound_Data.sound_for_a_tick = true;
        APP_Move_To(&stepper_1_Data,1000);
        APP_Move_To(&stepper_2_Data,1000);

    }
    calibration_status_test = calibration_ADC_status && calibration_Moteur_status;
    /* Check the application's current state. */
    switch ( appData.state )
    {
        /* Application's initial state. */
        case APP_STATE_INIT:
        {
            

            bool appInitialized = true;

            
            
            BSP_InitADC10(); //Init AD
            
            
            
            DRV_TMR0_Start();
            DRV_TMR1_Start();
            DRV_TMR2_Start();
            DRV_OC0_Enable();
            DRV_OC1_Enable();
            
            I2C_InitMCP79411();
            Check_If_Memory_exist(&systeme_info);
            MAX7219_Init(&systeme_info);
            
            stepper_1_Data.Position_X = 8000;
            stepper_1_Data.Position_FC1 = 0;
            stepper_1_Data.Position_FC2 = 16000;
            stepper_1_Data.Stepper_ID = 0;
            stepper_2_Data.Position_X = 8000;
            stepper_2_Data.Position_FC1 = 0;
            stepper_2_Data.Position_FC2 = 16000;
            stepper_2_Data.Stepper_ID = 1;
            
            stepper_1_Data.state = STEPPER_STBY;
            stepper_2_Data.state = STEPPER_STBY;

            stepper_1_Data.calib_state = STEPPER_CALIB_FIND_BTM;
            stepper_2_Data.calib_state = STEPPER_CALIB_FIND_BTM;

            stepper_1_Data.Request_to_move = false;
            stepper_2_Data.Request_to_move = false;

            stepper_1_Data.nb_step_left = 0;
            stepper_2_Data.nb_step_left = 0;
            
            appData.state = APP_STATE_SERVICE_WAIT;
            appData.in_game = false;
            MAX7219_DisplayString("cALiBr");
            PLIB_PORTS_PinWrite(PORTS_ID_0,PORT_CHANNEL_C,PORTS_BIT_POS_4, true);
            
            break;
        }

        case APP_STATE_SERVICE_TASKS:
        {
            
            systeme_info.sys_temp.LM92_Temp = LM92_GetTemperature();
            systeme_info.best_time_score = 59999;
            Gestion_Menu(&Joystick_Data);
            appData.state = APP_STATE_SERVICE_WAIT;
            break;
        }        

        
        case APP_STATE_SERVICE_CALIBRATION:
        {
            static uint32_t moyenne_adc_joystick1_X = 0;
            static uint32_t moyenne_adc_joystick1_Y = 0;
            static uint32_t moyenne_adc_joystick2_X = 0;
            static uint32_t moyenne_adc_joystick2_Y = 0;
            static uint8_t compteur_moyenne_adc = 0;
            adcRes = BSP_ReadAllADC();
            
            if(compteur_moyenne_adc >= 20 && !calibration_ADC_status){
                calibration_ADC_status = true;
                Joystick_Data.Joystick_1_X_Mid_Value = (moyenne_adc_joystick1_X / 20);
                Joystick_Data.Joystick_1_Y_Mid_Value = (moyenne_adc_joystick1_Y / 20);
                Joystick_Data.Joystick_2_X_Mid_Value = (moyenne_adc_joystick2_X / 20);
                Joystick_Data.Joystick_2_Y_Mid_Value = (moyenne_adc_joystick2_Y / 20);
            }
            else
            {
                compteur_moyenne_adc++;

                moyenne_adc_joystick1_X += adcRes.Joystick_1_X;
                moyenne_adc_joystick1_Y += adcRes.Joystick_1_Y;
                moyenne_adc_joystick2_X += adcRes.Joystick_2_X;
                moyenne_adc_joystick2_Y += adcRes.Joystick_2_Y;
            }
            
            
            appData.state = APP_STATE_SERVICE_WAIT;
            break;
        }
        case APP_STATE_SERVICE_READ_ADC:
        {
            adcRes = BSP_ReadAllADC();
            ADC_TO_SPEED(&stepper_1_Data,adcRes.Joystick_1_X , &Joystick_Data.Joystick_1_X_Mid_Value);
            ADC_TO_SPEED(&stepper_2_Data,adcRes.Joystick_2_X , &Joystick_Data.Joystick_2_X_Mid_Value);
            Scan_X_AXIS(&Joystick_Data,adcRes.Joystick_1_Y,Joystick_Data.Joystick_1_Y_Mid_Value,adcRes.Joystick_2_Y,Joystick_Data.Joystick_2_Y_Mid_Value);
            
            appData.state = APP_STATE_SERVICE_WAIT;
            break;
        }        
        
        case APP_STATE_SERVICE_JOYSTICK_X_ACTION:
        {
            static uint8_t compteur_vitesse_stepper_1 = 0;
            static uint8_t compteur_vitesse_stepper_2 = 0;
            if(appData.in_game){

                if(stepper_1_Data.Stepper_Direction == 0){
                    APP_Move_Up(&stepper_1_Data,stepper_1_Data.speed);
                }
                else if (stepper_1_Data.Stepper_Direction == 1){
                    APP_Move_Down(&stepper_1_Data,stepper_1_Data.speed);
                }

                if(stepper_2_Data.Stepper_Direction == 0){
                    APP_Move_Up(&stepper_2_Data,stepper_2_Data.speed);
                }
                else if (stepper_2_Data.Stepper_Direction == 1){
                    APP_Move_Down(&stepper_2_Data,stepper_2_Data.speed);
                }
            }

            appData.state = APP_STATE_SERVICE_WAIT;
            break;
        }

        case APP_STATE_SERVICE_WAIT:
        {
            break;
        }
        /* TODO: implement your application state machine.*/
        

        /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}

 

/*******************************************************************************
 End of File
 */
