/**
 * @file matrix_keyboard.h
 * @brief Professional ESP32-S3 4x4 Matrix Keyboard Driver Header
 * @author Mechatronics Engineer
 * @date August 2025
 * 
 * Professional-grade matrix keyboard driver for ESP32-S3 with industry-standard
 * features including hardware abstraction, debouncing, and real-time performance.
 * 
 * @copyright Professional Engineering Implementation - Open Source Hardware Project
 */

#ifndef MATRIX_KEYBOARD_H
#define MATRIX_KEYBOARD_H

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== INCLUDES ==================== */
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

/* ==================== CONFIGURATION CONSTANTS ==================== */

/** @brief Number of rows in the matrix keyboard */
#define MATRIX_ROWS                 4

/** @brief Number of columns in the matrix keyboard */
#define MATRIX_COLS                 4

/** @brief Debounce time in milliseconds (professional standard) */
#define DEBOUNCE_TIME_MS           50

/** @brief Keyboard scanning interval in milliseconds */
#define SCAN_INTERVAL_MS           10

/** @brief Maximum number of key events in the queue */
#define KEY_QUEUE_SIZE             16

/* ==================== DATA TYPES ==================== */

/**
 * @brief Key event structure for professional event handling
 * 
 * This structure contains comprehensive information about each key event,
 * enabling sophisticated input handling and system diagnostics.
 */
typedef struct {
    uint8_t row;           /**< Row index (0 to MATRIX_ROWS-1) */
    uint8_t col;           /**< Column index (0 to MATRIX_COLS-1) */
    char key_char;         /**< Mapped character for the key */
    bool pressed;          /**< true = key pressed, false = key released */
    uint64_t timestamp;    /**< Event timestamp in microseconds */
} key_event_t;

/**
 * @brief Matrix keyboard configuration structure
 * 
 * Professional configuration structure for customizing keyboard behavior
 * and hardware assignments.
 */
typedef struct {
    const int *row_pins;     /**< Array of GPIO pins for rows */
    const int *col_pins;     /**< Array of GPIO pins for columns */
    const char (*key_map)[MATRIX_COLS]; /**< Key character mapping */
    uint32_t debounce_ms;    /**< Debounce time in milliseconds */
    uint32_t scan_interval_ms; /**< Scan interval in milliseconds */
} matrix_keyboard_config_t;

/* ==================== FUNCTION PROTOTYPES ==================== */

/**
 * @brief Initialize the matrix keyboard driver system
 * 
 * This function initializes all necessary components for matrix keyboard operation:
 * - GPIO configuration for rows and columns
 * - FreeRTOS task creation for scanning
 * - Event queue creation
 * - Internal state initialization
 * 
 * @return ESP_OK on successful initialization
 * @return ESP_ERR_NO_MEM if memory allocation fails
 * @return ESP_ERR_INVALID_ARG if configuration is invalid
 * @return Other ESP error codes for GPIO or system failures
 * 
 * @note This function must be called before any other keyboard functions
 * @note The function is thread-safe and can be called multiple times
 */
esp_err_t matrix_keyboard_init(void);

/**
 * @brief Initialize matrix keyboard with custom configuration
 * 
 * Advanced initialization function allowing custom GPIO pins and timing parameters.
 * 
 * @param config Pointer to configuration structure
 * @return ESP_OK on successful initialization
 * @return ESP_ERR_INVALID_ARG if config is NULL or invalid
 * @return ESP_ERR_NO_MEM if memory allocation fails
 * 
 * @note For standard operation, use matrix_keyboard_init() instead
 */
esp_err_t matrix_keyboard_init_with_config(const matrix_keyboard_config_t *config);

/**
 * @brief Get the next key event from the event queue
 * 
 * This function retrieves key events in a non-blocking or blocking manner
 * depending on the timeout parameter. Professional event handling with
 * comprehensive error reporting.
 * 
 * @param event Pointer to store the retrieved key event
 * @param timeout_ms Timeout in milliseconds (0 = non-blocking, portMAX_DELAY = blocking)
 * 
 * @return ESP_OK if a key event was successfully retrieved
 * @return ESP_ERR_TIMEOUT if no event was available within the timeout period
 * @return ESP_ERR_INVALID_ARG if event pointer is NULL
 * @return ESP_ERR_INVALID_STATE if keyboard driver is not initialized
 * 
 * @note This function is thread-safe and can be called from multiple tasks
 * @note The function handles both key press and release events
 */
esp_err_t matrix_keyboard_get_key(key_event_t *event, uint32_t timeout_ms);

/**
 * @brief Check if the keyboard driver is initialized and operational
 * 
 * @return true if keyboard is initialized and ready
 * @return false if keyboard is not initialized or has errors
 */
bool matrix_keyboard_is_initialized(void);

/**
 * @brief Get the current number of events in the queue
 * 
 * Diagnostic function for monitoring keyboard event queue status.
 * 
 * @return Number of events currently in the queue (0 to KEY_QUEUE_SIZE)
 * @return -1 if keyboard driver is not initialized
 */
int matrix_keyboard_get_queue_count(void);

/**
 * @brief Deinitialize the matrix keyboard driver
 * 
 * Clean shutdown of the keyboard driver, freeing all allocated resources.
 * 
 * @return ESP_OK on successful deinitialization
 * @return ESP_ERR_INVALID_STATE if driver was not initialized
 * 
 * @note After calling this function, matrix_keyboard_init() must be called
 *       again before using any keyboard functions
 */
esp_err_t matrix_keyboard_deinit(void);

/**
 * @brief Get keyboard driver version information
 * 
 * @return Pointer to version string (e.g., "1.0.0")
 */
const char* matrix_keyboard_get_version(void);

/* ==================== ADVANCED CONFIGURATION ==================== */

/**
 * @brief Set custom debounce time
 * 
 * @param debounce_ms New debounce time in milliseconds (10-200ms recommended)
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if debounce time is out of range
 */
esp_err_t matrix_keyboard_set_debounce_time(uint32_t debounce_ms);

/**
 * @brief Set custom scan interval
 * 
 * @param interval_ms New scan interval in milliseconds (5-50ms recommended)
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if interval is out of range
 */
esp_err_t matrix_keyboard_set_scan_interval(uint32_t interval_ms);

/* ==================== DIAGNOSTIC FUNCTIONS ==================== */

/**
 * @brief Get keyboard driver statistics
 */
typedef struct {
    uint32_t total_key_presses;    /**< Total key presses since initialization */
    uint32_t total_key_releases;   /**< Total key releases since initialization */
    uint32_t queue_overflows;      /**< Number of queue overflow events */
    uint32_t debounce_rejections;  /**< Number of events rejected due to debouncing */
    uint64_t uptime_us;           /**< Driver uptime in microseconds */
} matrix_keyboard_stats_t;

/**
 * @brief Get driver statistics for diagnostics
 * 
 * @param stats Pointer to store statistics
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if stats pointer is NULL
 * @return ESP_ERR_INVALID_STATE if driver not initialized
 */
esp_err_t matrix_keyboard_get_stats(matrix_keyboard_stats_t *stats);

/**
 * @brief Reset driver statistics
 * 
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_STATE if driver not initialized
 */
esp_err_t matrix_keyboard_reset_stats(void);

#ifdef __cplusplus
}
#endif

#endif /* MATRIX_KEYBOARD_H */
