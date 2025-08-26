/**
 * @file main.c
 * @brief Professional ESP32-S3 4x4 Matrix Keyboard Driver Implementation
 * @author Mechatronics Engineer
 * @date August 2025
 * 
 * This implementation provides a robust, industry-grade matrix keyboard driver
 * optimized for ESP32-S3 hardware with professional features including:
 * - Hardware debouncing and software filtering
 * - Non-blocking key scanning using FreeRTOS
 * - Configurable key mapping and GPIO assignments
 * - Professional error handling and diagnostics
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "matrix_keyboard.h"
#include "hd44780.h"

/* ==================== CONFIGURATION CONSTANTS ==================== */

#define MATRIX_ROWS                 4
#define MATRIX_COLS                 4
#define DEBOUNCE_TIME_MS           50      // Professional debounce timing
#define SCAN_INTERVAL_MS           10      // Fast scanning for responsiveness
#define KEY_QUEUE_SIZE             16      // Buffer for key events

/* ==================== HAMBURGER GRILL CONSTANTS ==================== */
#define BLUE_RARE_MIN              20      // BLUE RARE temperature range
#define BLUE_RARE_MAX              25
#define MEDIUM_RARE_MIN            26      // MEDIUM RARE temperature range
#define MEDIUM_RARE_MAX            30
#define WELL_DONE_MIN              31      // WELL DONE temperature range
#define WELL_DONE_MAX              35
#define SOLE_RARE_MIN              36      // SOLE RARE temperature range
#define SOLE_RARE_MAX              40

#define TEMP_SENSOR_ADC_CHANNEL    ADC_CHANNEL_0  // ADC channel for potentiometer
#define ADC_ATTEN                  ADC_ATTEN_DB_12
#define ADC_WIDTH                  ADC_BITWIDTH_12
#define TEMP_UPDATE_INTERVAL_MS    500     // Temperature reading interval

/* ==================== GPIO PIN ASSIGNMENTS ==================== */
/* ESP32-S3 GPIO pins - easily configurable for different layouts */

// Row pins (outputs) - GPIO pins for driving rows
static const gpio_num_t row_pins[MATRIX_ROWS] = {
    GPIO_NUM_1,   // Row 0
    GPIO_NUM_2,   // Row 1
    GPIO_NUM_42,  // Row 2
    GPIO_NUM_41   // Row 3
};

// Column pins (inputs with pullup) - GPIO pins for reading columns
static const gpio_num_t col_pins[MATRIX_COLS] = {
    GPIO_NUM_40,  // Col 0
    GPIO_NUM_39,  // Col 1
    GPIO_NUM_38,  // Col 2
    GPIO_NUM_37   // Col 3
};

