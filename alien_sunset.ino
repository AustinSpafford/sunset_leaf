// These #include statements are automatically added by the Particle IDE.
#include "application.h"
#include "HttpClient/HttpClient.h"
#include "neopixel/neopixel.h"
#include "SparkJson/SparkJson.h"

// Automatically connect to the wifi router. This macro specifically defines our main().
SYSTEM_MODE(AUTOMATIC);

const int k_neopixel_pin= D6;
const int k_neopixel_count= 77;
const int k_neopixel_protocol= WS2812B;

static Adafruit_NeoPixel s_neopixel_strip=
	Adafruit_NeoPixel(
		k_neopixel_count, 
		k_neopixel_pin, 
		k_neopixel_protocol);
		
void update_status_led()
{
    if (Particle.connected())
    {
        if (RGB.controlled() == false)
        {
            // Since we're okay, silence the status-LED.
            RGB.control(true);
            RGB.brightness(0);
        }
    }
    else
    {
        if (RGB.controlled())
        {
            // Release control so the connection-status will be displayed.
            RGB.control(false);
        }
    }
}
		
void setup() 
{
	s_neopixel_strip.begin();
	
	// Initialize the pixels to display black.
	s_neopixel_strip.show();

    // Request that the cloud report back our public-facting IP address.
    Particle.subscribe("spark/", spark_event_handler);
    // Particle.publish("spark/device/ip");
    // Particle.publish("spark/device/name");
    // Particle.publish("spark/device/random");
    
}

void loop() 
{
	update_status_led();
	
	example_rainbow(50);
	
	Particle.publish(
		"loop_completed", // event_name
		NULL, // event_data
		60, // time_to_live
		PUBLIC);
}

void spark_event_handler(
    const char *event_name,
    const char *event_data)
{
	const String echo_message= ("received " + String(event_name) + ": " + String(event_data));
	
	Particle.publish(
		"echo_of_spark", // event_name
		echo_message, // event_data
		60, // time_to_live
		PUBLIC);
}

void example_rainbow(uint8_t wait)
{
	uint16_t i, j;

	for(j=0; j<256; j++) {
		for(i=0; i<s_neopixel_strip.numPixels(); i++) {
			s_neopixel_strip.setPixelColor(i, cycle_purple_green_blue((i+j) & 255));
		}
		
		s_neopixel_strip.show();
		
		delay(wait);
	}
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t example_get_hue(byte WheelPos)
{
	if(WheelPos < 85) {
	 return s_neopixel_strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
	} else if(WheelPos < 170) {
	 WheelPos -= 85;
	 return s_neopixel_strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
	} else {
	 WheelPos -= 170;
	 return s_neopixel_strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
	}
}

uint32_t cycle_white_green_blue(byte WheelPos)
{
	if(WheelPos < 85) {
		return s_neopixel_strip.Color(255 - WheelPos * 3, 255, 255 - WheelPos * 3);
	} else if(WheelPos < 170) {
		WheelPos -= 85;
		return s_neopixel_strip.Color(0, 255 - WheelPos * 3, WheelPos * 3);
	} else {
		WheelPos -= 170;
		return s_neopixel_strip.Color(WheelPos * 3, WheelPos * 3, 255);
	}
}

uint32_t cycle_purple_green_blue(byte WheelPos)
{
	if(WheelPos < 85) {
		return s_neopixel_strip.Color(85 - WheelPos * 1, WheelPos * 3, 170 - WheelPos * 2);
	} else if(WheelPos < 170) {
		WheelPos -= 85;
		return s_neopixel_strip.Color(0, 255 - WheelPos * 3, WheelPos * 3);
	} else {
		WheelPos -= 170;
		return s_neopixel_strip.Color(WheelPos * 1, 0, 255 - WheelPos * 1);
	}
}
