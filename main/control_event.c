#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "control_event.h"

static const char* TAG = "Control_Event";

void control_event_task(void* arg) {
    
    vTaskDelete(NULL);
}

/**
 * Initialise the control event subsysttem
*/

esp_err_t control_event_init() {

}