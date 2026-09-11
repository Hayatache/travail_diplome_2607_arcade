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
extern bool calibration_ADC_status ;
extern bool calibration_status_test ;
void __ISR(_TIMER_1_VECTOR, ipl1AUTO) IntHandlerDrvTmrInstance0(void)
{
    PLIB_INT_SourceFlagClear(INT_ID_0,INT_SOURCE_TIMER_1);
    // Timer 1 ISR
    if(ctr_tasks >= 210 ){
        ctr_tasks = 10;
        if(appData.state == APP_STATE_SERVICE_WAIT)
        {
            if(calibration_status_test)
                appData.state = APP_STATE_SERVICE_TASKS;
            else 
                appData.state = APP_STATE_SERVICE_CALIBRATION;
        }
    }
    else{
        ctr_tasks++;
    }

    if(ctr_read_adc >= 20 ){
        ctr_read_adc = 0;
        if(calibration_status_test && appData.state == APP_STATE_SERVICE_WAIT)
            appData.state = APP_STATE_SERVICE_READ_ADC;
    }
    else{
        ctr_read_adc++;
    }
}

uint16_t cpt_joystick_action = 0;
void __ISR(_TIMER_2_VECTOR, ipl1AUTO) IntHandlerDrvTmrInstance1(void)
{
    PLIB_INT_SourceFlagClear(INT_ID_0,INT_SOURCE_TIMER_2);
   
    // Timer 2 ISR
    
        stepper_1_Data.TMR0_is_done = true;
        stepper_2_Data.TMR0_is_done = true;
        if(cpt_joystick_action >= 5){
            cpt_joystick_action = 0;
            if(appData.state == APP_STATE_SERVICE_WAIT)
                appData.state = APP_STATE_SERVICE_JOYSTICK_X_ACTION;
        }
        else{
            cpt_joystick_action++;
        }
    

    
}
