// These #include statements are automatically added by the Particle IDE.
#include "application.h"
#include "neopixel/neopixel.h"

// Disable the typical arduino Processing->C++ preprocessor.
#pragma SPARK_NO_PREPROCESSOR

// Automatically connect to the wifi router. This macro specifically defines our main().
SYSTEM_MODE(AUTOMATIC);

// Run the particle-system in a separate thread, specifically so
// wifi-reconnects and the like do not interrupt/reset any animations.
SYSTEM_THREAD(ENABLED);

// ------ Constants

const int k_neopixel_pin= D6;
const int k_neopixel_count= 77;
const int k_neopixel_protocol= WS2812B;

const int k_sunset_period_millis= 6000;

enum e_display_mode
{
	_display_mode_high_noon,
	_display_mode_sunset,
	
	k_display_mode_count
};

// ------ File-scope Variables

static Adafruit_NeoPixel s_neopixel_strip=
	Adafruit_NeoPixel(
		k_neopixel_count, 
		k_neopixel_pin, 
		k_neopixel_protocol);

static e_display_mode s_current_display_mode= _display_mode_sunset;
static int s_cloud_variable_current_display_mode= s_current_display_mode;

static unsigned long s_millis_timestamp_of_last_loop= 0;

static long s_sunset_elapsed_millis= 0;

// ------ File-scope Declarations

static uint32_t get_purple_green_blue_cycle_color(byte color_index);

static int handle_cycle_display_mode_command(String command_args);

static void handle_web_alert_event(const char *event, const char *data);

static void update_neopixel_strip(long elapsed_millis);
static void update_status_led();

// ------ Arduino-hooks

void setup() 
{
	s_neopixel_strip.begin();
	
	// Initialize the pixels to display black.
	s_neopixel_strip.show();
	
	Particle.function("cycle_mode", handle_cycle_display_mode_command);
	
	Particle.subscribe("web_alert", handle_web_alert_event);
	
	Particle.variable("display_mode", s_cloud_variable_current_display_mode);
	
	s_millis_timestamp_of_last_loop= millis();
}

void loop() 
{
	const unsigned long current_millis_timestamp= millis();
	const long elapsed_millis= (current_millis_timestamp - s_millis_timestamp_of_last_loop);
	
	s_millis_timestamp_of_last_loop= current_millis_timestamp;
	
	update_status_led();
	
	update_neopixel_strip(elapsed_millis);
}

// ------ File-scope Implementations

static uint32_t get_purple_green_blue_cycle_color(byte color_index)
{
	if (color_index < 85)
	{
		// Purple-to-green.
		return s_neopixel_strip.Color(85 - color_index * 1, color_index * 3, 170 - color_index * 2);
	}
	else if (color_index < 170)
	{
		// Green-to-blue.
		color_index -= 85;
		return s_neopixel_strip.Color(0, 0xFF - color_index * 3, color_index * 3);
	}
	else
	{
		// Blue-to-purple.
		color_index -= 170;
		return s_neopixel_strip.Color(color_index * 1, 0, 0xFF - color_index * 1);
	}
}

static int handle_cycle_display_mode_command(String command_args)
{
	s_current_display_mode= static_cast<e_display_mode>((s_current_display_mode + 1) % k_display_mode_count);
	
	s_cloud_variable_current_display_mode= s_current_display_mode;
	
	Particle.publish("display_mode", String(s_current_display_mode));
	
	return 0;
}

static void handle_web_alert_event(const char *event, const char *data)
{
	// TODO: Replace this testing-stub.
	handle_cycle_display_mode_command("");
}

static void update_neopixel_strip(long elapsed_millis)
{
	switch (s_current_display_mode)
	{
		case _display_mode_high_noon:
		{
			for (int pixel_index= 0; pixel_index < s_neopixel_strip.numPixels(); pixel_index++)
			{
				s_neopixel_strip.setPixelColor(
					pixel_index,
					s_neopixel_strip.Color(0xFF, 0xFF, 0xFF));
			}
		}
		break;
		
		case _display_mode_sunset:
		{
			s_sunset_elapsed_millis= ((s_sunset_elapsed_millis + elapsed_millis) % k_sunset_period_millis);
			
			const byte base_color_index= map(
				s_sunset_elapsed_millis,
				0, // from_min
				(k_sunset_period_millis - 1), // from_max
				0, // to_min
				0xFF); // to_max
			
			for (int pixel_index= 0; pixel_index < s_neopixel_strip.numPixels(); pixel_index++)
			{
				const byte pixel_color_index= ((base_color_index + pixel_index) & 0xFF);
				
				s_neopixel_strip.setPixelColor(
					pixel_index,
					get_purple_green_blue_cycle_color(pixel_color_index));
			}
		}
		break;
	}
	
	s_neopixel_strip.show();
}

static void update_status_led()
{
	if (Particle.connected())
	{
		if (RGB.controlled() == false)
		{
			// Since we're doing just fine, silence the status-LED.
			RGB.control(true);
			RGB.brightness(0);
		}
	}
	else
	{
		if (RGB.controlled())
		{
			// Release control so the connection-status will be displayed.
			RGB.brightness(0xFF);
			RGB.control(false);
		}
	}
}
