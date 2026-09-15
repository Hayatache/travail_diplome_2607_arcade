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

#ifndef _GES_MENU_H    /* Guard against multiple inclusion */
#define _GES_MENU_H


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include "Ges_Adc.h"
/* This section lists the other files that are included in this file.
 */

/* TODO:  Include other files here if needed. */


/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif
    
    
#define MAX_BRIGHTNESS 15
#define MIN_BRIGHTNESS 0
    
typedef enum
{
	/* Application's state machine's initial state. */
	MENU_CALIBRATION = 0,
    MENU_DEFAULT,
    MENU_GAME_IN_PROGRESS,
    MENU_GAME_WON,
    MENU_GAME_LOST,
    MENU_END_GAME,
    MENU_GAME_STOPPED,
    MENU_SETTING
	/* TODO: Define states used by the application state machine. */

} MENU_STATES;

typedef enum
{
	/* Application's state machine's initial state. */
	SET_DEFAULT = 0,
    SET_BRIGHTNESS,
    SET_TIME,
    SEE_TIME,
    SEE_NBR_PLAYED_GAME_SESSION,
    SEE_NBR_PLAYED_GAME_ALL_TIME,
    SEE_SYS_TEMP,
    SEE_ALL_TIME_BEST_SCORE,
    SEE_LAST_SCORE,
    SYS_RECALIBRATION,
            
	/* TODO: Define states used by the application state machine. */

} SETTING_STATES;

typedef enum
{
	/* Application's state machine's initial state. */
	MOD_DEFAULT = 0,
    MOD_ENTERING_SETTING,
    MOD_EXITING_SETTING,
            
	/* TODO: Define states used by the application state machine. */

} MODIFICATION_STATES;

  typedef struct
  {
      float LM92_Temp;
      float DS18B20_Stepper_1;
      float DS18B20_Stepper_2;
      
  } SYS_TEMP;
  
  typedef struct
  {
      uint32_t nbr_game_all_time;
      uint32_t nbr_game_session;
      uint32_t best_time_score;
      uint32_t last_time_score;
      SYS_TEMP sys_temp;
      uint8_t sys_brightness;
      uint32_t Magic;
  } SYS_INFO;




void Gestion_Menu(JOYSTICK_DATA *ptr_joystickData);
    void Gestion_Menu_Setting(JOYSTICK_DATA *ptr_joystickData);

   

    /* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* GES_MENU_H */

/* *****************************************************************************
 End of File
 */
