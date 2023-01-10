#include "platform.h"
#include <array>

// A normed RGB color type
typedef std::array<float, 3> ColorRGB;

// Handles the low-level PWM based functionality of the RGB led.
// Color and brightness can be set independently to allow easy fading and blinking
// without changing the color.
class RgbLedDriver
{
public:
    //#ifdef E1459
    RgbLedDriver(wiced_pwm_t pinR, wiced_pwm_t pinG, wiced_pwm_t pinB, wiced_pwm_t pinW);
    //#else
    //RgbLedDriver(wiced_pwm_t pinR, wiced_pwm_t pinG, wiced_pwm_t pinB, wiced_pwm_t);
    //#endif
    ~RgbLedDriver();
    int set_color(const ColorRGB& rgb);
    int get_color(ColorRGB* rgb);
    // the global brightness. All color channles get pre-multiplied by that factor
    int set_brightness(float brightness);
    // setup the platform pwm
    int init();
    // release the platform pwm
    void deinit();
private:
    std::array<wiced_pwm_t, 3> m_pins;
    ColorRGB m_colors;
    wiced_pwm_t m_pin_white;
    bool m_white_active;
};