/* ==================== KEY MAPPING CONFIGURATION ==================== */
/* Professional keypad layout - easily customizable */
static const char key_map[MATRIX_ROWS][MATRIX_COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

/* ==================== LCD CONFIGURATION ==================== */
/* HD44780 LCD GPIO pin assignments (4-bit mode) */

static hd44780_t lcd = {
    .write_cb = NULL,  // Direct GPIO connection
    .font = HD44780_FONT_5X8,
    .lines = 2,
    .pins = {
        .rs = GPIO_NUM_10,   // Register Select
        .e  = GPIO_NUM_11,   // Enable
        .d4 = GPIO_NUM_16,   // Data bit 4
        .d5 = GPIO_NUM_17,   // Data bit 5  
        .d6 = GPIO_NUM_18,   // Data bit 6
        .d7 = GPIO_NUM_7,    // Data bit 7
        .bl = HD44780_NOT_USED  // No backlight control
    }
};

/* ==================== DATA STRUCTURES ==================== */

/**
 * @brief System state enumeration
 */
typedef enum {
    STATE_ASK_TEMPERATURE,      // Ask user to input temperature
    STATE_INPUTTING_TEMPERATURE, // User is typing temperature
    STATE_SHOWING_MEAT_TERM,    // Show determined meat term
    STATE_SHOWING_STATUS        // Show status temporarily
} system_state_t;

/**
 * @brief Hamburger cooking levels enumeration
 */
typedef enum {
    BLUE_RARE = 0,
    MEDIUM_RARE,
    WELL_DONE,
    SOLE_RARE,
    NO_DETERMINATION
} cooking_level_t;

/**
 * @brief Grill system state structure
 */
typedef struct {
    system_state_t current_state;           // Current system state
    cooking_level_t determined_level;       // Automatically determined cooking level
    int input_temperature;                  // Temperature input by user
    char temp_input_buffer[4];              // Buffer for temperature input (max 3 digits)
    int temp_input_index;                   // Current position in input buffer
    float sensor_temp;                      // Current temperature from sensor (potentiometer)
    bool temp_in_range;                     // Temperature within determined range
    bool warning_active;                    // Warning state for out of range
} grill_state_t;

/**
 * @brief Matrix keyboard state management structure
 */
typedef struct {
    bool current_state[MATRIX_ROWS][MATRIX_COLS];    // Current key states
    bool previous_state[MATRIX_ROWS][MATRIX_COLS];   // Previous key states
    uint64_t last_change_time[MATRIX_ROWS][MATRIX_COLS]; // Debounce timing
    bool initialized;                                 // Driver initialization flag
} matrix_keyboard_t;

/* ==================== GLOBAL VARIABLES ==================== */

static const char *TAG = "HAMBURGER_GRILL";
static matrix_keyboard_t keyboard = {0};
static QueueHandle_t key_event_queue = NULL;
static TaskHandle_t scan_task_handle = NULL;
static grill_state_t grill_system = {
    .current_state = STATE_ASK_TEMPERATURE,
    .determined_level = NO_DETERMINATION,
    .input_temperature = -1,
    .temp_input_buffer = {0},
    .temp_input_index = 0,
    .sensor_temp = 0.0,
    .temp_in_range = false,
    .warning_active = false
};

// Cooking level names for display
static const char* cooking_names[] = {
    "BLUE RARE",     // 20-25°C
    "MEDIUM RARE",   // 26-30°C  
    "WELL DONE",     // 31-35°C
    "SOLE RARE"      // 36-40°C
};

// ADC handles for new API
static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t adc1_cali_handle;

/* ==================== FUNCTION PROTOTYPES ==================== */

static esp_err_t matrix_keyboard_gpio_init(void);
static void matrix_keyboard_scan_task(void *pvParameters);
static bool is_key_debounced(uint8_t row, uint8_t col);
static void process_key_change(uint8_t row, uint8_t col, bool new_state);
static void matrix_keyboard_scan_once(void);

// Hamburger grill system functions
static esp_err_t temperature_sensor_init(void);
static float read_temperature_sensor(void);
static void update_grill_display(void);
static bool is_temperature_in_range(float temp, cooking_level_t level);
static cooking_level_t determine_meat_term_from_temperature(int temperature);
static void handle_temperature_input(char key);
static void process_temperature_input(void);
static void handle_control_keys(char key);
static void temperature_monitoring_task(void *pvParameters);

/* ==================== IMPLEMENTATION ==================== */

/**
 * @brief Initialize GPIO pins for matrix keyboard operation
 * @return ESP_OK on success, error code otherwise
 */
static esp_err_t matrix_keyboard_gpio_init(void)
{
    esp_err_t ret = ESP_OK;
    
    ESP_LOGI(TAG, "Initializing matrix keyboard GPIO configuration");
    
    // Configure row pins as outputs with initial HIGH state
    for (int i = 0; i < MATRIX_ROWS; i++) {
        gpio_config_t row_config = {
            .pin_bit_mask = (1ULL << row_pins[i]),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        
        ret = gpio_config(&row_config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure row pin %d: %s", 
                     row_pins[i], esp_err_to_name(ret));
            return ret;
        }
        
        // Set row to HIGH (inactive state)
        gpio_set_level(row_pins[i], 1);
    }
    
    // Configure column pins as inputs with pull-up resistors
    for (int i = 0; i < MATRIX_COLS; i++) {
        gpio_config_t col_config = {
            .pin_bit_mask = (1ULL << col_pins[i]),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        
        ret = gpio_config(&col_config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure column pin %d: %s", 
                     col_pins[i], esp_err_to_name(ret));
            return ret;
        }
    }
    
    ESP_LOGI(TAG, "GPIO configuration completed successfully");
    return ESP_OK;
}

/**
 * @brief Check if key state change is debounced (professional implementation)
 * @param row Row index
 * @param col Column index
 * @return true if debounced, false if still bouncing
 */
static bool is_key_debounced(uint8_t row, uint8_t col)
{
    uint64_t current_time = esp_timer_get_time();
    uint64_t time_diff = current_time - keyboard.last_change_time[row][col];
    
    return (time_diff >= (DEBOUNCE_TIME_MS * 1000));
}

/**
 * @brief Process key state change and generate events
 * @param row Row index
 * @param col Column index
 * @param new_state New key state (true = pressed)
 */
static void process_key_change(uint8_t row, uint8_t col, bool new_state)
{
    // Update timing for debounce
    keyboard.last_change_time[row][col] = esp_timer_get_time();
    keyboard.current_state[row][col] = new_state;
    
    // Create key event
    key_event_t event = {
        .row = row,
        .col = col,
        .key_char = key_map[row][col],
        .pressed = new_state,
        .timestamp = esp_timer_get_time()
    };
    
    // Send event to queue (non-blocking)
    BaseType_t result = xQueueSend(key_event_queue, &event, 0);
    if (result != pdTRUE) {
        ESP_LOGW(TAG, "Key event queue full, dropping event for key '%c'", 
                 event.key_char);
    } else {
        ESP_LOGI(TAG, "Key '%c' %s at position [%d,%d]", 
                 event.key_char, 
                 new_state ? "PRESSED" : "RELEASED", 
                 row, col);
    }
}

/**
 * @brief Perform one complete matrix scan cycle
 */
static void matrix_keyboard_scan_once(void)
{
    // Scan each row
    for (int row = 0; row < MATRIX_ROWS; row++) {
        // Drive current row LOW (active)
        gpio_set_level(row_pins[row], 0);
        
        // Small delay for signal stabilization (hardware consideration)
        vTaskDelay(pdMS_TO_TICKS(1));
        
        // Read all columns for this row
        for (int col = 0; col < MATRIX_COLS; col++) {
            bool current_reading = !gpio_get_level(col_pins[col]); // Inverted logic
            bool previous_state = keyboard.current_state[row][col];
            
            // Check for state change
            if (current_reading != previous_state) {
                // Verify debouncing
                if (is_key_debounced(row, col)) {
                    process_key_change(row, col, current_reading);
                }
            }
        }
        
        // Set row back to HIGH (inactive)
        gpio_set_level(row_pins[row], 1);
    }
}

/**
 * @brief Matrix keyboard scanning task (FreeRTOS task)
 * @param pvParameters Task parameters (unused)
 */
static void matrix_keyboard_scan_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Matrix keyboard scan task started");
    
    TickType_t last_wake_time = xTaskGetTickCount();
    
    while (1) {
        // Perform matrix scan
        matrix_keyboard_scan_once();
        
        // Maintain precise timing using vTaskDelayUntil
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(SCAN_INTERVAL_MS));
    }
}

