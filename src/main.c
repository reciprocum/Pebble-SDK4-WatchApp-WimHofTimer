/*
   Project: Wim Hof timer (watchapp)
   File   : main.c
   Author : Afonso Santos, Portugal

   Last revision: 09h05 August 24 2016
*/

#include <pebble.h>
#include "Config.h"

#define PHASE1_SEC10TH_DEFAULT -300
#define PHASE2_SEC10TH_DEFAULT -150
#define SEC10TH_MAX +999

static Window    *window ;
static TextLayer *phase_text_layer ;
static TextLayer *phase1_text_layer ;
static TextLayer *phase2_text_layer ;

static AppTimer  *chronos_updateTimer = NULL ;

static int phase1_sec10th = PHASE1_SEC10TH_DEFAULT ;
static int phase2_sec10th = PHASE2_SEC10TH_DEFAULT ;

typedef enum
{ PHASE_UNDEFINED
, PHASE_0  // Hyperventilate
, PHASE_1  // Exhale + hold
, PHASE_2  // Inhale + hold
, PHASE_3  // Breathe normaly
} Phase ;

static Phase phase = PHASE_UNDEFINED ;


// Forward declaration
void chronos_timer_handler( void *data ) ;
void set_phase( Phase pPhase ) ;
void cycle_phase( ) ;
void phase_click_handler( ClickRecognizerRef recognizer, void *context ) ;
void reset_click_handler( ClickRecognizerRef recognizer, void *context ) ;
void click_config_provider( void *context ) ;


char *
format_sec10th( char *result, const int n, const int pSec10th )
{
  char *dest = result ;
  int sec10th ;

  if (pSec10th >= 0)
  {
    sec10th = pSec10th ;
    strcpy( dest++, "+" ) ;
  }
  else
  {
    sec10th = -pSec10th ;
    strcpy( dest++, "-" ) ;
  }
    
  snprintf( dest, n-1, "%u.%u", sec10th/10, sec10th%10 ) ;
  
  return result ;
}


void
phase1_timer_show( )
{
  static char phase1_sec10th_text[10] ;
  text_layer_set_text( phase1_text_layer, format_sec10th( phase1_sec10th_text, 10, phase1_sec10th ) ) ;
}


void
phase2_timer_show( )
{
  static char phase2_sec10th_text[10] ;
  text_layer_set_text( phase2_text_layer, format_sec10th( phase2_sec10th_text, 10, phase2_sec10th ) ) ;
}


void
set_phase( Phase pPhase )
{
  LOGD( "set_phase:: pPhase=%d", pPhase ) ;

  if (pPhase == phase)
    return ;

  switch (phase = pPhase)
  {
    case PHASE_0:
      text_layer_set_text( phase_text_layer, "0: Deep breath" ) ;

      phase1_sec10th = PHASE1_SEC10TH_DEFAULT ;
      phase1_timer_show( ) ;

      phase2_sec10th = PHASE2_SEC10TH_DEFAULT ;
      phase2_timer_show( ) ;
    break ;

    case PHASE_1:
      chronos_updateTimer = app_timer_register( 0, chronos_timer_handler, NULL ) ;
      text_layer_set_text( phase_text_layer, "1: Exhale & hold" ) ;
    break ;

    case PHASE_2:
      text_layer_set_text( phase_text_layer, "2: Inhale & hold" ) ;
    break ;

    case PHASE_3:
      if (chronos_updateTimer != NULL)
      {
        app_timer_cancel( chronos_updateTimer ) ;
        chronos_updateTimer = NULL ;
      }

      text_layer_set_text( phase_text_layer, "3: Breathe normally" ) ;
    break ;

    default:
    break ;
  }
}


void
cycle_phase( )
{
  LOGD( "cycle_phase:: phase=%d", phase ) ;

  switch (phase)
  {
    case PHASE_0:
      set_phase( PHASE_1 ) ;
    break ;

    case PHASE_1:
      set_phase( PHASE_2 ) ;
    break ;

    case PHASE_2:
      set_phase( PHASE_3 ) ;
    break ;

    case PHASE_3:
      set_phase( PHASE_0 ) ;
    break ;

    default:
    break ;
  }
}


void
chronos_timer_handler
( void *data )
{
  LOGD( "chronos_timer_handler:: phase=%d", phase ) ;

  switch (phase)
  {
    case PHASE_1:
      if (phase1_sec10th < SEC10TH_MAX)
      {
        if (++phase1_sec10th == 0)
          vibes_short_pulse( ) ;
      
        phase1_timer_show( ) ;
      }
      else
        cycle_phase( ) ;

      chronos_updateTimer = app_timer_register( 100, chronos_timer_handler, NULL ) ;
    break ;

    case PHASE_2:
      if (phase2_sec10th < SEC10TH_MAX)
      {
        if (++phase2_sec10th == 0)
          vibes_short_pulse( ) ;

        phase2_timer_show( ) ;
        chronos_updateTimer = app_timer_register( 100, chronos_timer_handler, NULL ) ;
      }
      else
        cycle_phase( ) ;
    break ;

    default:
    break ;
  }
}


