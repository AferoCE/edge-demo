# af-edge API

The af-edge API mimics the aflib API for peripherals, allowing you to send and receive attribute updates while running on a hub.

It relies on [libevent](https://github.com/libevent/libevent) to handle the I/O event loop, and communicates through callbacks.


## building

You will need to include `aflib.h` for types and function signatures:

```c
#include <aflib.h>
```

You will also need to link with the af-edge library, its dependencies, and libevent:

```
-laf_edge -laf_ipc -laf_util -levent
```

You may also need to tell your C compiler to use C99 mode:

```
-std=c99
```


## setup

Create an `event_base` from libevent and call `aflib_init` with two callbacks (described below). The init function will connect to the hub software ("hubby") and begin monitoring attribute updates. It writes logs to syslog.

```c
af_status_t aflib_init(struct event_base *ev, aflib_set_handler_t set_handler, aflib_notify_handler_t notify_handler);
```

Many API calls return `af_status_t`, which will be `AF_SUCCESS` (0) on success.

The libevent event loop is used to receive incoming attribute updates and to process outbound updates. If you don't use libevent as your primary event loop, be sure to call `event_base_loop(ev, EVLOOP_NONBLOCK)` periodically to check for new events.


## sending updates

Attribute values are primarily byte arrays, but convenience functions exist for converting several types into little-endian byte arrays before sending the update.

```c
af_status_t aflib_set_attribute_bytes(const uint16_t attr_id, const uint16_t value_len, const uint8_t *value);
af_status_t aflib_set_attribute_bool(const uint16_t attr_id, const bool value);
af_status_t aflib_set_attribute_i8(const uint16_t attr_id, const int8_t value);
af_status_t aflib_set_attribute_i16(const uint16_t attr_id, const int16_t value);
af_status_t aflib_set_attribute_i32(const uint16_t attr_id, const int32_t value);
af_status_t aflib_set_attribute_i64(const uint16_t attr_id, const int64_t value);
af_status_t aflib_set_attribute_str(const uint16_t attr_id, const uint16_t value_len, const char *value);
```

Outbound attribute updates are enqueued. `notify_handler` will be called on completion.


## receiving updates

Incoming attribute updates will invoke the `set_handler` callback, with the attribute ID and its new value as a byte array. The callback must return `true` if the new value is accepted. If it returns `false`, the new value is rejected and the current value is retained.

```c
typedef bool (\*aflib_set_handler_t)(const uint8_t request_id, const uint16_t attr_id, const uint16_t value_len, const uint8_t \*value);
typedef void (\*aflib_notify_handler_t)(const uint8_t request_id, const uint16_t attr_id, const uint16_t value_len, const uint8_t \*value);
```

If the new value is accepted, `notify_handler` will be called with the new value.
