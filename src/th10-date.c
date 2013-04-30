/** \file
 * TH10 -- Torgoen T10 analog style
 *
 */
#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
//#include "pebble_th.h"

#define UUID { 0xC4, 0x2B, 0x0D, 0x34, 0x5D, 0xC6, 0x47, 0x79, 0x87, 0xD6, 0x31, 0x58, 0xC8, 0x68, 0x08, 0xB0 }

PBL_APP_INFO(
             UUID,
             "TH10 Date",
             "hudson & Jnm",
             1, 0, // Version
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_WATCH_FACE
             );


static Window window;
static Layer hand_layer;
static Layer bg_layer;
static PblTm now;
static GFont font_time;
//static GFont font_date;

static int use_24hour;
#define USE_0_INSTEAD_OF_24 true
#define WHITE_ON_BLACK true

// Dimensions of the watch face
#define PEBBLE_SCREEN_WIDTH 144
#define PEBBLE_SCREEN_HEIGHT 168
#define W PEBBLE_SCREEN_WIDTH
#define H PEBBLE_SCREEN_HEIGHT
#define R (W/2 - 2)

// Hour hand
static GPath hour_path;
static GPoint hour_points[] = {
    {  -8, -10 },
    { -10, -40 },
    {   0, -60 },
    { +10, -40 },
    {  +8, -10 },
};

// Minute hand
static GPath minute_path;
static GPoint minute_points[] = {
    { -5, -10 },
    { -7, -60 },
    {  0, -76 },
    { +7, -60 },
    { +5, -10 },
};

// Hour hand ticks around the circle (slightly shorter)
static GPath hour_tick_path;
static GPoint hour_tick_points[] = {
    { -3, 70 },
    { +3, 70 },
    { +3, 84 },
    { -3, 84 },
};

// Non-hour major ticks around the circle
static GPath major_tick_path;
static GPoint major_tick_points[] = {
    { -3, 60 },
    { +3, 60 },
    { +3, 84 },
    { -3, 84 },
};

// Non-major ticks around the circle; will be drawn as lines
static GPath minor_tick_path;
static GPoint minor_tick_points[] = {
    { 0, 76 },
    { 0, 84 },
};

// Day of month box
static GPath mdbox_path;
static GPoint mdbox_points[] = {
    { 24, -8 },
    { 26, -10},
    { 48, -10 },
    { 50, -8 },
    { 50, 8 },
    { 48, 10 },
    { 26, 10 },
    { 24, 8 },
};

#define MAKEGPATH(POINTS) { sizeof(POINTS) / sizeof(*POINTS), POINTS }

// Day of month digits
static GPoint digit0[] =  {
    { 33, -5 },
    { 35, -7 },
    { 39, -7 },
    { 41, -5 },
    { 41, 5 },
    { 39, 7 },
    { 35, 7 },
    { 33, 5 },
    { 33, -5 },
};

static GPoint digit1[] =  {
    { 33, -2 },
    { 37, -7 },
    { 37, 7 },
    { 33, 7 },
    { 41, 7 }
};

static GPoint digit2[] =  {
    { 33, -5 },
    { 35, -7 },
    { 39, -7 },
    { 41, -5 },
    { 41, -2 },
    { 33, 7 },
    { 41, 7 },
};

static GPoint digit3[] =  {
    { 33, -5 },
    { 35, -7 },
    { 39, -7 },
    { 41, -5 },
    { 41, 0 },
    { 36, 0 },
    { 41, 0 },
    { 41, 5 },
    { 39, 7 },
    { 35, 7 },
    { 33, 5 },
};

static GPoint digit4[] =  {
    { 39, 7 },
    { 39, -7 },
    { 33, 3 },
    { 41, 3 },
};

static GPoint digit5[] = {
    { 41, -7 },
    { 33, -7 },
    { 33, 1 },
    { 35, -2 },
    { 39, -2 },
    { 41, 1 },
    { 41, 5 },
    { 39, 7 },
    { 35, 7 },
    { 33, 5 },
};
static GPoint digit6[] = {
    { 41, -5 },
    { 39, -7 },
    { 35, -7 },
    { 33, -5 },
    { 33, 5 },
    { 35, 7 },
    { 39, 7 },
    { 41, 5 },
    { 41, 2 },
    { 39, -1 },
    { 35, -1 },
    { 33, 2 },
};

static GPoint digit7[] = {
    { 33, -7 },
    { 41, -7 },
    { 35, 7 },
};

static GPoint digit8[] = {
    { 41, 0 },
    { 33, 0 },
    { 33, -5 },
    { 35, -7 },
    { 39, -7 },
    { 41, -5 },
    { 41, 5 },
    { 39, 7 },
    { 35, 7 },
    { 33, 5 },
    { 33, -5 },
};

