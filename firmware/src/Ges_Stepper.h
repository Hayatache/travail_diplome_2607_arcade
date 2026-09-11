#ifndef GES_STEPPER_H
#define GES_STEPPER_H

#include <stdint.h>
#include <stdbool.h>

#define STEPPER_SAFETY_MARGIN       150
#define STEPPER_CALIB_SEARCH_STEPS  1  /* > course reelle max, a ajuster */

typedef enum
{
    STEPPER_MOVING = 0,
    STEPPER_STBY,
} STEPPER_STATES;

typedef enum
{
    STEPPER_DIR_DOWN = 1,
    STEPPER_DIR_UP   = 0
} STEPPER_DIRECTION;

typedef enum
{
    STEPPER_CALIB_FIND_BTM = 0,
    STEPPER_CALIB_FIND_TOP,
    STEPPER_CALIB_DONE
} STEPPER_CALIB_STATE;

typedef struct
{
    STEPPER_STATES       state;
    STEPPER_DIRECTION    Stepper_Direction;
    bool                 Stepper_Enable;
    int32_t              Position_FC2;
    int32_t              Position_FC1;
    int32_t              Position_X;
    int32_t              Stepper_nb_step;
    int32_t              Stepper_ID;
    int32_t              nb_step_left;
    bool                 Stepper_Done_moving;
    bool                 Request_to_move;
    float                Rapport_step_mm;
    bool                 TMR0_is_done;
    uint16_t             speed;
    uint16_t             ADC_Mid_point;
    STEPPER_CALIB_STATE  calib_state;      /* remplace calibration_done / BTM_FC_found */
} STEPPER_DATA;

/* Mouvements normaux : uniquement depuis STBY, marge de securite appliquee */
void APP_Move_To(STEPPER_DATA *pStepperData, int32_t pos);
void APP_Move_Up(STEPPER_DATA *pStepperData, int32_t pas);
void APP_Move_Down(STEPPER_DATA *pStepperData, int32_t pas);

/* Mouvement de calibration : pas de verification logicielle de limite,
   uniquement le switch materiel doit arreter le mouvement. */
void APP_Move_Calibration(STEPPER_DATA *pStepperData, STEPPER_DIRECTION dir, int32_t pas);

/* Arret immediat (utilise par la calibration a la detection d'un switch) */
void APP_Stepper_Stop(STEPPER_DATA *pStepperData);

void APP_Ges_stepper(STEPPER_DATA *pStepperData);

#endif /* STEPPER_H */