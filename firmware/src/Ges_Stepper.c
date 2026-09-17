#include <math.h>
#include "Ges_Stepper.h"
#include "app.h"
#include "peripheral/ports/plib_ports.h"
#include "driver/oc/drv_oc.h"

extern APP_DATA appData;

/* Les deux tiges du systeme, definies dans app.c.
   stepper_1_Data -> Stepper_ID = 0
   stepper_2_Data -> Stepper_ID = 1                                     */
extern STEPPER_DATA stepper_1_Data;
extern STEPPER_DATA stepper_2_Data;


/* Distance horizontale entre les deux tiges, en mm. */
#define STEPPER_BASE_DISTANCE_MM     175.0f

/* Deplacement lineaire d'une tige pour un pas moteur, en mm/pas.
   = (pas de vis en mm) / (pas par tour * micro-pas).  */
#define STEPPER_MM_PER_STEP          0.0125f

/* Angle maximum autorise, en degres. */
#define STEPPER_MAX_TILT_ANGLE_DEG   15.0f

/* tan(15 deg). A changer si STEPPER_MAX_TILT_ANGLE_DEG change. */
#define STEPPER_MAX_TILT_TANGENT     0.26794919f

/* Ecart maximum autorise entre Position_X des deux tiges, en pas.   */
#define STEPPER_MAX_DELTA_STEPS \
    ((int32_t)((STEPPER_BASE_DISTANCE_MM * STEPPER_MAX_TILT_TANGENT) / STEPPER_MM_PER_STEP))

/* Offset de du moteur pas-à-pas 0 si les deux fins de courses du bas ne sont pas aligné */
#define STEPPER_ZERO_OFFSET_STEPS    0

/* Memorise si le dernier mouvement d'une tige a ete interrompu par la
   limite d'angle (indexe par Stepper_ID).*/
static bool s_tiltLimitBlocked[2] = { false, false };

/* Ecart courant entre les deux tiges, en pas, signe. */
static inline int32_t Stepper_GetDeltaSteps(void)
{
    return (stepper_1_Data.Position_X - stepper_2_Data.Position_X)
           - STEPPER_ZERO_OFFSET_STEPS;
}

//----------------------------------------------------------------------------------//
//-- nom fct : APP_Stepper_GetTiltAngleDeg
//-- paramètre entrée : aucun
//-- paramètre sortie : float angle
//-- description : calcule l'angle d'inclinaison actuel du système
//----------------------------------------------------------------------------------//
float APP_Stepper_GetTiltAngleDeg(void)
{
    int32_t deltaSteps    = Stepper_GetDeltaSteps();
    float   deltaHeightMM = (float)deltaSteps * STEPPER_MM_PER_STEP;
    float   angleRad      = atanf(fabsf(deltaHeightMM) / STEPPER_BASE_DISTANCE_MM);

    return angleRad * (180.0f / 3.14159265358979323846f);
}

//----------------------------------------------------------------------------------//
//-- nom fct : APP_Stepper_TiltLimitBlocked
//-- paramètre entrée : int32_t stepperId
//-- paramètre sortie : bool
//-- description : indique si le mouvement d'une tige a été bloqué par la limite d'angle
//----------------------------------------------------------------------------------//
bool APP_Stepper_TiltLimitBlocked(int32_t stepperId)
{
    if ((stepperId < 0) || (stepperId > 1))
    {
        return false;
    }
    return s_tiltLimitBlocked[stepperId];
}

//----------------------------------------------------------------------------------//
//-- nom fct : APP_Stepper_GetMaxDeltaSteps
//-- paramètre entrée : aucun
//-- paramètre sortie : int32_t
//-- description : retourne l'écart maximum autorisé entre les deux tiges
//----------------------------------------------------------------------------------//
int32_t APP_Stepper_GetMaxDeltaSteps(void)
{
    return STEPPER_MAX_DELTA_STEPS;
}

