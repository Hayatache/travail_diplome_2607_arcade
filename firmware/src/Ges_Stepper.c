#include <math.h>
#include "Ges_Stepper.h"
#include "app.h"
#include "peripheral/ports/plib_ports.h"
#include "driver/oc/drv_oc.h"

extern void printf_lcd(const char *fmt, ...);
extern APP_DATA appData;

/* Les deux tiges du systeme, definies dans app.c.
   stepper_1_Data -> Stepper_ID = 0
   stepper_2_Data -> Stepper_ID = 1                                     */
extern STEPPER_DATA stepper_1_Data;
extern STEPPER_DATA stepper_2_Data;

/* =====================================================================
   SURVEILLANCE DE L'ANGLE ENTRE LES DEUX TIGES
   =====================================================================
   Geometrie retenue : les deux tiges sont deux verins verticaux separes
   par une distance horizontale fixe D. L'inclinaison de la traverse qui
   les relie vaut :

        angle = atan( |delta_hauteur| / D )

   On NE calcule PAS cet angle dans l'ISR. La condition

        atan( |delta_pas| * mm_par_pas / D )  >  15 deg

   est strictement equivalente a

        |delta_pas|  >  D * tan(15 deg) / mm_par_pas

   Le membre de droite est une constante : elle est evaluee a la
   compilation (STEPPER_MAX_DELTA_STEPS) et la verification par pas se
   reduit a une comparaison d'entiers. Aucun calcul flottant dans l'ISR.
   ===================================================================== */

/* Distance horizontale entre les deux tiges, en mm. A MESURER. */
#define STEPPER_BASE_DISTANCE_MM     175.0f

/* Deplacement lineaire d'une tige pour un pas moteur, en mm/pas.
   = (pas de vis en mm) / (pas par tour * micro-pas * reduction).
   A CALCULER selon votre mecanique.                                    */
#define STEPPER_MM_PER_STEP          0.0125f

/* Angle maximum autorise, en degres. */
#define STEPPER_MAX_TILT_ANGLE_DEG   15.0f

/* tan(15 deg). A changer si STEPPER_MAX_TILT_ANGLE_DEG change. */
#define STEPPER_MAX_TILT_TANGENT     0.26794919f

/* Ecart maximum autorise entre Position_X des deux tiges, en pas.
   Constante repliee par le compilateur : aucun calcul a l'execution.   */
#define STEPPER_MAX_DELTA_STEPS \
    ((int32_t)((STEPPER_BASE_DISTANCE_MM * STEPPER_MAX_TILT_TANGENT) / STEPPER_MM_PER_STEP))

/* Decalage mecanique entre les zeros des deux tiges, en pas.
   Chaque tige est calibree sur SON propre fin de course bas : si les
   deux butees basses ne sont pas a la meme hauteur physique, un ecart
   de 0 pas ne correspond pas a une traverse horizontale. Mesurez le
   decalage residuel apres calibration et reportez-le ici.
   Laisser a 0 si les deux butees sont alignees.                        */
#define STEPPER_ZERO_OFFSET_STEPS    0

/* Memorise si le dernier mouvement d'une tige a ete interrompu par la
   limite d'angle (indexe par Stepper_ID). Pour affichage / diagnostic. */
static bool s_tiltLimitBlocked[2] = { false, false };

/* Ecart courant entre les deux tiges, en pas, signe. */
static inline int32_t Stepper_GetDeltaSteps(void)
{
    return (stepper_1_Data.Position_X - stepper_2_Data.Position_X)
           - STEPPER_ZERO_OFFSET_STEPS;
}

/* Angle courant en degres. Utilise du flottant : a n'appeler QUE depuis
   la boucle principale (affichage LCD / MAX7219), jamais depuis l'ISR. */
float APP_Stepper_GetTiltAngleDeg(void)
{
    int32_t deltaSteps    = Stepper_GetDeltaSteps();
    float   deltaHeightMM = (float)deltaSteps * STEPPER_MM_PER_STEP;
    float   angleRad      = atanf(fabsf(deltaHeightMM) / STEPPER_BASE_DISTANCE_MM);

    return angleRad * (180.0f / 3.14159265358979323846f);
}

