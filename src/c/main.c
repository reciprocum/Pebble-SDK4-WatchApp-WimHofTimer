/*
   WatchApp: Wim Hof timer
   File    : main.c
   Author  : Afonso Santos, Portugal

   Last revision: 25 October 2016
*/

#include <pebble.h>

#define PHASE1_SEC10TH_DEFAULT -300
#define PHASE2_SEC10TH_DEFAULT -150
#define SEC10TH_MAX            +999

#define PHASE_YOFFSET_PCT       6
#define PHASE1_YOFFSET_PCT      20
#define PHASE2_YOFFSET_PCT      60
#define PHASE_YSIZE_PCT         10
#define PHASE1_YSIZE_PCT        45
#define PHASE2_YSIZE_PCT        PHASE1_YSIZE_PCT


static Window    *s_window ;
static Layer     *s_window_layer ;

static TextLayer *s_phase_text_layer ;
static Layer     *s_phase_layer ;

static TextLayer *s_phase1_text_layer ;
static Layer     *s_phase1_layer ;

static TextLayer *s_phase2_text_layer ;
static Layer     *s_phase2_layer ;

static AppTimer  *s_chronos_updateTimer = NULL ;

static int s_phase1_sec10th = PHASE1_SEC10TH_DEFAULT ;
static int s_phase2_sec10th = PHASE2_SEC10TH_DEFAULT ;

typedef enum
{ PHASE_UNDEFINED
, PHASE_0  // Deep breath
, PHASE_1  // Exhale + hold
, PHASE_2  // Inhale + hold
, PHASE_3  // Breathe normaly
} Phase ;

static Phase s_phase = PHASE_UNDEFINED ;

GSize unobstructed_screen ;


// Forward declaration
void chronos_timer_handler( void *data ) ;
void set_phase( Phase pPhase ) ;
void next_phase( ) ;
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
  text_layer_set_text( s_phase1_text_layer, format_sec10th( phase1_sec10th_text, 10, s_phase1_sec10th ) ) ;
}


void
phase2_timer_show( )
{
  static char phase2_sec10th_text[10] ;
  text_layer_set_text( s_phase2_text_layer, format_sec10th( phase2_sec10th_text, 10, s_phase2_sec10th ) ) ;
}


void
set_phase( Phase pPhase )
{
  if (pPhase == s_phase)
    return ;

  switch (s_phase = pPhase)
  {
    case PHASE_0:
      text_layer_set_text( s_phase_text_layer, "0: Deep breath" ) ;

      s_phase1_sec10th = PHASE1_SEC10TH_DEFAULT ;
      phase1_timer_show( ) ;

      s_phase2_sec10th = PHASE2_SEC10TH_DEFAULT ;
      phase2_timer_show( ) ;
    break ;

    case PHASE_1:
      s_chronos_updateTimer = app_timer_register( 0, chronos_timer_handler, NULL ) ;
      text_layer_set_text( s_phase_text_layer, "1: Exhale & hold" ) ;
    break ;

    case PHASE_2:
      text_layer_set_text( s_phase_text_layer, "2: Inhale & hold" ) ;
    break ;

    case PHASE_3:
      if (s_chronos_updateTimer != NULL)
      {
        app_timer_cancel( s_chronos_updateTimer ) ;
        s_chronos_updateTimer = NULL ;
      }

      text_layer_set_text( s_phase_text_layer, "3: Breathe normally" ) ;
    break ;

    default:
    break ;
  }
}


void
next_phase( )
{
  switch (s_phase)
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
  switch (s_phase)
  {
    case PHASE_1:
      if (s_phase1_sec10th < SEC10TH_MAX)
      {
        if (++s_phase1_sec10th == 0)
          vibes_short_pulse( ) ;
      
        phase1_timer_show( ) ;
      }
      else
        next_phase( ) ;

      s_chronos_updateTimer = app_timer_register( 100, chronos_timer_handler, NULL ) ;
    break ;

    case PHASE_2:
      if (s_phase2_sec10th < SEC10TH_MAX)
      {
        if (++s_phase2_sec10th == 0)
          vibes_short_pulse( ) ;

        phase2_timer_show( ) ;
        s_chronos_updateTimer = app_timer_register( 100, chronos_timer_handler, NULL ) ;
      }
      else
        next_phase( ) ;
    break ;

    default:
    break ;
  }
}


void
phase_click_handler( ClickRecognizerRef recognizer, void *context )
{ next_phase( ) ; }


void
reset_click_handler( ClickRecognizerRef recognizer, void *context )
{ set_phase( PHASE_0 ) ; }


void
click_config_provider( void *context )
{
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

    case ACCEL_AXIS_Y:    // Twist: next s_phase
      next_phase( ) ;
    break ;

    default:
    break ;
  }
}


int16_t
percentOf( const int16_t pct, const int16_t max )
{ return (max * pct) / 100 ; }


