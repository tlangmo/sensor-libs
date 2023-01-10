
#include "rgb_led.h"
#include <utility>
#include "micro_clock.h"
#include "rbs_sync_table.h"
namespace motesque {

ColorRGB Colors::black = {0,0,0};
ColorRGB Colors::white = {1,1,1};
ColorRGB Colors::red = {1,0,0};
ColorRGB Colors::blue = {0,0,1};
ColorRGB Colors::green = {0,1,0};
ColorRGB Colors::yellow = {1,1,0};
ColorRGB Colors::cyan = {0,0.5,0.5};
ColorRGB Colors::magenta = {1,0,1};
//ColorRGB Colors::notused = {0,0,0};


RgbLedDriver rgb_driver(WICED_PWM_1, WICED_PWM_2, WICED_PWM_3, WICED_PWM_4);


wiced_timed_event_t led_timer;

static void pattern_fade_period(uint64_t time_us, float period, ColorRGB* rgb, float* brightness)
{
    const uint64_t period_us = 1000000*period;
    uint64_t pos = time_us % period_us;
    uint64_t dir = (time_us / period_us) % 2;
    if (dir != 0) {
        pos = period_us-pos; // Fade in
    }
    float norm = pos/(float)period_us;
    *brightness = norm;
}

static void pattern_blink_period(uint64_t time_us, float period, ColorRGB* rgb, float* brightness)
{
    const uint64_t period_us = 1000000*period;
    uint64_t pos = time_us % period_us;
    uint64_t dir = (time_us / period_us) % 2;
    if (dir != 0) {
        pos = period_us-pos; // Fade in
    }
    float norm = pos/(float)period_us;
    *brightness = norm < 0.5 ? 0 : 1;
}

static wiced_result_t on_led_update( void* arg )
{
    RgbLed& rgb_led = RgbLed::get_instance();
    uint64_t time_us = get_time_micros();
    uint64_t time_us_synced = 0; 
    if (calculate_synced_time(time_us, &time_us_synced) == 0) {
        time_us = time_us_synced;
    }
    float brightness = 0.1;
    PatternColorPair active;
    rgb_led.get_active(&active);
    switch (active.first) {
        case LED_PATTERN_FADE_SLOW: pattern_fade_period(time_us, 2.0, nullptr ,&brightness); break;
        case LED_PATTERN_FADE_FAST: pattern_fade_period(time_us, 1.0, nullptr, &brightness); break;
        case LED_PATTERN_FADE_ULTRAFAST: pattern_fade_period(time_us, 0.2, nullptr, &brightness); break;
        case LED_PATTERN_BLINK_SLOW: pattern_blink_period(time_us, 0.4, nullptr, &brightness); break;
        case LED_PATTERN_BLINK_FAST: pattern_blink_period(time_us, 0.2, nullptr,  &brightness); break;
        case LED_PATTERN_BLINK_ULTRAFAST: pattern_blink_period(time_us, 0.1, nullptr,  &brightness); break;
        case LED_PATTERN_SOLID: brightness = 1; break;
        default: brightness = 0;
    }
    rgb_driver.set_color(active.second);
    rgb_driver.set_brightness(brightness);
    return WICED_SUCCESS;
}

RgbLed::RgbLed():
    m_active_pattern(), m_mutex()
{
    m_active_pattern = std::make_pair(LED_PATTERN_DISABLED, Colors::black);
    wiced_rtos_init_mutex(&m_mutex);
}

RgbLed::~RgbLed()
{
    wiced_rtos_deinit_mutex(&m_mutex);
}

int RgbLed::initialize()
{
    memset(&led_timer, 0, sizeof(wiced_timed_event_t));
        int rc = rgb_driver.init();
    for (int i=0; i < LED_PRIORITY_COUNT; i++) {
        m_patterns[i] = std::make_pair(LED_PATTERN_DISABLED, Colors::black);
    }
    // we use an existing wiced thread to update the led...
    wiced_rtos_register_timed_event( &led_timer, WICED_HARDWARE_IO_WORKER_THREAD, on_led_update, 100, this );
    return rc;
}

RgbLed& RgbLed::get_instance()
{
    static RgbLed led;
    return led;
}


void RgbLed::set_update_interval(uint32_t interval_ms)
{
    if (led_timer.thread) {
        wiced_rtos_deregister_timed_event(&led_timer);
        memset(&led_timer, 0, sizeof(wiced_timed_event_t));
    }
    wiced_rtos_register_timed_event( &led_timer, WICED_HARDWARE_IO_WORKER_THREAD, on_led_update, interval_ms, nullptr );
}

void RgbLed::get_active(PatternColorPair* active)
{
    wiced_rtos_lock_mutex(&m_mutex);
    *active = m_active_pattern;
    wiced_rtos_unlock_mutex(&m_mutex);
}

void RgbLed::set(LedPriority prio, LedPattern pattern, const ColorRGB& rgb)
{
    // this function will be called from many thread contexts. We need to guard it to avoid data corruption
    wiced_rtos_lock_mutex(&m_mutex);
    m_patterns[prio] = std::make_pair(pattern, rgb);
    // update the active pattern based on the priorities;
    for (int p=LED_PRIORITY_COUNT-1; p >= 0; p--) {
        if (m_patterns[p].first != LED_PATTERN_DISABLED) {
            m_active_pattern = m_patterns[p];
            wiced_rtos_unlock_mutex(&m_mutex);
            return;
        }
    }
    wiced_rtos_unlock_mutex(&m_mutex);
}

void RgbLed::set_from_hex(uint32_t color_hex, LedPriority priority)
{
    ColorRGB rgb = {(float) ((color_hex >> 16) & 0xff) / 255.0f, (float)((color_hex >> 8) & 0xff) / 255.0f, (float)(color_hex & 0xff) / 255.0f};
    set(priority, m_active_pattern.first, rgb);
}

} // end ns