void
phase_click_handler( ClickRecognizerRef recognizer, void *context )
{
  LOGD( "phase_click_handler:: phase=%d", phase ) ;

  cycle_phase( ) ;
}


void
reset_click_handler( ClickRecognizerRef recognizer, void *context )
{
  LOGD( "reset_click_handler:: phase=%d", phase ) ;

  set_phase( PHASE_0 ) ;
}


void
click_config_provider( void *context )
{
  LOGD( "click_config_provider" ) ;

  window_single_click_subscribe( BUTTON_ID_SELECT, phase_click_handler ) ;
  window_single_click_subscribe( BUTTON_ID_UP    , phase_click_handler ) ;
  window_single_click_subscribe( BUTTON_ID_DOWN  , phase_click_handler ) ;

  window_long_click_subscribe( BUTTON_ID_SELECT
                             , 0                                    // Use default 500ms
                             , (ClickHandler)reset_click_handler    // Down handler.
                             , NULL                                 // Up handler.
                             ) ;

  window_long_click_subscribe( BUTTON_ID_UP
                             , 0                                    // Use default 500ms
                             , (ClickHandler)reset_click_handler    // Down handler.
                             , NULL                                 // Up handler.
                             ) ;

  window_long_click_subscribe( BUTTON_ID_DOWN
                             , 0                                    // Use default 500ms
                             , (ClickHandler)reset_click_handler    // Down handler.
                             , NULL                                 // Up handler.
                             ) ;
}


void
accel_tap_service_handler
( AccelAxisType  axis        // Process tap on ACCEL_AXIS_X, ACCEL_AXIS_Y or ACCEL_AXIS_Z
, int32_t        direction   // Direction is 1 or -1
)
{
  switch (axis)
  {
    case ACCEL_AXIS_X:    // Punch: reset
      set_phase( PHASE_0 ) ;
    break ;

    case ACCEL_AXIS_Y:    // Twist: next phase
      cycle_phase( ) ;
    break ;

    default:
    break ;
  }
}


void
window_load
( Window *window )
{
  Layer *window_root_layer = window_get_root_layer( window ) ;
  GRect bounds = layer_get_bounds( window_root_layer );

  phase_text_layer = text_layer_create( GRect(0, 20, bounds.size.w, 20) ) ;
  text_layer_set_text_alignment( phase_text_layer, GTextAlignmentCenter ) ;
  layer_add_child( window_root_layer, text_layer_get_layer( phase_text_layer ) ) ;

  phase1_text_layer = text_layer_create( GRect(0, 40, bounds.size.w, 50) ) ;
  text_layer_set_text_alignment( phase1_text_layer, GTextAlignmentCenter ) ;
  layer_add_child( window_root_layer, text_layer_get_layer( phase1_text_layer ) ) ;
  text_layer_set_font( phase1_text_layer, fonts_get_system_font( FONT_KEY_BITHAM_42_BOLD ) ) ;
  
  phase2_text_layer = text_layer_create( GRect(0, 94, bounds.size.w, 50) ) ;
  text_layer_set_text_alignment( phase2_text_layer, GTextAlignmentCenter ) ;
  layer_add_child( window_root_layer, text_layer_get_layer( phase2_text_layer ) ) ;
  text_layer_set_font( phase2_text_layer, fonts_get_system_font( FONT_KEY_BITHAM_42_BOLD ) ) ;

  // Become tap aware.
  accel_tap_service_subscribe( accel_tap_service_handler ) ;

  set_phase( PHASE_0 ) ;
}


void
window_unload
( Window *window )
{
  if (chronos_updateTimer != NULL)
  {
    app_timer_cancel( chronos_updateTimer ) ;
    chronos_updateTimer = NULL ;
  }

  // Tap unaware.
  accel_tap_service_unsubscribe( ) ;

  text_layer_destroy( phase_text_layer ) ;
  text_layer_destroy( phase1_text_layer ) ;
  text_layer_destroy( phase2_text_layer ) ;
}


void
app_init
( void )
{
  window = window_create( ) ;
  window_set_click_config_provider( window, click_config_provider ) ;
 
  window_set_window_handlers( window
                            , (WindowHandlers)
                              { .load   = window_load
                              , .unload = window_unload
                              }
                            ) ;

  const bool animated = true ;
  window_stack_push( window, animated ) ;
}


void
app_deinit
( void )
{
  window_destroy( window ) ;
}


int
main
( void )
{
  app_init( ) ;
  app_event_loop( ) ;
  app_deinit( ) ;
}