/**
 * @brief Initialize matrix keyboard driver
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t matrix_keyboard_init(void)
{
    esp_err_t ret;
    
    ESP_LOGI(TAG, "Initializing professional matrix keyboard driver");
    
    // Initialize GPIO configuration
    ret = matrix_keyboard_gpio_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO initialization failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Create key event queue
    key_event_queue = xQueueCreate(KEY_QUEUE_SIZE, sizeof(key_event_t));
    if (key_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create key event queue");
        return ESP_ERR_NO_MEM;
    }
    
    // Initialize keyboard state
    memset(&keyboard, 0, sizeof(matrix_keyboard_t));
    keyboard.initialized = true;
    
    // Create scanning task with appropriate priority
    BaseType_t task_result = xTaskCreate(
        matrix_keyboard_scan_task,
        "matrix_scan",
        2048,                    // Stack size
        NULL,                    // Parameters
        5,                       // Priority (higher than normal)
        &scan_task_handle
    );
    
    if (task_result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create scanning task");
        vQueueDelete(key_event_queue);
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "Matrix keyboard driver initialized successfully");
    return ESP_OK;
}

/**
 * @brief Get next key event from queue (non-blocking)
 * @param event Pointer to store the key event
 * @param timeout_ms Timeout in milliseconds (0 = no wait)
 * @return ESP_OK if event received, ESP_ERR_TIMEOUT if no event
 */
