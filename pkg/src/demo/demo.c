/*
 * demo that uses af-edge to manage attributes for a hub.
 *
 * this demo assumes that the hub has a profile with... FIXME
 */

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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
#define ATTR_ASR_STATE 65013
#define ASR_STATE_INITIALIZED 4

#define AFLIB3

/* for demo purposes, pretend the battery level drains every 15 seconds */

static bool g_siren_enabled = 0;
static int8_t g_siren_pattern = 0;
static int8_t g_battery = 100;
static bool g_battery_charging = 1;
static int16_t g_siren_duration = 10;
static char g_dev_serNum[33] = "1.0.0";
static char g_plat_verNum[33] = "1.0.0";
static char g_hub_verNum[33] = "1.0.0";

#ifdef AFLIB3
static af_lib_t *s_af_lib = NULL;
#endif

static void notify_handler(const uint16_t attr_id, const uint16_t value_len, const uint8_t *value) {
    char hex_buffer[1024];
    af_util_buffer_to_hex(hex_buffer, sizeof(hex_buffer), value, value_len);
    AFLOG_INFO("EDGE DEMO: NOTIFY: %i = 0x%s", attr_id, hex_buffer);
    if (attr_id == ATTR_ASR_STATE && value_len == 1 && value[0] == ASR_STATE_INITIALIZED) {
#ifdef AFLIB3
        af_lib_set_attribute_bool(s_af_lib, ATTR_SIREN_ENABLED, g_siren_enabled);
        af_lib_set_attribute_8(s_af_lib, ATTR_SIREN_PATTERN, g_siren_pattern);
        af_lib_set_attribute_8(s_af_lib, ATTR_BATTERY_LEVEL, g_battery);
        af_lib_set_attribute_bool(s_af_lib, ATTR_BATTERY_CHARGE, g_battery_charging);
        af_lib_set_attribute_16(s_af_lib, ATTR_SIREN_DURATION, g_siren_duration);
        af_lib_set_attribute_str(s_af_lib, ATTR_DEV_SER_NUM, strlen(g_dev_serNum), g_dev_serNum);
        af_lib_set_attribute_str(s_af_lib, ATTR_PLAT_VER_NUM, strlen(g_plat_verNum), g_plat_verNum);
        af_lib_set_attribute_str(s_af_lib, ATTR_HUB_VER_NUM, strlen(g_hub_verNum), g_hub_verNum);
#else
        aflib_set_attribute_bool(ATTR_SIREN_ENABLED, g_siren_enabled);
        aflib_set_attribute_i8(ATTR_SIREN_PATTERN, g_siren_pattern);
        aflib_set_attribute_i8(ATTR_BATTERY_LEVEL, g_battery);
        aflib_set_attribute_bool(ATTR_BATTERY_CHARGE, g_battery_charging);
        aflib_set_attribute_i16(ATTR_SIREN_DURATION, g_siren_duration);
        aflib_set_attribute_str(ATTR_DEV_SER_NUM, strlen(g_dev_serNum), g_dev_serNum);
        aflib_set_attribute_str(ATTR_PLAT_VER_NUM, strlen(g_plat_verNum), g_plat_verNum);
        aflib_set_attribute_str(ATTR_HUB_VER_NUM, strlen(g_hub_verNum), g_hub_verNum);
#endif
    }
}

#ifdef AFLIB3

static void event_handler(const af_lib_event_type_t event_type, const af_lib_error_t error, const uint16_t attr_id, const uint16_t value_len, const uint8_t *value) {
    AFLOG_INFO("EDGE DEMO: EVENT: event_type=%d, error=%d", event_type, error);
    switch(event_type) {
        case AF_LIB_EVENT_MCU_DEFAULT_NOTIFICATION :
            if (attr_id == ATTR_DEV_SER_NUM && value_len < sizeof(g_dev_serNum)) {
                AFLOG_INFO("EDGE DEMO: take default value for attribute %d", attr_id);
                strcpy(g_dev_serNum, (char *)value);
                g_dev_serNum[value_len] = '\0';
            }
            break;
        case AF_LIB_EVENT_MCU_SET_REQUEST : {
            char hex_buffer[1024];
            af_util_buffer_to_hex(hex_buffer, sizeof(hex_buffer), value, value_len);
            AFLOG_INFO("EDGE DEMO: SET: %i = 0x%s", attr_id, hex_buffer);
            bool ret = true;

            // allow updates to any attribute except the one we're demo'ing.
            if (attr_id == ATTR_SIREN_ENABLED) {

                // reject any value outside 0 - 1.
                int8_t enabled = (int8_t) *value;
                if (enabled < 0 || enabled > 1) {
                    fprintf(stderr, "SET rejected.\n");
                    ret = false;
                } else {
                    g_siren_enabled = enabled;
                }
            }
            af_lib_send_set_response(s_af_lib, attr_id, ret, value_len, value);
            break;
        }
        case AF_LIB_EVENT_ASR_NOTIFICATION :
            notify_handler(attr_id, value_len, value);
            break;
        case AF_LIB_EVENT_MCU_SET_REQ_SENT :
            AFLOG_DEBUG1("seq req sent: attr_id=%d", attr_id);
            break;
        case AF_LIB_EVENT_MCU_SET_REQ_REJECTION :
            AFLOG_ERR("Set request for attribute %d was rejected: error=%d", attr_id, error);
            break;
        default :
            AFLOG_INFO("EDGE event %d not handled", event_type);
            break;
    }
}
#else
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

#endif

static void update_battery(evutil_socket_t fd, short events, void *arg) {
    // only drain if siren is enabled
    if (g_siren_enabled) {
        AFLOG_INFO("EDGE DEMO: decrement battery level!");
        g_battery--;
        if (g_battery < 0) {
            g_battery += 100;
        }
#ifdef AFLIB3
        af_lib_set_attribute_8(s_af_lib, ATTR_BATTERY_LEVEL, g_battery);
#else
        aflib_set_attribute_i8(ATTR_BATTERY_LEVEL, g_battery);
#endif
    }
}

int main(int argc, char **argv) {
    struct event_base *ev = event_base_new();

    AFLOG_INFO("EDGE DEMO: connecting to hubby");
#ifdef AFLIB3
    af_lib_set_event_base(ev);
    if ((s_af_lib = af_lib_create_with_unified_callback(event_handler, NULL)) == NULL) {
#else
    if (aflib_init(ev, set_handler, notify_handler) != AF_SUCCESS) {
#endif
        perror("aflib_init");
        exit(1);
    }


    AFLOG_INFO("EDGE DEMO: online & monitoring");

    /* set a recurring timer for 15 seconds */
    struct event *timer = event_new(ev, -1, EV_TIMEOUT | EV_PERSIST, update_battery, NULL);
    struct timeval period = { .tv_sec = 15, .tv_usec = 0 };
    event_add(timer, &period);

    event_base_dispatch(ev);
}