//----------------------------------------------------------------------------------//
//-- nom fct : Stepper_WouldExceedTiltLimit
//-- paramètre entrée : const STEPPER_DATA *stepperData
//-- paramètre sortie : bool
//-- description : vérifie si le prochain pas dépasserait la limite d'inclinaison
//----------------------------------------------------------------------------------//
static bool Stepper_WouldExceedTiltLimit(const STEPPER_DATA *stepperData)
{
    int32_t curDelta = Stepper_GetDeltaSteps();
    int32_t newDelta;
    int32_t step = (stepperData->Stepper_Direction == STEPPER_DIR_UP) ? 1 : -1;

    if (stepperData->Stepper_ID == 0)
    {
        newDelta = curDelta + step;     /* tige 1 monte  -> ecart augmente */
    }
    else if (stepperData->Stepper_ID == 1)
    {
        newDelta = curDelta - step;     /* tige 2 monte  -> ecart diminue  */
    }
    else
    {
        return false;                   /* ID inconnu : pas de limitation  */
    }

    int32_t curAbs = (curDelta < 0) ? -curDelta : curDelta;
    int32_t newAbs = (newDelta < 0) ? -newDelta : newDelta;

    return (newAbs > STEPPER_MAX_DELTA_STEPS) && (newAbs > curAbs);
}

//----------------------------------------------------------------------------------//
//-- nom fct : Stepper_ApplyDirectionPin
//-- paramètre entrée : STEPPER_DATA *stepperData
//-- paramètre sortie : aucune
//-- description : configure la broche de direction du moteur
//----------------------------------------------------------------------------------//
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

