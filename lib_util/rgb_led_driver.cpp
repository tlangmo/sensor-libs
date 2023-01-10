#include "rgb_led_driver.h"
#include "wiced.h"
#include <algorithm>

RgbLedDriver::RgbLedDriver(wiced_pwm_t pinR, wiced_pwm_t pinG, wiced_pwm_t pinB, wiced_pwm_t pinW)
: m_pins(),m_colors()
{
    m_pin_white = pinW;
    m_pins = {pinR,pinG,pinB};
    m_colors = {1,1,1};
    m_white_active = true;
}

RgbLedDriver::~RgbLedDriver() {

}

int RgbLedDriver::set_brightness(float brightness)
{
    const float m = -30;
    const float t = 100;
  
    if (m_white_active){
        wiced_pwm_change(m_pin_white, 1000, brightness*1 > 0.01 ? m * brightness*1+t : 100);
    }
    else {
        for (size_t i=0; i < m_pins.size(); i++) {
            wiced_pwm_change( m_pins[i], 1000, brightness*m_colors[i] > 0.01 ? m * brightness*m_colors[i]+t : 100);
        }
  }

    return 0;
}

int RgbLedDriver::set_color(const ColorRGB& rgb)
{
    m_colors = rgb;
    const float m = -30;
    const float t = 100;
    ColorRGB white = {1, 1, 1};
  
    if ( rgb == white ){
        m_white_active = true;
        wiced_pwm_change(m_pin_white, 1000, m*1+t);
        for (size_t i=0; i < m_pins.size(); i++) {
            wiced_pwm_change( m_pins[i], 1000, 100);
        }
    }
    else {
        m_white_active = false;
        wiced_pwm_change(m_pin_white, 1000, 100);
        for (size_t i=0; i < m_pins.size(); i++) {
            wiced_pwm_change( m_pins[i], 1000, m_colors[i] > 0.01 ? m*m_colors[i]+t : 100);
        }
    }
    return 0;
}


int RgbLedDriver::get_color(ColorRGB* rgb)
{
    *rgb = m_colors;
    return 0;
}

int RgbLedDriver::init()
{
    int result = 0;
    std::for_each(m_pins.begin(), m_pins.end(), [&result](wiced_pwm_t pin) {
        wiced_result_t rc = wiced_pwm_init( pin, 1000, 100 );
        result |= rc == WICED_SUCCESS ? 0 : -1;
    });
    m_white_active = false;
    result |= wiced_pwm_init(m_pin_white, 1000, 100);

    return result;
}