esp_err_t matrix_keyboard_get_key(key_event_t *event, uint32_t timeout_ms)
{
    if (event == NULL || !keyboard.initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    TickType_t timeout_ticks = (timeout_ms == 0) ? 0 : pdMS_TO_TICKS(timeout_ms);
    
    if (xQueueReceive(key_event_queue, event, timeout_ticks) == pdTRUE) {
        return ESP_OK;
    }
    
    return ESP_ERR_TIMEOUT;
}

/* ==================== HAMBURGER GRILL SYSTEM IMPLEMENTATION ==================== */

/**
 * @brief Initialize ADC for temperature sensor emulation
 */
static esp_err_t temperature_sensor_init(void)
{
    esp_err_t ret = ESP_OK;
    
    // Initialize ADC1 oneshot unit
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ret = adc_oneshot_new_unit(&init_config1, &adc1_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ADC1 unit: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configure ADC1 channel
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_WIDTH,
        .atten = ADC_ATTEN,
    };
    ret = adc_oneshot_config_channel(adc1_handle, TEMP_SENSOR_ADC_CHANNEL, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Initialize ADC calibration
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .chan = TEMP_SENSOR_ADC_CHANNEL,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_WIDTH,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &adc1_cali_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "ADC calibration failed, using default values: %s", esp_err_to_name(ret));
        adc1_cali_handle = NULL;
    }
    
    ESP_LOGI(TAG, "Temperature sensor (ADC) initialized successfully");
    return ESP_OK;
}

/**
 * @brief Read temperature from potentiometer using sensor equation
 * V = 0.046 * T - 0.40
 * Solving for T: T = (V + 0.40) / 0.046
 */
static float read_temperature_sensor(void)
{
    int adc_raw = 0;
    esp_err_t ret = adc_oneshot_read(adc1_handle, TEMP_SENSOR_ADC_CHANNEL, &adc_raw);
    
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "ADC read failed: %s", esp_err_to_name(ret));
        return 25.0; // Return default temperature
    }
    
    // Convert to voltage (mV)
    int voltage_mv = 0;
    if (adc1_cali_handle != NULL) {
        ret = adc_cali_raw_to_voltage(adc1_cali_handle, adc_raw, &voltage_mv);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "ADC calibration failed: %s", esp_err_to_name(ret));
            // Use basic conversion if calibration fails
            voltage_mv = (adc_raw * 3300) / 4095; // 3.3V reference, 12-bit ADC
        }
    } else {
        // Use basic conversion without calibration
        voltage_mv = (adc_raw * 3300) / 4095; // 3.3V reference, 12-bit ADC
    }
    
    float voltage_v = voltage_mv / 1000.0;  // Convert to volts
    
    // Apply sensor equation: T = (V + 0.40) / 0.046
    float temperature = (voltage_v + 0.40) / 0.046;
    
    return temperature;
}

/**
 * @brief Check if temperature is within the selected cooking level range
 */
static bool is_temperature_in_range(float temp, cooking_level_t level)
{
    switch(level) {
        case BLUE_RARE:
            return (temp >= BLUE_RARE_MIN && temp <= BLUE_RARE_MAX);
        case MEDIUM_RARE:
            return (temp >= MEDIUM_RARE_MIN && temp <= MEDIUM_RARE_MAX);
        case WELL_DONE:
            return (temp >= WELL_DONE_MIN && temp <= WELL_DONE_MAX);
        case SOLE_RARE:
            return (temp >= SOLE_RARE_MIN && temp <= SOLE_RARE_MAX);
        default:
            return false;
    }
}

/**
 * @brief Update LCD display based on current grill system state
 */
