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
#define ATTR_BATTERY_LEVEL 3
#define ATTR_SIREN_ENABLED 1

/* for demo purposes, pretend the battery level drains every 15 seconds */
static int8_t g_battery = 100;
static int8_t g_siren_enabled = 0;

static bool set_handler(const uint8_t request_id, const uint16_t attr_id, const uint16_t value_len, const uint8_t *value) {
    char hex_buffer[1024];
    af_util_buffer_to_hex(hex_buffer, sizeof(hex_buffer), value, value_len);
    AFLOG_INFO("EDGE DEMO: SET (request_id=%u): %i = 0x%s", request_id, attr_id, hex_buffer);

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

static void notify_handler(const uint8_t request_id, const uint16_t attr_id, const uint16_t value_len, const uint8_t *value) {
    char hex_buffer[1024];
    af_util_buffer_to_hex(hex_buffer, sizeof(hex_buffer), value, value_len);
    AFLOG_INFO("EDGE DEMO: NOTIFY (request_id=%u): %i = 0x%s", request_id, attr_id, hex_buffer);
}

static void update_battery(evutil_socket_t fd, short events, void *arg) {
    AFLOG_INFO("EDGE DEMO: decrement battery level!");
    g_battery--;
    // drain twice as fast if the siren is enabled.
    if (g_siren_enabled) g_battery--;
    if (g_battery < 0) g_battery += 100;
    aflib_set_attribute_i8(ATTR_BATTERY_LEVEL, g_battery);
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
    aflib_set_attribute_i8(ATTR_SIREN_ENABLED, g_siren_enabled);
    aflib_set_attribute_i8(ATTR_BATTERY_LEVEL, g_battery);

    AFLOG_INFO("EDGE DEMO: online & monitoring");

    /* set a recurring timer for 15 seconds */
    struct event *timer = event_new(ev, -1, EV_TIMEOUT | EV_PERSIST, update_battery, NULL);
    struct timeval period = { .tv_sec = 10, .tv_usec = 0 };
    event_add(timer, &period);

    event_base_dispatch(ev);
}