void
unobstructed_area_change_handler
( AnimationProgress progress
, void             *context
)
{
  unobstructed_screen = layer_get_unobstructed_bounds( s_window_layer ).size ;

  GRect phase_frame = layer_get_frame( s_phase_layer ) ;
  phase_frame.origin.y = percentOf( PHASE_YOFFSET_PCT, unobstructed_screen.h ) ;
  layer_set_frame( s_phase_layer, phase_frame ) ;

  GRect phase1_frame = layer_get_frame( s_phase1_layer ) ;
  phase1_frame.origin.y = percentOf( PHASE1_YOFFSET_PCT, unobstructed_screen.h ) ;
  layer_set_frame( s_phase1_layer, phase1_frame ) ;

  GRect phase2_frame = layer_get_frame( s_phase2_layer ) ;
  phase2_frame.origin.y = percentOf( PHASE2_YOFFSET_PCT, unobstructed_screen.h ) ;
  layer_set_frame( s_phase2_layer, phase2_frame ) ;
}


void
window_load
( Window *s_window )
{
  // Create and configure the layers
  s_window_layer      = window_get_root_layer( s_window ) ;
  unobstructed_screen = layer_get_unobstructed_bounds( s_window_layer ).size ;

  // Phase
  s_phase_text_layer = text_layer_create( GRect( 0
                                               , percentOf( PHASE_YOFFSET_PCT, unobstructed_screen.h )
                                               , unobstructed_screen.w
                                               , percentOf( PHASE_YSIZE_PCT, unobstructed_screen.h )
                                               )
                                        ) ;
  text_layer_set_text_alignment( s_phase_text_layer, GTextAlignmentCenter ) ;

  // Phase-1
  s_phase1_text_layer = text_layer_create( GRect( 0                                                        // x
                                                , percentOf( PHASE1_YOFFSET_PCT, unobstructed_screen.h )   // y
                                                , unobstructed_screen.w                                    // width
                                                , percentOf( PHASE1_YSIZE_PCT, unobstructed_screen.h )     // height
                                                )
                                         ) ;
  text_layer_set_text_alignment( s_phase1_text_layer, GTextAlignmentCenter ) ;
  text_layer_set_font( s_phase1_text_layer, fonts_get_system_font( FONT_KEY_BITHAM_42_BOLD ) ) ;

  // Phase-2
  s_phase2_text_layer = text_layer_create( GRect( 0                                                        // x
                                                , percentOf( PHASE2_YOFFSET_PCT, unobstructed_screen.h )   // y
                                                , unobstructed_screen.w                                    // width
                                                , percentOf( PHASE2_YSIZE_PCT, unobstructed_screen.h )     // height
                                                )
                                         ) ;
  text_layer_set_text_alignment( s_phase2_text_layer, GTextAlignmentCenter ) ;
  text_layer_set_font( s_phase2_text_layer, fonts_get_system_font( FONT_KEY_BITHAM_42_BOLD ) ) ;

  // Add the layers to the main window layer.
  layer_add_child( s_window_layer, s_phase_layer  = text_layer_get_layer( s_phase_text_layer  ) ) ;
  layer_add_child( s_window_layer, s_phase1_layer = text_layer_get_layer( s_phase1_text_layer ) ) ;
  layer_add_child( s_window_layer, s_phase2_layer = text_layer_get_layer( s_phase2_text_layer ) ) ;

  // Become tap aware.
  accel_tap_service_subscribe( accel_tap_service_handler ) ;

  // Obstrution handling.
  UnobstructedAreaHandlers unobstructed_area_handlers = { .change = unobstructed_area_change_handler } ;
  unobstructed_area_service_subscribe( unobstructed_area_handlers, NULL ) ;

  set_phase( PHASE_0 ) ;
}


void
window_unload
( Window *s_window )
{
  if (s_chronos_updateTimer != NULL)
  {
    app_timer_cancel( s_chronos_updateTimer ) ;
    s_chronos_updateTimer = NULL ;
  }

  // Unsubscribe services.
  accel_tap_service_unsubscribe( ) ;
  unobstructed_area_service_unsubscribe( ) ;

  // Destroy layers.
  text_layer_destroy( s_phase_text_layer ) ;
  text_layer_destroy( s_phase1_text_layer ) ;
  text_layer_destroy( s_phase2_text_layer ) ;
}


void
app_initialize
( void )
{
  s_window = window_create( ) ;
  window_set_click_config_provider( s_window, click_config_provider ) ;
 
  window_set_window_handlers( s_window
                            , (WindowHandlers)
                              { .load   = window_load
                              , .unload = window_unload
                              }
                            ) ;

  window_stack_push( s_window, true /* animated */ ) ;
}


void
app_finalize
( void )
{
  window_destroy( s_window ) ;
}


int
main
( void )
{
  app_initialize( ) ;
  app_event_loop( ) ;
  app_finalize( ) ;
}