static void update_grill_display(void)
{
    hd44780_clear(&lcd);
    hd44780_gotoxy(&lcd, 0, 0);
    
    switch(grill_system.current_state) {
        case STATE_ASK_TEMPERATURE:
            hd44780_puts(&lcd, "Enter Temp (C):");
            hd44780_gotoxy(&lcd, 0, 1);
            hd44780_puts(&lcd, "Use 0-9, # OK");
            break;
            
        case STATE_INPUTTING_TEMPERATURE:
            hd44780_puts(&lcd, "Temperature:");
            hd44780_gotoxy(&lcd, 0, 1);
            if (grill_system.temp_input_index > 0) {
                char display_temp[16];
                snprintf(display_temp, sizeof(display_temp), "%s C (# to OK)", 
                        grill_system.temp_input_buffer);
                hd44780_puts(&lcd, display_temp);
            } else {
                hd44780_puts(&lcd, "__ C (# to OK)");
            }
            break;
            
        case STATE_SHOWING_MEAT_TERM:
            if (is_temperature_in_safe_range(grill_system.input_temperature)) {
                // Temperature is in safe range (20-40°C)
                if (grill_system.determined_level != NO_DETERMINATION) {
                    // Show determined meat term
                    hd44780_puts(&lcd, cooking_names[grill_system.determined_level]);
                    hd44780_gotoxy(&lcd, 0, 1);
                    // Second line remains EMPTY for safe temperatures
                    hd44780_puts(&lcd, "                "); // Clear second line
                } else {
                    // This shouldn't happen for temperatures in 20-40 range, but just in case
                    hd44780_puts(&lcd, "Unknown Term");
                    hd44780_gotoxy(&lcd, 0, 1);
                    hd44780_puts(&lcd, "                "); // Clear second line
                }
            } else {
                // Temperature is outside safe range (< 20°C or > 40°C)
                if (grill_system.determined_level != NO_DETERMINATION) {
                    // Show determined meat term (if any)
                    hd44780_puts(&lcd, cooking_names[grill_system.determined_level]);
                } else {
                    // Show that temperature is out of range
                    hd44780_puts(&lcd, "Out of Range");
                }
                hd44780_gotoxy(&lcd, 0, 1);
                // Show warning for unsafe temperatures
                hd44780_puts(&lcd, "OH! OH! BE CAREFUL");
            }
            break;
            
        case STATE_SHOWING_STATUS:
            hd44780_puts(&lcd, "Status Check:");
            hd44780_gotoxy(&lcd, 0, 1);
            if (grill_system.input_temperature != -1) {
                char status[16];
                bool safe = is_temperature_in_safe_range(grill_system.input_temperature);
                snprintf(status, sizeof(status), "%dC %s", 
                        grill_system.input_temperature, safe ? "SAFE" : "UNSAFE");
                hd44780_puts(&lcd, status);
            } else {
                hd44780_puts(&lcd, "No temperature");
            }
            break;
    }
}

/**
 * @brief Determine meat cooking term based on input temperature
 */
static cooking_level_t determine_meat_term_from_temperature(int temperature)
{
    if (temperature >= BLUE_RARE_MIN && temperature <= BLUE_RARE_MAX) {
        return BLUE_RARE;
    } else if (temperature >= MEDIUM_RARE_MIN && temperature <= MEDIUM_RARE_MAX) {
        return MEDIUM_RARE;
    } else if (temperature >= WELL_DONE_MIN && temperature <= WELL_DONE_MAX) {
        return WELL_DONE;
    } else if (temperature >= SOLE_RARE_MIN && temperature <= SOLE_RARE_MAX) {
        return SOLE_RARE;
    } else {
        return NO_DETERMINATION;
    }
}

/**
 * @brief Check if input temperature is within safe cooking range (20-40°C)
 */
static bool is_temperature_in_safe_range(int temperature)
{
    return (temperature >= 20 && temperature <= 40);
}

/**
 * @brief Handle temperature input from keypad
 */
