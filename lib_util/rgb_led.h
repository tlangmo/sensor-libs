#pragma once
#include "wiced.h"
#include "rgb_led_driver.h"
#include <map>
namespace motesque {

struct Colors {
    static ColorRGB white;
    static ColorRGB black;
    static ColorRGB red;
    static ColorRGB blue;
    static ColorRGB green;
    static ColorRGB yellow;
    static ColorRGB cyan;
    static ColorRGB magenta;
    static ColorRGB notused;
};

// Leds patterns are a higher-level interface to the RGBLedDriver.
enum LedPattern {
    LED_PATTERN_DISABLED = 0,
    LED_PATTERN_SOLID,
    LED_PATTERN_FADE_ULTRAFAST,
    LED_PATTERN_FADE_FAST,
    LED_PATTERN_FADE_SLOW,
    LED_PATTERN_BLINK_SLOW,
    LED_PATTERN_BLINK_FAST,
    LED_PATTERN_BLINK_ULTRAFAST
};
enum LedPriority {
    LED_PRIORITY_IDLE = 0,
    LED_PRIORITY_NETWORK,
    LED_PRIORITY_APP,
    LED_PRIORITY_ERROR,
    LED_PRIORITY_COUNT
};
typedef std::pair<LedPattern, ColorRGB> PatternColorPair;

//  Central point to control the rgb led.
//  We can control temporal patterns and color. Each pattern-color pair has an associated priority.
//  This allows us e.g. to display network status and temporarily override this, if a use sets a color manually
//
class RgbLed {
private:
    RgbLed();
    virtual ~RgbLed();
public:
    static RgbLed& get_instance();
    int initialize();
    // all patterns rely on updates in regular intervals.
    void set_update_interval(uint32_t interval_ms);
    void set(LedPriority prio, LedPattern pattern, const ColorRGB& rgb);
    void set_from_hex(uint32_t color_hex, LedPriority priority);
    void get_active(PatternColorPair* active);
private:
    std::map<int, PatternColorPair> m_patterns;
    PatternColorPair m_active_pattern;
    wiced_mutex_t    m_mutex;
};


}
