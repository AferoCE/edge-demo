/*
 * demo that uses af-edge to manage attributes for a hub.
 *
 * this demo assumes that the hub has a profile with... FIXME
 */

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <af_util.h>
#include <aflib.h>

/* "battery level" on the d-link hub */
#define ATTR_SIREN_ENABLED 1
#define ATTR_SIREN_PATTERN 2
#define ATTR_BATTERY_LEVEL 3
#define ATTR_BATTERY_CHARGE 4
#define ATTR_SIREN_DURATION 5
#define ATTR_DEV_SER_NUM 6
#define ATTR_PLAT_VER_NUM 7
#define ATTR_HUB_VER_NUM 8

#define verCount 5

/* for demo purposes, pretend the battery level drains every 15 seconds */

static bool g_siren_enabled = 0;
static int8_t g_siren_pattern = 0;
static int8_t g_battery = 100;
static bool g_battery_charging = 1;
static int16_t g_siren_duration = 10;
static char g_dev_serNum[] = "1.0.0";
static char g_plat_verNum[] = "1.0.0";
static char g_hub_verNum[] = "1.0.0";

static bool set_handler(const uint16_t attr_id, const uint16_t value_len, const uint8_t *value) {
    char hex_buffer[1024];
    af_util_buffer_to_hex(hex_buffer, sizeof(hex_buffer), value, value_len);
    AFLOG_INFO("EDGE DEMO: SET: %i = 0x%s", attr_id, hex_buffer);

    // allow updates to any attribute except the one we're demo'ing.
    if (attr_id != ATTR_SIREN_ENABLED) return true;

    // reject any value outside 0 - 1.
    int8_t enabled = (int8_t) *value;
    if (enabled < 0 || enabled > 1) {
        fprintf(stderr, "SET rejected.\n");
        return false;
    }
    g_siren_enabled = enabled;
    return true;
}

static void notify_handler(const uint16_t attr_id, const uint16_t value_len, const uint8_t *value) {
    char hex_buffer[1024];
    af_util_buffer_to_hex(hex_buffer, sizeof(hex_buffer), value, value_len);
    AFLOG_INFO("EDGE DEMO: NOTIFY: %i = 0x%s", attr_id, hex_buffer);
}

static void update_battery(evutil_socket_t fd, short events, void *arg) {
    // only drain if siren is enabled
    if (g_siren_enabled) {
        AFLOG_INFO("EDGE DEMO: decrement battery level!");
        g_battery--;
        if (g_battery < 0) {
            g_battery += 100;
        }
        aflib_set_attribute_i8(ATTR_BATTERY_LEVEL, g_battery);
    }
}

int main(int argc, char **argv) {
    struct event_base *ev = event_base_new();

    AFLOG_INFO("EDGE DEMO: connecting to hubby");
    if (aflib_init(ev, set_handler, notify_handler) != AF_SUCCESS) {
        perror("aflib_init");
        exit(1);
    }

    /*
     * after starting, we must call "set" for every attribute we control.
     */
    aflib_set_attribute_bool(ATTR_SIREN_ENABLED, g_siren_enabled);
    aflib_set_attribute_i8(ATTR_SIREN_PATTERN, g_siren_pattern);
    aflib_set_attribute_i8(ATTR_BATTERY_LEVEL, g_battery);
    aflib_set_attribute_bool(ATTR_BATTERY_CHARGE, g_battery_charging);
    aflib_set_attribute_i16(ATTR_SIREN_DURATION, g_siren_duration);
    aflib_set_attribute_str(ATTR_DEV_SER_NUM, verCount, g_dev_serNum);
    aflib_set_attribute_str(ATTR_PLAT_VER_NUM, verCount, g_plat_verNum);
    aflib_set_attribute_str(ATTR_HUB_VER_NUM, verCount, g_hub_verNum);

    AFLOG_INFO("EDGE DEMO: online & monitoring");

    /* set a recurring timer for 15 seconds */
    struct event *timer = event_new(ev, -1, EV_TIMEOUT | EV_PERSIST, update_battery, NULL);
    struct timeval period = { .tv_sec = 15, .tv_usec = 0 };
    event_add(timer, &period);

    event_base_dispatch(ev);
}