static void handle_temperature_input(char key)
{
    if (key >= '0' && key <= '9') {
        // Add digit to temperature input
        if (grill_system.temp_input_index < 3) { // Max 3 digits (up to 999°C)
            grill_system.temp_input_buffer[grill_system.temp_input_index] = key;
            grill_system.temp_input_index++;
            grill_system.temp_input_buffer[grill_system.temp_input_index] = '\0';
            
            // Update state to show we're inputting
            if (grill_system.current_state == STATE_ASK_TEMPERATURE) {
                grill_system.current_state = STATE_INPUTTING_TEMPERATURE;
            }
            
            ESP_LOGI(TAG, "Temperature input: %s", grill_system.temp_input_buffer);
        }
    } else if (key == '#') {
        // Confirm temperature input
        process_temperature_input();
    } else if (key == '*') {
        // Clear input and reset
        memset(grill_system.temp_input_buffer, 0, sizeof(grill_system.temp_input_buffer));
        grill_system.temp_input_index = 0;
        grill_system.current_state = STATE_ASK_TEMPERATURE;
        grill_system.determined_level = NO_DETERMINATION;
        grill_system.input_temperature = -1;
        ESP_LOGI(TAG, "Temperature input cleared");
    }
    
    update_grill_display();
}

/**
 * @brief Process the temperature input and determine meat term
 */
static void process_temperature_input(void)
{
    if (grill_system.temp_input_index > 0) {
        // Convert string to integer
        grill_system.input_temperature = atoi(grill_system.temp_input_buffer);
        
        ESP_LOGI(TAG, "Processing temperature: %d°C", grill_system.input_temperature);
        
        // Always try to determine meat cooking term
        grill_system.determined_level = determine_meat_term_from_temperature(grill_system.input_temperature);
        
        // Check if temperature is in safe range (20-40°C)
        bool in_safe_range = is_temperature_in_safe_range(grill_system.input_temperature);
        
        if (in_safe_range) {
            if (grill_system.determined_level != NO_DETERMINATION) {
                ESP_LOGI(TAG, "Temperature %d°C -> %s (SAFE RANGE)", 
                        grill_system.input_temperature, cooking_names[grill_system.determined_level]);
            } else {
                ESP_LOGI(TAG, "Temperature %d°C in safe range but no specific meat term", 
                        grill_system.input_temperature);
            }
        } else {
            ESP_LOGW(TAG, "Temperature %d°C is OUTSIDE safe range (20-40°C) - WARNING!", 
                    grill_system.input_temperature);
        }
        
        // Always go to showing meat term state
        grill_system.current_state = STATE_SHOWING_MEAT_TERM;
        update_grill_display();
    }
}

/**
 * @brief Handle control keys (status, reset)
 */
static void handle_control_keys(char key)
{
    if (key == '#' && grill_system.current_state == STATE_SHOWING_MEAT_TERM) {
        // Show status for 2 seconds
        grill_system.current_state = STATE_SHOWING_STATUS;
        update_grill_display();
        vTaskDelay(pdMS_TO_TICKS(2000));
        grill_system.current_state = STATE_SHOWING_MEAT_TERM;
        update_grill_display();
        
    } else if (key == '*') {
        // Reset to ask temperature again
        memset(grill_system.temp_input_buffer, 0, sizeof(grill_system.temp_input_buffer));
        grill_system.temp_input_index = 0;
        grill_system.current_state = STATE_ASK_TEMPERATURE;
        grill_system.determined_level = NO_DETERMINATION;
        grill_system.input_temperature = -1;
        grill_system.warning_active = false;
        ESP_LOGI(TAG, "System reset - asking for new temperature");
        update_grill_display();
    }
}



/**
 * @brief Temperature monitoring task (simplified for input-only system)
 */
static void temperature_monitoring_task(void *pvParameters)
{
    TickType_t last_wake_time = xTaskGetTickCount();
    
    while (1) {
        // This task is now simplified since we only care about input temperature
        // Keep reading sensor for future use if needed
        grill_system.sensor_temp = read_temperature_sensor();
        
        // No automatic display updates needed - only input temperature matters
        ESP_LOGD(TAG, "Sensor: %.1f°C, Input: %d°C", 
                grill_system.sensor_temp, grill_system.input_temperature);
        
        // Wait for next measurement
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(TEMP_UPDATE_INTERVAL_MS));
    }
}

/**
 * @brief Application main function - Professional implementation
 */
