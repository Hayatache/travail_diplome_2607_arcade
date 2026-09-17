
#include "Ges_Adc.h"
#include "Ges_Stepper.h"
#include <stdint.h>
#include <stdbool.h>
#define HYSTERESIS_JOYSTICK  20


//----------------------------------------------------------------------------------//
//-- nom fct : ADC_TO_SPEED
//-- paramètre entrée : STEPPER_DATA *ptr_stepperData, uint16_t adc,
//--                     uint16_t *ptr_ADC_Mid_point
//-- paramètre sortie : aucune
//-- description : convertit la position du joystick en direction et vitesse moteur
//----------------------------------------------------------------------------------//
void ADC_TO_SPEED(STEPPER_DATA *ptr_stepperData,
                  uint16_t adc,
                  uint16_t *ptr_ADC_Mid_point)
{
    int Speed_Value_ADC;

    uint16_t upper_start =
        *ptr_ADC_Mid_point + DEAD_ZONE_JOYSTICK + HYSTERESIS_JOYSTICK;

    uint16_t upper_stop =
        *ptr_ADC_Mid_point + DEAD_ZONE_JOYSTICK;

    uint16_t lower_start =
        *ptr_ADC_Mid_point - DEAD_ZONE_JOYSTICK - HYSTERESIS_JOYSTICK;

    uint16_t lower_stop =
        *ptr_ADC_Mid_point - DEAD_ZONE_JOYSTICK;


    /* ========================= */
    /* Joystick vers le haut      */
    /* ========================= */

    if (ptr_stepperData->Stepper_Direction == 1)
    {
        /* Le moteur est d�j� en mouvement vers le haut */

        if (adc <= upper_stop)
        {
            /* Retour dans la zone morte */
            ptr_stepperData->speed = 0;
            ptr_stepperData->Stepper_Direction = 2;
            return;
        }

        /* Plus l'ADC augmente, plus la vitesse augmente */
        Speed_Value_ADC =
            1 + ((adc - upper_stop) * 4)
            / (1023 - upper_stop);

        if (Speed_Value_ADC < 1)
            Speed_Value_ADC = 1;

        if (Speed_Value_ADC > 5)
            Speed_Value_ADC = 5;

        ptr_stepperData->speed = Speed_Value_ADC;
    }


    /* ========================= */
    /* Joystick vers le bas      */
    /* ========================= */

    else if (ptr_stepperData->Stepper_Direction == 0)
    {
        /* Le moteur est deja en mouvement vers le bas */

        if (adc >= lower_stop)
        {
            /* Retour dans la zone morte */
            ptr_stepperData->speed = 0;
            ptr_stepperData->Stepper_Direction = 2;
            return;
        }

        /* Plus l'ADC diminue, plus la vitesse augmente */
        Speed_Value_ADC =
            1 + ((lower_stop - adc) * 4)
            / lower_stop;

        if (Speed_Value_ADC < 1)
            Speed_Value_ADC = 1;

        if (Speed_Value_ADC > 5)
            Speed_Value_ADC = 5;

        ptr_stepperData->speed = Speed_Value_ADC;
    }


    /* ========================= */
    /* Moteur a l'arret          */
    /* ========================= */

    else
    {
        if (adc > upper_start)
        {
            /* Demarrage vers le haut */
            ptr_stepperData->Stepper_Direction = 1;
        }
        else if (adc < lower_start)
        {
            /* Demarrage vers le bas */
            ptr_stepperData->Stepper_Direction = 0;
        }
        else
        {
            ptr_stepperData->speed = 0;
            ptr_stepperData->Stepper_Direction = 2;
            return;
        }
    }
}

//----------------------------------------------------------------------------------//
//-- nom fct : ADC_TO_DIG
//-- paramètre entrée : uint16_t adc, uint16_t *ADC_Mid_point
//-- paramètre sortie : uint8_t direction
//-- description : détermine la direction du joystick à partir de la valeur ADC
//----------------------------------------------------------------------------------//
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

//----------------------------------------------------------------------------------//
//-- nom fct : Scan_X_AXIS
//-- paramètre entrée : JOYSTICK_DATA *ptr_joystickData,
//--                     uint16_t adc1, uint16_t *ADC_Mid_point1,
//--                     uint16_t adc2, uint16_t *ADC_Mid_point2
//-- paramètre sortie : aucune
//-- description : détecte les combinaisons de déplacement des deux joysticks
//----------------------------------------------------------------------------------//
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