static GPoint digit9[] = {
    { 33, 5 },
    { 35, 7 },
    { 39, 7 },
    { 41, 5 },
    { 41, -5 },
    { 39, -7 },
    { 35, -7 },
    { 33, -5 },
    { 33, -2 },
    { 35, 1 },
    { 39, 1 },
    { 41, -2 },
};


static GPathInfo digits[10] = {
    MAKEGPATH(digit0),
    MAKEGPATH(digit1),
    MAKEGPATH(digit2),
    MAKEGPATH(digit3),
    MAKEGPATH(digit4),
    MAKEGPATH(digit5),
    MAKEGPATH(digit6),
    MAKEGPATH(digit7),
    MAKEGPATH(digit8),
    MAKEGPATH(digit9),
};

static void
hand_layer_update(
                  Layer * const me,
                  GContext * ctx
                  )
{
    (void) me;
    
    // Draw the minute hand outline in black and filled with white
    int minute_angle = (now.tm_min * TRIG_MAX_ANGLE) / 60;
    
    gpath_rotate_to(&minute_path, minute_angle);
    graphics_context_set_fill_color(ctx, GColorWhite);
    gpath_draw_filled(ctx, &minute_path);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    gpath_draw_outline(ctx, &minute_path);
    
    // Draw the hour hand outline in black and filled with white
    // above the minute hand
    int hour_angle;
    //if (use_24hour)
    //{
        //hour_angle = ((now.tm_hour * 60 + now.tm_min) * TRIG_MAX_ANGLE) / (60 * 24);
    //} else {
        int hour = now.tm_hour%12;
        //if (hour > 12)
            //hour -= 12;
        hour_angle = ((hour * 60 + now.tm_min) * TRIG_MAX_ANGLE) / (60 * 12);
    //}
    
    gpath_rotate_to(&hour_path, hour_angle);
    graphics_context_set_fill_color(ctx, GColorWhite);
    gpath_draw_filled(ctx, &hour_path);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    gpath_draw_outline(ctx, &hour_path);

    graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_circle(ctx, GPoint(W/2, H/2), 20);

    // Draw the center circle
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_circle(ctx, GPoint(W/2,H/2), 3);
    
}


/** Called once per minute */
static void
handle_tick(
            AppContextRef ctx,
            PebbleTickEvent * const event
            )
{
    (void) ctx;
    
    // If the day of month changes, for a redraw of the background
    if (now.tm_mday != event->tick_time->tm_mday)
        layer_mark_dirty(&bg_layer);
    
    layer_mark_dirty(&hand_layer);
    
    now = *event->tick_time;
}


/** Draw the initial background image */
static void
bg_layer_update(
                Layer * const me,
                GContext * ctx
                )
{
    (void) me;
    
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_fill_color(ctx, GColorWhite);
    
    // Draw the outside marks
    for (int min = 0 ; min < 60 ; min++)
    {
        const int angle = (min * TRIG_MAX_ANGLE) / 60;
        if ((min % 15) == 0)
        {
            gpath_rotate_to(&hour_tick_path, angle);
            gpath_draw_filled(ctx, &hour_tick_path);
        } else
            if ((min % 5) == 0)
            {
                gpath_rotate_to(&major_tick_path, angle);
                gpath_draw_filled(ctx, &major_tick_path);
            } else {
                gpath_rotate_to(&minor_tick_path, angle);
                gpath_draw_outline(ctx, &minor_tick_path);
            }
    }
    
    // And the large labels
    graphics_context_set_text_color(ctx, GColorWhite);
	bool shift = use_24hour && (now.tm_hour >=12);
    graphics_text_draw(ctx,
                       shift ? (USE_0_INSTEAD_OF_24 ? "0" : "24") : "12",
                       font_time,
                       GRect(W/2-30,4,60,50),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL
                       );
    
    graphics_text_draw(ctx,
                       shift ? "15" : "3",
                       font_time,
                       GRect(W/2,H/2-26,70,50),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentRight,
                       NULL
                       );
    
    graphics_text_draw(ctx,
                       shift ? "18" : "6",
                       font_time,
                       GRect(W/2-30,110,60,50),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL
                       );
    
    graphics_text_draw(ctx,
                       shift ? "21" : "9",
                       font_time,
                       GRect(W/2-70,H/2-26,60,50),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft,
                       NULL
                       );
    
    // Draw a small box with the current date
    int mday = now.tm_mday;
	/*
    char mday_str[3] = {
        '0' + (mday / 10),
        '0' + (mday % 10),
        '\0'
    };
   */ 
#if 0
    graphics_context_set_fill_color(ctx, GColorWhite);
    const int date_x = W/2+25;
    const int date_y = H/2+25;
    
    graphics_fill_rect(
                       ctx,
                       GRect(date_x, date_y, 20, 18),
                       2,
                       GCornersAll
                       );
    
    graphics_context_set_text_color(ctx, GColorBlack);
    graphics_text_draw(
                       ctx,
                       mday_str,
                       font_date,
                       GRect(date_x, date_y-1, 20, 18),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL
                       );
#else
    // lower right corner? Looks ok, but not classic
	/*
    const int date_x = W - 20;
    const int date_y = H - 18;
    
    graphics_text_draw(
                       ctx,
                       mday_str,
                       font_date,
                       GRect(date_x, date_y-1, 20, 18),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL
                       );
	*/

    
	gpath_rotate_to(&mdbox_path, TRIG_MAX_ANGLE / 8);
    graphics_context_set_fill_color(ctx, GColorWhite);
	if (WHITE_ON_BLACK) {
    	gpath_draw_outline(ctx, &mdbox_path);
		graphics_context_set_stroke_color(ctx, GColorWhite);
	} else {
    	gpath_draw_filled(ctx, &mdbox_path);
		graphics_context_set_stroke_color(ctx, GColorBlack);
	}

    int ndigits = (mday<10)?0:1;
    int digit[2] = { mday%10, mday/10 };
    
    for (int d=ndigits; d>=0; d--) {
        for (int i=0; i<digits[digit[d]].num_points-1; i++) {
            for (int j=0; j<4; j++) {
                GPoint p1, p2;
                int offset = ndigits * (4 - 9*d);

                p1.x = digits[digit[d]].points[i].x + j%2 + offset;
                p1.y = digits[digit[d]].points[i].y + j/2 + offset;
                p2.x = digits[digit[d]].points[i+1].x + j%2 + offset;
                p2.y = digits[digit[d]].points[i+1].y + j/2 + offset;
                graphics_draw_line(ctx, p1, p2);
            }
        }
	}
#endif
}