/* Indique si la tige "stepperId" (0 ou 1) a ete arretee par la limite. */
bool APP_Stepper_TiltLimitBlocked(int32_t stepperId)
{
    if ((stepperId < 0) || (stepperId > 1))
    {
        return false;
    }
    return s_tiltLimitBlocked[stepperId];
}

/* Ecart maximum autorise, expose pour affichage / mise au point. */
int32_t APP_Stepper_GetMaxDeltaSteps(void)
{
    return STEPPER_MAX_DELTA_STEPS;
}

/* Un pas de plus dans la direction courante ferait-il depasser 15 deg ?

   Un pas qui REDUIT l'ecart reste toujours autorise, meme au-dela de la
   limite : sans cela, un systeme qui depasse les 15 deg (demarrage,
   calibration desynchronisee) serait bloque definitivement, sans aucun
   moyen de revenir. Seuls les pas qui AGGRAVENT l'inclinaison au-dela
   du seuil sont refuses.                                               */
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

/* L'OC est en OC_SET_LOW_SINGLE_PULSE_MODE : c'est la transition
   ON 0 -> 1 qui rearme l'impulsion unique. Un Enable sur un module deja
   actif ne produit aucun front.                                        */
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

static bool Stepper_TargetIsAllowed(const STEPPER_DATA *stepperData, int32_t targetPos)
{
    /* Garde-fou : si la calibration a echoue, FC1 et FC2 sont trop
       proches et tout mouvement serait refuse en silence. */
    if ((stepperData->Position_FC2 - stepperData->Position_FC1) <= (2 * STEPPER_SAFETY_MARGIN))
    {
        return false;
    }

    if (targetPos > stepperData->Position_X)
    {
        return (targetPos <= (stepperData->Position_FC2 - STEPPER_SAFETY_MARGIN));
    }
    else if (targetPos < stepperData->Position_X)
    {
        return (targetPos >= (stepperData->Position_FC1 + STEPPER_SAFETY_MARGIN));
    }
    return false;
}

bool APP_Stepper_CourseIsValid(const STEPPER_DATA *stepperData)
{
    return ((stepperData->Position_FC2 - stepperData->Position_FC1) > (2 * STEPPER_SAFETY_MARGIN));
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

    /* Refus immediat si des le premier pas la limite d'angle serait
       depassee : evite de demarrer un mouvement pour l'arreter aussitot. */
    if (Stepper_WouldExceedTiltLimit(stepperData))
    {
        if ((stepperData->Stepper_ID >= 0) && (stepperData->Stepper_ID <= 1))
        {
            s_tiltLimitBlocked[stepperData->Stepper_ID] = true;
        }
        return;
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
    /* Pas de verification de limite de POSITION ici : seul le switch
       materiel doit arreter le mouvement.  */
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

                if ((stepperData->Stepper_ID >= 0) && (stepperData->Stepper_ID <= 1))
                {
                    s_tiltLimitBlocked[stepperData->Stepper_ID] = false;
                }

                Stepper_ApplyDirectionPin(stepperData);
            }
            break;
        }

        case STEPPER_MOVING:
        {
            if (stepperData->nb_step_left != 0)
            {
                /* Surveillance de l'angle, a chaque pas */
                if (Stepper_WouldExceedTiltLimit(stepperData))
                {
                    if ((stepperData->Stepper_ID >= 0) && (stepperData->Stepper_ID <= 1))
                    {
                        s_tiltLimitBlocked[stepperData->Stepper_ID] = true;
                    }
                    stepperData->nb_step_left        = 0;
                    stepperData->state               = STEPPER_STBY;
                    stepperData->Stepper_Done_moving = true;
                    break;
                }

                if (stepperData->Stepper_Direction == STEPPER_DIR_UP)
                    stepperData->Position_X++;
                else
                    stepperData->Position_X--;

                Stepper_StartOC(stepperData);
                stepperData->nb_step_left--;
            }
            else
            {
                stepperData->state               = STEPPER_STBY;
                stepperData->Stepper_Done_moving = true;
            }
            break;
        }
    }
}