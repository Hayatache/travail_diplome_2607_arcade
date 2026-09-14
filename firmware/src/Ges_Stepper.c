#include "Ges_Stepper.h"
#include "app.h"
#include "peripheral/ports/plib_ports.h"
#include "driver/oc/drv_oc.h"

extern void printf_lcd(const char *fmt, ...);
extern APP_DATA appData;

static void Stepper_ApplyDirectionPin(STEPPER_DATA *stepperData)
{
    bool level = (stepperData->Stepper_Direction == STEPPER_DIR_UP) ? false : true;

    if(stepperData->Stepper_ID == 0)
    {
        PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_C, PORTS_BIT_POS_7, level);
    }
    else if(stepperData->Stepper_ID == 1)
    {
        PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_B, PORTS_BIT_POS_11, level);
    }
}

static void Stepper_SetEnable(STEPPER_DATA *stepperData, bool enableActive)
{
    bool level = !enableActive;

    if(stepperData->Stepper_ID == 0)
    {
        PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_C, PORTS_BIT_POS_6, level);
    }
    else if(stepperData->Stepper_ID == 1)
    {
        PLIB_PORTS_PinWrite(PORTS_ID_0, PORT_CHANNEL_C, PORTS_BIT_POS_9, level);
    }

    stepperData->Stepper_Enable = enableActive;
}

static void Stepper_StartOC(STEPPER_DATA *stepperData)
{
    if(stepperData->Stepper_ID == 0)
    {
        DRV_OC0_Start();
    }
    else if(stepperData->Stepper_ID == 1)
    {
        DRV_OC1_Start();
    }
}

/* Verification des limites : utilisee UNIQUEMENT pour les mouvements
   normaux (Move_To/Up/Down), une fois la calibration terminee. */
static bool Stepper_TargetIsAllowed(const STEPPER_DATA *stepperData, int32_t targetPos)
{
    if (targetPos > stepperData->Position_X)
    {
        int32_t limit = stepperData->Position_FC2 - STEPPER_SAFETY_MARGIN;
        return (targetPos < stepperData->Position_FC2) && (targetPos <= limit);
    }
    else if (targetPos < stepperData->Position_X)
    {
        int32_t limit = stepperData->Position_FC1 + STEPPER_SAFETY_MARGIN;
        return (targetPos >= limit);
    }
    return false;
}

static void Stepper_RequestMoveTo(STEPPER_DATA *stepperData, int32_t targetPos)
{
    if (stepperData->state != STEPPER_STBY)
    {
        return;
    }

    if (!Stepper_TargetIsAllowed(stepperData, targetPos))
    {
        return;
    }

    if (targetPos > stepperData->Position_X)
    {
        stepperData->Stepper_nb_step   = targetPos - stepperData->Position_X;
        stepperData->Stepper_Direction = STEPPER_DIR_UP;
    }
    else
    {
        stepperData->Stepper_nb_step   = stepperData->Position_X - targetPos;
        stepperData->Stepper_Direction = STEPPER_DIR_DOWN;
    }

    stepperData->Request_to_move = true;
}

void APP_Move_To(STEPPER_DATA *pStepperData, int32_t pos)
{
    Stepper_RequestMoveTo(pStepperData, pos);
}

void APP_Move_Up(STEPPER_DATA *stepperData, int32_t pas)
{
    Stepper_RequestMoveTo(stepperData, stepperData->Position_X + pas);
}

void APP_Move_Down(STEPPER_DATA *stepperData, int32_t pas)
{
    Stepper_RequestMoveTo(stepperData, stepperData->Position_X - pas);
}

void APP_Move_Calibration(STEPPER_DATA *stepperData, STEPPER_DIRECTION dir, int32_t pas)
{
    /* Pas de verification de limite ici : seul le switch materiel,
       surveille par Stepper_RunCalibration, doit arreter le mouvement. */
    if (stepperData->state != STEPPER_STBY)
    {
        return;
    }

    stepperData->Stepper_nb_step   = pas;
    stepperData->Stepper_Direction = dir;
    stepperData->Request_to_move   = true;
}

void APP_Stepper_Stop(STEPPER_DATA *stepperData)
{
    stepperData->nb_step_left    = 0;
    stepperData->Request_to_move = false;
    stepperData->state           = STEPPER_STBY;
}

void APP_Ges_stepper(STEPPER_DATA *stepperData)
{
    switch (stepperData->state)
    {
        case STEPPER_STBY:
        {
            if (stepperData->Request_to_move)
            {
                stepperData->nb_step_left    = stepperData->Stepper_nb_step;
                stepperData->state           = STEPPER_MOVING;
                stepperData->Request_to_move = false;
                Stepper_ApplyDirectionPin(stepperData);
            }
            break;
        }

        case STEPPER_MOVING:
        {
            stepperData->TMR0_is_done = true;
            if (stepperData->TMR0_is_done)
            {

                if (stepperData->nb_step_left != 0)
                {
                    if (stepperData->Stepper_Direction == STEPPER_DIR_UP)
                        stepperData->Position_X++;
                    else
                        stepperData->Position_X--;

                    Stepper_StartOC(stepperData);
                    stepperData->nb_step_left--;
                }
                else
                {
                    stepperData->state              = STEPPER_STBY;
                    stepperData->Stepper_Done_moving = true;
                }
            }
            break;
        }
    }
}