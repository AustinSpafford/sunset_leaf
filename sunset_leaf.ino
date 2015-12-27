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

const int k_sunset_period_millis= 6000;

const char *k_weather_api_key= "<removed>"; // TODO: Figure out the git-friendly way to keep this value private.
const int k_weather_refresh_period_millis= 7000;

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

static int s_remaining_millis_until_weather_refresh= 0;

static HttpClient s_weather_scratch_http_client;
static DynamicJsonBuffer s_weather_scratch_json_buffer;
static String s_weather_full_city_name;
static String s_weather_temperature_string;
static String s_weather_wind_string;

// ------ File-scope Declarations

static int handle_cycle_display_mode_command(String command_args);

static uint32_t get_purple_green_blue_cycle_color(byte color_index);

static void update_neopixel_strip(long elapsed_millis);
static void update_status_led();
static void update_weather(long elapsed_millis);

// ------ Arduino-hooks

void setup() 
{
	s_neopixel_strip.begin();
	
	// Initialize the pixels to display black.
	s_neopixel_strip.show();
	
	Particle.function("cycle_mode", handle_cycle_display_mode_command);
	
	Particle.variable("display_mode", s_cloud_variable_current_display_mode);
	Particle.variable("weather_city", s_weather_full_city_name);
	Particle.variable("weather_temp", s_weather_temperature_string);
	Particle.variable("weather_wind", s_weather_wind_string);
	
	s_millis_timestamp_of_last_loop= millis();
}

void loop() 
{
	const unsigned long current_millis_timestamp= millis();
	const long elapsed_millis= static_cast<unsigned long>(current_millis_timestamp - s_millis_timestamp_of_last_loop);
	
	s_millis_timestamp_of_last_loop= current_millis_timestamp;
	
	update_status_led();
	
	update_weather(elapsed_millis);
	
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

static void update_weather(long elapsed_millis)
{
	s_remaining_millis_until_weather_refresh-= elapsed_millis;
	
	if (s_remaining_millis_until_weather_refresh <= 0)
	{
		s_remaining_millis_until_weather_refresh+= k_weather_refresh_period_millis;

		http_response_t http_response;
		{
			http_request_t http_request;
			http_request.hostname = "api.wunderground.com";
			http_request.port = 80;
			http_request.path = ("/api/" + String(k_weather_api_key) + "/conditions/q/jp/tokyo.json");
			
			// Headers currently need to be set at init, useful for API keys etc.
			http_header_t http_headers[] = {
				{ "Accept" , "application/json" },
				{ NULL, NULL } // NOTE: Always terminate headers with NULL.
			};

			s_weather_scratch_http_client.get(http_request, http_response, http_headers);
		}

		if (http_response.status != 200)
		{
			Particle.publish(
				"error",
				("Expected a weather response-code of 200 \"OK\", but received [" + String(http_response.status) + "]"));
		}
		else
		{
			JsonObject &root_object= 
				s_weather_scratch_json_buffer.parseObject(
					const_cast<char *>(http_response.body.c_str()));
			
			if (root_object.success() == false)
			{
				Particle.publish("error", "Failed to parse the weather response.");
				Particle.publish("resp_length", String(http_response.body.length()));
			}
			else
			{
				s_weather_full_city_name= root_object["current_observation"]["display_location"]["full"];
				s_weather_temperature_string= root_object["current_observation"]["temperature_string"];
				s_weather_wind_string= root_object["current_observation"]["wind_string"];
				
				Particle.publish("weather_city", s_weather_full_city_name);
				Particle.publish("weather_temp", s_weather_temperature_string);
				Particle.publish("weather_wind", s_weather_wind_string);
			}
		}
	}
}