//----------------------------------------------------------------------------------//
//-- nom fct : Stepper_SetEnable
//-- paramètre entrée : STEPPER_DATA *stepperData, bool enableActive
//-- paramètre sortie : aucune
//-- description : active ou désactive le driver du moteur
//----------------------------------------------------------------------------------//
void Stepper_SetEnable(STEPPER_DATA *stepperData, bool enableActive)
{
    bool level = !enableActive;   /* ENABLE actif a l'etat bas */

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

//----------------------------------------------------------------------------------//
//-- nom fct : APP_Stepper_Init
//-- paramètre entrée : STEPPER_DATA *stepperData
//-- paramètre sortie : aucune
//-- description : initialise la structure de données du moteur pas-à-pas
//----------------------------------------------------------------------------------//
void APP_Stepper_Init(STEPPER_DATA *stepperData)
{
    stepperData->state           = STEPPER_STBY;
    stepperData->Request_to_move = false;
    stepperData->nb_step_left    = 0;
    stepperData->TMR0_is_done    = false;

    if ((stepperData->Stepper_ID >= 0) && (stepperData->Stepper_ID <= 1))
    {
        s_tiltLimitBlocked[stepperData->Stepper_ID] = false;
    }

    Stepper_ApplyDirectionPin(stepperData);
    Stepper_SetEnable(stepperData, true);
}

//----------------------------------------------------------------------------------//
//-- nom fct : Stepper_StartOC
//-- paramètre entrée : STEPPER_DATA *stepperData
//-- paramètre sortie : aucune
//-- description : génère une impulsion pour faire avancer le moteur
//----------------------------------------------------------------------------------//
static void Stepper_StartOC(STEPPER_DATA *stepperData)
{
    if(stepperData->Stepper_ID == 0)
    {
        DRV_OC0_Stop();
        DRV_OC0_Start();
    }
    else if(stepperData->Stepper_ID == 1)
    {
        DRV_OC1_Stop();
        DRV_OC1_Start();
    }
}

//----------------------------------------------------------------------------------//
//-- nom fct : Stepper_TargetIsAllowed
//-- paramètre entrée : const STEPPER_DATA *stepperData, int32_t targetPos
//-- paramètre sortie : bool
//-- description : vérifie si une position cible peut être atteinte
//----------------------------------------------------------------------------------//
static bool Stepper_TargetIsAllowed(const STEPPER_DATA *stepperData, int32_t targetPos)
{
    /* verifie que la position calibrée est suffisamment grande */
    if ((stepperData->Position_FC2 - stepperData->Position_FC1) <= (2 * STEPPER_SAFETY_MARGIN))
    {
        return false;
    }

    /*si on se deplace vers le haut*/
    if (targetPos > stepperData->Position_X)
    {
        
        /*verifie si la position reste avant la limite du fin de courses*/
        return (targetPos <= (stepperData->Position_FC2 - STEPPER_SAFETY_MARGIN));
    }
    /*si on se deplace vers le bas*/
    else if (targetPos < stepperData->Position_X)
    {
        /*verifie si la position reste au dessus de la limite du fin de courses*/
        return (targetPos >= (stepperData->Position_FC1 + STEPPER_SAFETY_MARGIN));
    }

    /*la position demande est la meme que la position actuelle*/
    return false;
}

//----------------------------------------------------------------------------------//
//-- nom fct : APP_Stepper_CourseIsValid
//-- paramètre entrée : const STEPPER_DATA *stepperData
//-- paramètre sortie : bool
//-- description : vérifie que la course obtenue après calibration est valide
//----------------------------------------------------------------------------------//
bool APP_Stepper_CourseIsValid(const STEPPER_DATA *stepperData)
{
    /* la distance entre les deux fins de courses doit etre superieure a deux fois la marge de securité */
    return ((stepperData->Position_FC2 - stepperData->Position_FC1) > (2 * STEPPER_SAFETY_MARGIN));
}

//----------------------------------------------------------------------------------//
//-- nom fct : Stepper_RequestMoveTo
//-- paramètre entrée : STEPPER_DATA *stepperData, int32_t targetPos
//-- paramètre sortie : aucune
//-- description : prépare une demande de déplacement vers une position
//----------------------------------------------------------------------------------//
static void Stepper_RequestMoveTo(STEPPER_DATA *stepperData, int32_t targetPos)
{
    /*si le moteur est deja en train de bouger on ne demande pas le nouveau mouvement*/
    if (stepperData->state != STEPPER_STBY)
    {
        return;
    }

    /*verifie que la position demandée est valid*/
    if (!Stepper_TargetIsAllowed(stepperData, targetPos))
    {
        return;
    }

    /* deplacement vers le haut */
    if (targetPos > stepperData->Position_X)
    {
        /* calcule le nombre de pas a faire */
        stepperData->Stepper_nb_step   = targetPos - stepperData->Position_X;
        /* defini le nouveau sens de rotation */
        stepperData->Stepper_Direction = STEPPER_DIR_UP;
    }
    /* deplacement vers le bas */
    else
    {
        /* calcule le nombre de pas a faire */
        stepperData->Stepper_nb_step   = stepperData->Position_X - targetPos;
        /* defini le nouveau sens de rotation */
        stepperData->Stepper_Direction = STEPPER_DIR_DOWN;
    }

    /* verifie que le deplacement ne depassera pas la limite d'incliaison */
    if (Stepper_WouldExceedTiltLimit(stepperData))
    {
        
        /* dans le cas ou ca ferait un depassement, memorise */
        if ((stepperData->Stepper_ID >= 0) && (stepperData->Stepper_ID <= 1))
        {
            s_tiltLimitBlocked[stepperData->Stepper_ID] = true;
        }
        /* annule la demande de deplacement*/
        return;
    }
    /* autorise le deplacement */
    stepperData->Request_to_move = true;
}

//----------------------------------------------------------------------------------//
//-- nom fct : APP_Move_To
//-- paramètre entrée : STEPPER_DATA *pStepperData, int32_t pos
//-- paramètre sortie : aucune
//-- description : demande un déplacement vers une position précise
//----------------------------------------------------------------------------------//
void APP_Move_To(STEPPER_DATA *pStepperData, int32_t pos)
{
    Stepper_RequestMoveTo(pStepperData, pos);
}

//----------------------------------------------------------------------------------//
//-- nom fct : APP_Move_Up
//-- paramètre entrée : STEPPER_DATA *stepperData, int32_t pas
//-- paramètre sortie : aucune
//-- description : demande un déplacement d'un nombre de pas vers le haut
//----------------------------------------------------------------------------------//
void APP_Move_Up(STEPPER_DATA *stepperData, int32_t pas)
{
    Stepper_RequestMoveTo(stepperData, stepperData->Position_X + pas);
}

//----------------------------------------------------------------------------------//
//-- nom fct : APP_Move_Down
//-- paramètre entrée : STEPPER_DATA *stepperData, int32_t pas
//-- paramètre sortie : aucune
//-- description : demande un déplacement d'un nombre de pas vers le bas
//----------------------------------------------------------------------------------//
void APP_Move_Down(STEPPER_DATA *stepperData, int32_t pas)
{
    Stepper_RequestMoveTo(stepperData, stepperData->Position_X - pas);
}

//----------------------------------------------------------------------------------//
//-- nom fct : APP_Move_Calibration
//-- paramètre entrée : STEPPER_DATA *stepperData, STEPPER_DIRECTION dir, int32_t pas
//-- paramètre sortie : aucune
//-- description : demande un déplacement utilisé pendant la calibration
//----------------------------------------------------------------------------------//
void APP_Move_Calibration(STEPPER_DATA *stepperData, STEPPER_DIRECTION dir, int32_t pas)
{
    /*la calibration peut commencer uniquement lorsque le moteur est a l'arret*/
    if (stepperData->state != STEPPER_STBY)
    {
        return;
    }

    /* defini le nombre de pas a faire par deplacement de la calibration */
    stepperData->Stepper_nb_step   = pas;

    /* defini la direction de la calibration */
    stepperData->Stepper_Direction = dir;

    /*demande le demarrage du moteur */
    stepperData->Request_to_move   = true;
}

//----------------------------------------------------------------------------------//
//-- nom fct : APP_Stepper_Stop
//-- paramètre entrée : STEPPER_DATA *stepperData
//-- paramètre sortie : aucune
//-- description : arrête immédiatement le déplacement du moteur
//----------------------------------------------------------------------------------//
void APP_Stepper_Stop(STEPPER_DATA *stepperData)
{
    stepperData->nb_step_left    = 0;
    stepperData->Request_to_move = false;
    stepperData->state           = STEPPER_STBY;
}

//----------------------------------------------------------------------------------//
//-- nom fct : APP_Ges_stepper
//-- paramètre entrée : STEPPER_DATA *stepperData
//-- paramètre sortie : aucune
//-- description : gère la machine d'état du moteur pas-à-pas
//----------------------------------------------------------------------------------//
void APP_Ges_stepper(STEPPER_DATA *stepperData)
{
    switch (stepperData->state)
    {
        /*dans le cas ou le moteur est a l'arret*/
        case STEPPER_STBY:
        {
            /*verifie si un deplacement est demander*/
            if (stepperData->Request_to_move)
            {
                /*
                change le nombre de pas
                passe a l'etat de deplacement
                concidere la demande comme traitée
                */
                stepperData->nb_step_left    = stepperData->Stepper_nb_step;
                stepperData->state           = STEPPER_MOVING;
                stepperData->Request_to_move = false;

                /* reinitialise le blocage de la limite d'angle */
                if ((stepperData->Stepper_ID >= 0) && (stepperData->Stepper_ID <= 1))
                {
                    s_tiltLimitBlocked[stepperData->Stepper_ID] = false;
                }

                /* configure la direction */
                Stepper_ApplyDirectionPin(stepperData);
            }
            break;
        }

        /* dans le cas ou le moteur est en deplacement */
        case STEPPER_MOVING:
        {
            /* verifie s'il reste des pas a effectuer */
            if (stepperData->nb_step_left != 0)
            {
                /* verifie la limite d'inclinaison avant chaque pas */
                if (Stepper_WouldExceedTiltLimit(stepperData))
                {
                    /*memorise si le mouvement a été bloqué */
                    if ((stepperData->Stepper_ID >= 0) && (stepperData->Stepper_ID <= 1))
                    {
                        s_tiltLimitBlocked[stepperData->Stepper_ID] = true;
                    }

                    /* arrete le deplacement */
                    stepperData->nb_step_left        = 0;
                    stepperData->state               = STEPPER_STBY;

                    /*signale que le mouvement est terminé*/
                    stepperData->Stepper_Done_moving = true;
                    break;
                }

                /* met a jour la position selon le sens du moteur */
                if (stepperData->Stepper_Direction == STEPPER_DIR_UP)
                    stepperData->Position_X++;
                else
                    stepperData->Position_X--;

                /* genere une impulsion pour faire avancer le moteur */
                Stepper_StartOC(stepperData);

                /* decrement le nombre de pas restant */
                stepperData->nb_step_left--;
            }
            else
            {
                /* tous les pas ont été effectués */
                stepperData->state               = STEPPER_STBY;

                /* signale que le mouvement est terminé */
                stepperData->Stepper_Done_moving = true;
            }
            break;
        }
    }
}