// These #include statements are automatically added by the Particle IDE.
#include "application.h"
#include "HttpClient/HttpClient.h"
#include "neopixel/neopixel.h"
#include "SparkJson/SparkJson.h"

// Disable the typical arduino Processing->C++ preprocessor.
#pragma SPARK_NO_PREPROCESSOR

// Automatically connect to the wifi router. This macro specifically defines our main().
SYSTEM_MODE(AUTOMATIC);

// ------ Constants

const int k_neopixel_pin= D6;
const int k_neopixel_count= 77;
const int k_neopixel_protocol= WS2812B;

const int k_sunset_duration_millis= 6000;

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

static long s_millis_timestamp_of_last_loop= 0;

static long s_sunset_elapsed_millis= 0;

static int s_cloud_variable_current_display_mode= s_current_display_mode;

// ------ File-scope Declarations

static int handle_cycle_display_mode_command(String command_args);

static uint32_t get_purple_green_blue_cycle_color(byte cyclic_index);

static void update_neopixel_strip(long elapsed_millis);
static void update_status_led();

// ------ Arduino-hooks

void setup() 
{
	s_neopixel_strip.begin();
	
	// Initialize the pixels to display black.
	s_neopixel_strip.show();
	
	Particle.function("cycle_mode", handle_cycle_display_mode_command);
	
	Particle.variable("display_mode", s_cloud_variable_current_display_mode);
	
	s_millis_timestamp_of_last_loop= millis();
}

void loop() 
{
	const long current_millis_timestamp= millis();
	const long elapsed_millis= (current_millis_timestamp - s_millis_timestamp_of_last_loop);
	
	s_millis_timestamp_of_last_loop= current_millis_timestamp;
	
	update_status_led();
	
	update_neopixel_strip(elapsed_millis);
}

// ------ File-scope Implementations

static int handle_cycle_display_mode_command(String command_args)
{
	s_current_display_mode= static_cast<e_display_mode>((s_current_display_mode + 1) % k_display_mode_count);
	
	s_cloud_variable_current_display_mode= s_current_display_mode;
	
	Particle.publish("display_mode", String(s_current_display_mode));
	
	return 0;
}

static uint32_t get_purple_green_blue_cycle_color(byte cyclic_index)
{
	if (cyclic_index < 85)
	{
		// Purple-to-green.
		return s_neopixel_strip.Color(85 - cyclic_index * 1, cyclic_index * 3, 170 - cyclic_index * 2);
	}
	else if (cyclic_index < 170)
	{
		// Green-to-blue.
		cyclic_index -= 85;
		return s_neopixel_strip.Color(0, 255 - cyclic_index * 3, cyclic_index * 3);
	}
	else
	{
		// Blue-to-purple.
		cyclic_index -= 170;
		return s_neopixel_strip.Color(cyclic_index * 1, 0, 255 - cyclic_index * 1);
	}
}

static void update_neopixel_strip(long elapsed_millis)
{
	switch (s_current_display_mode)
	{
	   case _display_mode_high_noon:
			{
				for (int pixel_index= 0; pixel_index < s_neopixel_strip.numPixels(); pixel_index++)
				{
					s_neopixel_strip.setPixelColor(pixel_index, s_neopixel_strip.Color(0xFF, 0xFF, 0xFF));
				}
			}
			break;
			
		case _display_mode_sunset:
			{
				s_sunset_elapsed_millis= ((s_sunset_elapsed_millis + elapsed_millis) % k_sunset_duration_millis);
				
				const byte sunset_base_color_index= map(
					s_sunset_elapsed_millis,
					0, // from_min
					(k_sunset_duration_millis - 1), // from_max
					0, // to_min
					0xFF); // to_max
				
				for (int pixel_index= 0; pixel_index < s_neopixel_strip.numPixels(); pixel_index++)
				{
					s_neopixel_strip.setPixelColor(pixel_index, get_purple_green_blue_cycle_color((sunset_base_color_index + pixel_index) & 0xFF));
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
			RGB.brightness(255);
			RGB.control(false);
		}
	}
}
