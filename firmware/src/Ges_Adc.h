/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Company Name

  @File Name
    filename.h

  @Summary
    Brief description of the file.

  @Description
    Describe the purpose of this file.
 */
/* ************************************************************************** */

#ifndef _GES_ADC_H    /* Guard against multiple inclusion */
#define _GES_ADC_H


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include <stdint.h>
#include <stdbool.h>
#include "Ges_Stepper.h"
/* This section lists the other files that are included in this file.


/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif

    
    #define DEAD_ZONE_JOYSTICK 100

    typedef struct
    {
        /* The application's current state */
        uint16_t Joystick_1_Y_Mid_Value;
        uint16_t Joystick_1_X_Mid_Value;
        uint16_t Joystick_2_Y_Mid_Value;
        uint16_t Joystick_2_X_Mid_Value;

                        //12
        bool Joystick_are_RR;
        bool Joystick_are_RL;
        bool Joystick_are_LR;
        bool Joystick_are_LL;
    } JOYSTICK_DATA;
    
    void ADC_TO_SPEED(STEPPER_DATA *ptr_stepperData, uint16_t adc , uint16_t *ptr_ADC_Mid_point);
    uint8_t ADC_TO_DIG(uint16_t adc , uint16_t ADC_Mid_point);
    void Scan_X_AXIS(JOYSTICK_DATA *ptr_joystickData, uint16_t adc1 , uint16_t ADC_Mid_point1, uint16_t adc2 , uint16_t ADC_Mid_point2);
    /* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _EXAMPLE_FILE_NAME_H */

/* *****************************************************************************
 End of File
 */
