/*******************************************************************************
 System Interrupts File

  File Name:
    system_interrupt.c

  Summary:
    Raw ISR definitions.

  Description:
    This file contains a definitions of the raw ISRs required to support the
    interrupt sub-system.

  Summary:
    This file contains source code for the interrupt vector functions in the
    system.

  Description:
    This file contains source code for the interrupt vector functions in the
    system.  It implements the system and part specific vector "stub" functions
    from which the individual "Tasks" functions are called for any modules
    executing interrupt-driven in the MPLAB Harmony system.

  Remarks:
    This file requires access to the systemObjects global data structure that
    contains the object handles to all MPLAB Harmony module objects executing
    interrupt-driven in the system.  These handles are passed into the individual
    module "Tasks" functions to identify the instance of the module to maintain.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2011-2014 released Microchip Technology Inc.  All rights reserved.

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

#include "system/common/sys_common.h"
#include "app.h"
#include "system_definitions.h"
#include "Ges_Stepper.h"    
#include "AD5620.h"
#include "Ges_Menu.h"
// *****************************************************************************
// *****************************************************************************
// Section: System Interrupt Vector Functions
// *****************************************************************************
// *****************************************************************************

 extern STEPPER_DATA stepper_1_Data;
extern STEPPER_DATA stepper_2_Data;
extern APP_DATA appData;

int ctr_tasks = 0;
int ctr_read_adc = 0;
int vald = 1;
extern APP_DATA appData;
extern SOUND_DATA Sound_Data;
extern bool calibration_ADC_status ;
extern bool calibration_status_test ;


void __ISR(_TIMER_1_VECTOR, ipl2AUTO) IntHandlerDrvTmrInstance0(void)
{
    PLIB_INT_SourceFlagClear(INT_ID_0,INT_SOURCE_TIMER_1);
    // Timer 1 ISR

    // compteur pour lancer les taches principals
    if(ctr_tasks >= 210 ){
        /* reset du compteur */
        ctr_tasks = 10;

        /* si on est dans l'etat d'attente*/
        if(appData.state == APP_STATE_SERVICE_WAIT)
        {
            /* si la calibration a été faites */
            if(calibration_status_test){
                /* passe dans l'execusion de l'app tasks */
                appData.state = APP_STATE_SERVICE_TASKS;
                /* joue un tick du son */
                Play_Sound_1_Tick(&Sound_Data);
            }
            else 
            /* si la calibration n'a pas été faites, lanbce la calibration */
                appData.state = APP_STATE_SERVICE_CALIBRATION;
        }
    }
    else{
        /* incremente le compteur */
        ctr_tasks++;
    }

    /* compteur pour la lecture del 'ADC*/
    if(ctr_read_adc >= 20 ){

        /* reset du compteur */
        ctr_read_adc = 0;

        /* lance la lecture de l'ADC si la calibration est terminée */
        if(calibration_status_test && appData.state == APP_STATE_SERVICE_WAIT)
            appData.state = APP_STATE_SERVICE_READ_ADC;
    }
    else{
        /* incrementation du compteur */
        ctr_read_adc++;
    }
}

uint16_t cpt_joystick_action = 0;

void __ISR(_TIMER_2_VECTOR, ipl1AUTO) IntHandlerDrvTmrInstance1(void)
{
    PLIB_INT_SourceFlagClear(INT_ID_0,INT_SOURCE_TIMER_2);

        /*gestion des pas a pas a chaque passage dans l'isr*/
        APP_Ges_stepper(&stepper_2_Data);
        APP_Ges_stepper(&stepper_1_Data);

        /*si le compteur pour verifier les joysticks est egal a 5 */
        if(cpt_joystick_action >= 5){
            /* reset du compteur */
            cpt_joystick_action = 0;
            /* si on est dans l'etat d'attente, on se met dans l'etat d'action avec les joysticks */
            if(appData.state == APP_STATE_SERVICE_WAIT)
                appData.state = APP_STATE_SERVICE_JOYSTICK_X_ACTION;
        }
        else{
            /*sinon incrementation du compteur*/
            cpt_joystick_action++;
        }
    

    
}


/*Gestion audio*/
void __ISR(_TIMER_3_VECTOR, ipl1AUTO) IntHandlerDrvTmrInstance2(void)
{
    PLIB_INT_SourceFlagClear(INT_ID_0,INT_SOURCE_TIMER_3);
    dac_ad5620_generate_pulse();
}


extern MENU_STATES menu_state;

/* interruption du capteur de bile de victoire */
void __ISR(_EXTERNAL_2_VECTOR, IPL1AUTO) _IntHandlerExternalInterruptInstance0(void)
{
    PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_EXTERNAL_2);

    if(menu_state == MENU_GAME_IN_PROGRESS){
        menu_state = MENU_GAME_WON;
    }
}


/* interruption du capteur de bille de defaite */
void __ISR(_EXTERNAL_3_VECTOR, IPL1AUTO) _IntHandlerExternalInterruptInstance1(void)
{
    PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_EXTERNAL_3);

    if(menu_state == MENU_GAME_IN_PROGRESS){
        menu_state = MENU_GAME_LOST;
    }
}