#define GPATH_INIT(PATH, POINTS) ({ \
GPathInfo __info = { sizeof(POINTS) / sizeof(*POINTS), POINTS }; \
gpath_init(PATH, &__info); \
})

static void
handle_init(
            AppContextRef ctx
            )
{
    (void) ctx;
    
    GPATH_INIT(&hour_path, hour_points);
    gpath_move_to(&hour_path, GPoint(W/2,H/2));
    
    GPATH_INIT(&minute_path, minute_points);
    gpath_move_to(&minute_path, GPoint(W/2,H/2));
    
    GPATH_INIT(&major_tick_path, major_tick_points);
    gpath_move_to(&major_tick_path, GPoint(W/2,H/2));
    
    GPATH_INIT(&hour_tick_path, hour_tick_points);
    gpath_move_to(&hour_tick_path, GPoint(W/2,H/2));
    
    GPATH_INIT(&minor_tick_path, minor_tick_points);
    gpath_move_to(&minor_tick_path, GPoint(W/2,H/2));
    
    GPATH_INIT(&mdbox_path, mdbox_points);
    gpath_move_to(&mdbox_path, GPoint(W/2,H/2));
    
    get_time(&now);
    use_24hour = clock_is_24h_style();
    
    window_init(&window, "Main");
    window_stack_push(&window, true);
    window_set_background_color(&window, GColorBlack);
    
    resource_init_current_app(&RESOURCES);
    
    font_time = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_GILLSANS_40));
    //font_date = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_GILLSANS_16));
    
	// Rotate digits
	int i, p;
	int32_t cos, sin;
	int32_t a = TRIG_MAX_ANGLE / 8;
	int x, y;
	cos = cos_lookup(a);
	sin = sin_lookup(a);
	for (i=0; i<10; i++) {
		for (p=0; p<digits[i].num_points; p++) {
			x = cos*digits[i].points[p].x/0xffff - sin*digits[i].points[p].y/0xffff;
			y = sin*digits[i].points[p].x/0xffff + cos*digits[i].points[p].y/0xffff;

			digits[i].points[p].x = W/2 + x;
			digits[i].points[p].y = H/2 + y;
		}
	}

    layer_init(&bg_layer, GRect(0, 0, W, H));
    layer_add_child(&window.layer, &bg_layer);
    bg_layer.update_proc = bg_layer_update;
    layer_mark_dirty(&bg_layer);
    
    layer_init(&hand_layer, GRect(0, 0, W, H));
    layer_add_child(&window.layer, &hand_layer);
    hand_layer.update_proc = hand_layer_update;
    layer_mark_dirty(&hand_layer);
}


static void
handle_deinit(
              AppContextRef ctx
              )
{
    (void) ctx;
    
    fonts_unload_custom_font(font_time);
    //fonts_unload_custom_font(font_date);
}


void
pbl_main(
         void * const params
         )
{
    PebbleAppHandlers handlers = {
        .init_handler   = &handle_init,
        .deinit_handler = &handle_deinit,
        .tick_info      = {
            .tick_handler = &handle_tick,
            .tick_units = SECOND_UNIT,
        },
    };
    
    app_event_loop(params, &handlers);
}
