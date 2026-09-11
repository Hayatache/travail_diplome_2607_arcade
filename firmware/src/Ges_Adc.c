
#include "Ges_Adc.h"
#include "Ges_Stepper.h"
#include <stdint.h>
#include <stdbool.h>
void ADC_TO_SPEED(STEPPER_DATA *ptr_stepperData, uint16_t adc , uint16_t *ptr_ADC_Mid_point){

    int Speed_Value_ADC;
    if (adc > *ptr_ADC_Mid_point + DEAD_ZONE_JOYSTICK)
        {
        // Joystick vers le haut 
        Speed_Value_ADC =
            5 - ((adc - (*ptr_ADC_Mid_point + DEAD_ZONE_JOYSTICK)) * 4)
            / (1023 - (*ptr_ADC_Mid_point + DEAD_ZONE_JOYSTICK));
        if (Speed_Value_ADC < 2)
            Speed_Value_ADC = 2;
        if (Speed_Value_ADC > 5)
            Speed_Value_ADC = 5;
        ptr_stepperData->speed = Speed_Value_ADC;
        ptr_stepperData->Stepper_Direction = 1;
     }
     else if (adc < *ptr_ADC_Mid_point - DEAD_ZONE_JOYSTICK)
     {
        // Joystick vers le bas 
        Speed_Value_ADC =
            5 - (((*ptr_ADC_Mid_point - DEAD_ZONE_JOYSTICK) - adc) * 4)
            / (*ptr_ADC_Mid_point - DEAD_ZONE_JOYSTICK);
        if (Speed_Value_ADC < 2)
            Speed_Value_ADC = 2;
        if (Speed_Value_ADC > 5)
            Speed_Value_ADC = 5;
        ptr_stepperData->speed = Speed_Value_ADC;
        ptr_stepperData->Stepper_Direction = 0;
     }
     else
     {
        // Joystick au milieu 
        Speed_Value_ADC = 0;
        ptr_stepperData->speed = 0;
        ptr_stepperData->Stepper_Direction = 2;
     }
}

uint8_t ADC_TO_DIG(uint16_t adc , uint16_t ADC_Mid_point){

    if (adc > ADC_Mid_point + DEAD_ZONE_JOYSTICK)
    {
        // Joystick vers le haut
        return 2;
    }
    else if (adc < ADC_Mid_point - DEAD_ZONE_JOYSTICK)
    {
        // Joystick vers le bas
        return 1;
    }
    else
    {
        // Joystick au milieu
        return 0;
    }
}

void Scan_X_AXIS(JOYSTICK_DATA *ptr_joystickData, uint16_t adc1 , uint16_t ADC_Mid_point1, uint16_t adc2 , uint16_t ADC_Mid_point2){
    static uint8_t ctr_joystick1 = 0;
    static uint8_t ctr_joystick2 = 0; 
    
    uint8_t direction_ADC1 = ADC_TO_DIG(adc1,ADC_Mid_point1); 
    uint8_t direction_ADC2 = ADC_TO_DIG(adc2,ADC_Mid_point2); 
    
    if(direction_ADC1 >= 1){
        ctr_joystick1 ++;
    }
    else{
        ctr_joystick1 = 0;
    }
    
    if(direction_ADC2 >= 1){
        ctr_joystick2 ++;
    }
    else{
        ctr_joystick2 = 0;
    }
    
    if((ctr_joystick1 >= 50 )&&( ctr_joystick2 >= 50)){
        
        ctr_joystick1 = 0;
        ctr_joystick2 = 0;
        
        if((direction_ADC1 == 1 ) && (direction_ADC2 == 1)){ // les deux gauche
            ptr_joystickData->Joystick_are_LL = true;
        }
        else if((direction_ADC1 == 2 ) && (direction_ADC2 == 2)){ // les deux droite
            ptr_joystickData->Joystick_are_RR = true;
        }
        else if((direction_ADC1 == 2 ) && (direction_ADC2 == 1)){ // un a droite et un gauche
            ptr_joystickData->Joystick_are_RL = true;
        }
        else if((direction_ADC1 == 1 ) && (direction_ADC2 == 2)){ // un a droite et un gauche 
            ptr_joystickData->Joystick_are_LR = true;
        }
    }
}