void app_main(void)
{
    ESP_LOGI(TAG, "ESP32-S3 Hamburger Grill Control System Starting");
    ESP_LOGI(TAG, "Mechatronics Engineering Implementation v2.0");
    
    // Initialize LCD display first
    ESP_LOGI(TAG, "Initializing LCD display...");
    esp_err_t ret = hd44780_init(&lcd);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LCD initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // Initialize temperature sensor (ADC)
    ESP_LOGI(TAG, "Initializing temperature sensor...");
    ret = temperature_sensor_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Temperature sensor initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // Display initial message on LCD
    hd44780_clear(&lcd);
    hd44780_gotoxy(&lcd, 0, 0);
    hd44780_puts(&lcd, "Hello World");
    hd44780_gotoxy(&lcd, 0, 1);
    hd44780_puts(&lcd, "Meca");
    vTaskDelay(pdMS_TO_TICKS(2000)); // Show welcome message for 2 seconds
    
    // Initialize matrix keyboard driver
    ESP_LOGI(TAG, "Initializing matrix keyboard...");
    ret = matrix_keyboard_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Matrix keyboard initialization failed: %s", 
                 esp_err_to_name(ret));
        hd44780_clear(&lcd);
        hd44780_gotoxy(&lcd, 0, 0);
        hd44780_puts(&lcd, "ERROR:");
        hd44780_gotoxy(&lcd, 0, 1);
        hd44780_puts(&lcd, "Keyboard Init");
        return;
    }
    
    // Start temperature monitoring task
    ESP_LOGI(TAG, "Starting temperature monitoring task...");
    BaseType_t task_created = xTaskCreate(
        temperature_monitoring_task,
        "temp_monitor", 
        4096,
        NULL,
        5,
        NULL
    );
    
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create temperature monitoring task");
        return;
    }
    
    // Show initial grill interface
    update_grill_display();
    
    ESP_LOGI(TAG, "Hamburger Grill System Ready!");
    hd44780_puts(&lcd, "Press any key...");
    
    ESP_LOGI(TAG, "System ready - Matrix keyboard and LCD active");
    ESP_LOGI(TAG, "Key mapping: 1-9,0,*,#,A-D");
    
    // Main application loop - Hamburger Grill Control System
    key_event_t key_event;
    
    while (1) {
        // Get key events with 100ms timeout
        ret = matrix_keyboard_get_key(&key_event, 100);
        
        if (ret == ESP_OK) {
            if (key_event.pressed) {
                // Key press event - Enhanced console output
                ESP_LOGI(TAG, "🔑 KEY PRESSED: '%c' at position [%d,%d]", 
                         key_event.key_char, key_event.row, key_event.col);
                
                printf(">>> Key '%c' PRESSED <<<\n", key_event.key_char);
                
                // Handle hamburger grill temperature input and control
                char key = key_event.key_char;
                
                // Process based on current system state
                if (grill_system.current_state == STATE_ASK_TEMPERATURE || 
                    grill_system.current_state == STATE_INPUTTING_TEMPERATURE) {
                    // Handle temperature input (digits, #, *)
                    handle_temperature_input(key);
                    ESP_LOGI(TAG, "Temperature input key: %c", key);
                    
                } else if (grill_system.current_state == STATE_SHOWING_MEAT_TERM) {
                    // Handle control commands when showing meat term
                    if (key == '#' || key == '*') {
                        handle_control_keys(key);
                        ESP_LOGI(TAG, "Control key processed: %c", key);
                    } else {
                        ESP_LOGW(TAG, "Invalid key '%c' - use # for status, * for reset", key);
                    }
                    
                } else if (key >= 'A' && key <= 'D') {
                    // Additional functions (future expansion)
                    hd44780_clear(&lcd);
                    hd44780_gotoxy(&lcd, 0, 0);
                    char func_msg[20];
                    snprintf(func_msg, sizeof(func_msg), "Function %c", key);
                    hd44780_puts(&lcd, func_msg);
                    hd44780_gotoxy(&lcd, 0, 1);
                    hd44780_puts(&lcd, "Not Available");
                    ESP_LOGI(TAG, "Function %c pressed (not implemented)", key);
                    vTaskDelay(pdMS_TO_TICKS(1500));
                    update_grill_display(); // Return to normal display
                    
                } else {
                    // Invalid key for current state
                    ESP_LOGW(TAG, "Invalid key '%c' for current state", key);
                }
            }
        } else if (ret == ESP_ERR_TIMEOUT) {
            // No key events - system idle, temperature monitoring continues
        }
        
        // Small delay to prevent excessive CPU usage
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}