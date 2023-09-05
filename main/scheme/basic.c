#include "sdkconfig.h"

#ifdef CONFIG_CT_SCHEME_BASIC

#include "basic.h"

static const char* TAG = "scheme_basic";

SCHEME_INIT {
    SCHEME_SET_PAGE_COUNT(3);

    SCHEME_SET_GESTURE_MASK(0, 0 | (1 << SIGNAL_C));
    SCHEME_SET_GESTURE_MASK(1, ~0);
    SCHEME_SET_GESTURE_MASK(2, 0 | (1 << SIGNAL_C));
}

SCHEME_ON_GESTURE {
    SCHEME_BEGIN_EVENT(event)
    ESP_LOGI(TAG, "Button %d, pressed %d, %s", event->button, event->gesture.count, event->gesture.long_press ? "long press" : "normal press");

    switch(event->button) {
        case SIGNAL_C:
            
            if(event->gesture.long_press) {
                if(event->gesture.count == 1) {
                    hid_cc_change_key(HID_CONSUMER_VOICE_ASSIST, true);
                    hid_cc_change_key(HID_CONSUMER_VOICE_ASSIST, false);
                } else {
                    SCHEME_SELECT_PAGE(event->gesture.count - 1);
                }
                return;
            }

            switch(event->gesture.count) {
                case 2:
                    hid_cc_change_key(HID_CONSUMER_PLAY_PAUSE, true);
                    hid_cc_change_key(HID_CONSUMER_PLAY_PAUSE, false);
                break;
                case 3:
                    hid_cc_change_key(HID_CONSUMER_SCAN_NEXT_TRK, true);
                    hid_cc_change_key(HID_CONSUMER_SCAN_NEXT_TRK, false);
                break;
                case 4:
                    hid_cc_change_key(HID_CONSUMER_SCAN_PREV_TRK, true);
                    hid_cc_change_key(HID_CONSUMER_SCAN_PREV_TRK, false);
                break;
                case 5:
                    SCHEME_NEXT_PAGE();
                break;
                default:
                break;
            }
            
            break;
        default:
        break;
    }
}

SCHEME_ON_PLAIN {
    SCHEME_BEGIN_EVENT(event)
    ESP_LOGI(TAG, "Button %d, pressed %d", event->button, event->plain.state);

    switch(event->button) {
        case JOY_SEL:
            hid_keyboard_change_key(HID_KEY_ENTER, (bool) event->plain.state);
        break;
        case JOY_D:
            hid_keyboard_change_key(HID_KEY_DOWN_ARROW, (bool) event->plain.state);
        break;
        case JOY_U:
            hid_keyboard_change_key(HID_KEY_UP_ARROW, (bool) event->plain.state);
        break;
        case JOY_R:
            hid_keyboard_change_key(HID_KEY_RIGHT_ARROW, (bool) event->plain.state);
        break;
        case JOY_L:
            hid_keyboard_change_key(HID_KEY_LEFT_ARROW, (bool) event->plain.state);
        break;
        default:
        break;
    }
}

#endif