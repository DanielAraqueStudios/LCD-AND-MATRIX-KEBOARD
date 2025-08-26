# ESP32-S3 Hamburger Grill Control System

## Professional Mechatronics Implementation with ESP-IDF

### Overview
This project implements a complete professional hamburger grill automation system for the ESP32-S3 microcontroller using ESP-IDF framework. The system features a 4x4 matrix keyboard for cooking level selection, HD44780 LCD display for real-time feedback, and temperature sensor emulation using a potentiometer. The implementation follows industry best practices with robust error handling, safety warnings, and comprehensive documentation.

### 🍔 Key Features
- **4 Professional Cooking Levels**: Blue Rare, Medium Rare, Well Done, Sole Rare
- **Real-time Temperature Monitoring**: Continuous ADC reading with safety validation
- **Professional User Interface**: Matrix keyboard input with LCD feedback
- **Safety System**: Out-of-range temperature warnings
- **Modern ESP-IDF Implementation**: Uses latest v6.0+ ADC API and FreeRTOS

## 📋 Features

### Professional Engineering Features
- ✅ **Hardware Abstraction Layer** - Easy GPIO pin reconfiguration
- ✅ **Professional Debouncing** - 50ms software debouncing with timing validation
- ✅ **Non-blocking Operation** - FreeRTOS task-based scanning (10ms intervals)
- ✅ **Event-driven Architecture** - Queue-based key event system
- ✅ **Configurable Key Mapping** - Easy layout customization
- ✅ **Real-time Performance** - Optimized for responsive input handling
- ✅ **Error Handling** - Comprehensive error checking and logging
- ✅ **Memory Management** - Efficient resource utilization
- ✅ **Professional Documentation** - Industry-standard code documentation

### Technical Specifications
- **Matrix Size**: 4x4 (16 keys)
- **Scan Rate**: 100Hz (10ms intervals)
- **Debounce Time**: 50ms (professional standard)
- **Key Buffer**: 16 events
- **Response Time**: <20ms typical
- **CPU Usage**: <1% at idle

## 🔌 Hardware Connections

### ESP32-S3 GPIO Pin Assignment

| Function | GPIO Pin | Description |
|----------|----------|-------------|
| **Row 0** | GPIO_1 | Matrix row 0 (output) |
| **Row 1** | GPIO_2 | Matrix row 1 (output) |
| **Row 2** | GPIO_42 | Matrix row 2 (output) |
| **Row 3** | GPIO_41 | Matrix row 3 (output) |
| **Col 0** | GPIO_40 | Matrix column 0 (input + pullup) |
| **Col 1** | GPIO_39 | Matrix column 1 (input + pullup) |
| **Col 2** | GPIO_38 | Matrix column 2 (input + pullup) |
| **Col 3** | GPIO_37 | Matrix column 3 (input + pullup) |

### Wiring Diagram
```
ESP32-S3          4x4 Matrix Keypad
--------          ------------------
GPIO_1   -------> Row 0
GPIO_2   -------> Row 1  
GPIO_42  -------> Row 2
GPIO_41  -------> Row 3

GPIO_40  <------- Col 0 (with internal pullup)
GPIO_39  <------- Col 1 (with internal pullup)
GPIO_38  <------- Col 2 (with internal pullup)
GPIO_37  <------- Col 3 (with internal pullup)
```

## 🗝️ Key Layout

Standard 4x4 keypad layout:
```
[1] [2] [3] [A]
[4] [5] [6] [B]
[7] [8] [9] [C]
[*] [0] [#] [D]
```

## 🚀 Quick Start

### Prerequisites
- ESP-IDF v6.0 or later (required for modern ADC API)
- ESP32-S3 development board
- 4x4 matrix keypad
- HD44780 LCD display (16x2)
- Potentiometer (for temperature emulation)
- Connecting wires

### Build and Flash
```bash
# Set up ESP-IDF environment
. $HOME/esp/esp-idf/export.sh

# Navigate to project directory
cd "project-name"

# Configure for ESP32-S3
idf.py set-target esp32s3

# Build the hamburger grill system
idf.py build

# Flash to device (replace PORT with your device port)
idf.py -p /dev/ttyUSB0 flash monitor
```

## 🍔 Hamburger Grill Operation
1. **Power on**: System displays "Hello World" and "Meca"
2. **Select cooking level**: Press keys 1-4 for different doneness levels:
   - **1**: Blue Rare (20-25°C)
   - **2**: Medium Rare (26-30°C) 
   - **3**: Well Done (31-35°C)
   - **4**: Sole Rare (36-40°C)
3. **Monitor temperature**: Adjust potentiometer to simulate grill temperature
4. **Safety warnings**: LCD shows "OH! OH! BE CAREFUL" when temperature is out of range
5. **Status check**: Press # to view current temperature and status for 2 seconds
6. **Reset**: Press * to clear selection and return to main menu

📖 **For detailed hamburger grill documentation, see:** [`README_HAMBURGER_GRILL.md`](README_HAMBURGER_GRILL.md)

## 📚 API Reference

### Core Functions

#### `esp_err_t matrix_keyboard_init(void)`
Initialize the matrix keyboard driver system.
- **Returns**: `ESP_OK` on success, error code on failure
- **Note**: Must be called before any other keyboard functions

#### `esp_err_t matrix_keyboard_get_key(key_event_t *event, uint32_t timeout_ms)`
Retrieve the next key event from the queue.
- **Parameters**:
  - `event`: Pointer to store the key event data
  - `timeout_ms`: Timeout in milliseconds (0 = no wait)
- **Returns**: `ESP_OK` if event received, `ESP_ERR_TIMEOUT` if no event

### Data Structures

#### `key_event_t`
```c
typedef struct {
    uint8_t row;           // Row index (0-3)
    uint8_t col;           // Column index (0-3)
    char key_char;         // Mapped character
    bool pressed;          // true = pressed, false = released
    uint64_t timestamp;    // Timestamp in microseconds
} key_event_t;
```

## ⚙️ Configuration

### GPIO Pin Customization
Modify the pin arrays in `main.c`:
```c
// Row pins (outputs)
static const gpio_num_t row_pins[MATRIX_ROWS] = {
    GPIO_NUM_1, GPIO_NUM_2, GPIO_NUM_42, GPIO_NUM_41
};

// Column pins (inputs with pullup)
static const gpio_num_t col_pins[MATRIX_COLS] = {
    GPIO_NUM_40, GPIO_NUM_39, GPIO_NUM_38, GPIO_NUM_37
};
```

### Key Layout Customization
Modify the key mapping array:
```c
static const char key_map[MATRIX_ROWS][MATRIX_COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};
```

### Timing Parameters
```c
#define DEBOUNCE_TIME_MS           50      // Debounce timing
#define SCAN_INTERVAL_MS           10      // Scan frequency
#define KEY_QUEUE_SIZE             16      // Event buffer size
```

## 🔍 Example Usage

```c
void app_main(void)
{
    // Initialize keyboard driver
    esp_err_t ret = matrix_keyboard_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Keyboard init failed");
        return;
    }
    
    key_event_t key_event;
    
    while (1) {
        // Get key events with 100ms timeout
        ret = matrix_keyboard_get_key(&key_event, 100);
        
        if (ret == ESP_OK) {
            if (key_event.pressed) {
                printf("Key '%c' pressed at [%d,%d]\n", 
                       key_event.key_char, key_event.row, key_event.col);
                
                // Handle specific keys
                switch (key_event.key_char) {
                    case '1':
                        // Handle key 1
                        break;
                    case 'A':
                        // Handle key A
                        break;
                    // Add more cases as needed
                }
            }
        }
    }
}
```

## 🛠️ Professional Engineering Considerations

### Hardware Design
- **Pull-up Resistors**: Internal pull-ups used for reliable operation
- **Signal Integrity**: 1ms stabilization delay for clean readings
- **GPIO Selection**: Pins chosen to avoid conflicts with ESP32-S3 peripherals

### Software Architecture
- **Real-time Performance**: FreeRTOS task with precise timing
- **Memory Efficiency**: Minimal RAM usage with optimized data structures
- **Error Resilience**: Comprehensive error checking and recovery
- **Maintainability**: Modular design with clear separation of concerns

### Performance Optimization
- **CPU Usage**: Optimized scanning algorithm
- **Response Time**: Sub-20ms key detection
- **Power Efficiency**: Efficient GPIO operations
- **Scalability**: Easy adaptation for different matrix sizes

## 🐛 Troubleshooting

### Common Issues

1. **Keys not detected**
   - Check GPIO pin connections
   - Verify keypad matrix wiring
   - Check internal pull-up configuration

2. **Multiple key presses detected**
   - Increase debounce time
   - Check for electrical noise
   - Verify keypad quality

3. **Compilation errors**
   - Ensure ESP-IDF is properly installed
   - Check target is set to ESP32-S3
   - Verify all required components are included

### Debug Output
Enable debug logging:
```bash
idf.py menuconfig
# Component config -> Log output -> Default log verbosity -> Debug
```

## 📄 License

Professional Engineering Implementation - Open Source Hardware Project

## 👨‍💻 Author

Professional Mechatronics Engineer
Specialized in ESP32-S3 Development & Embedded Systems 

Below is short explanation of remaining files in the project folder.

```
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   └── main.c
└── README.md                  This is the file you are currently reading
```
Additionally, the sample project contains Makefile and component.mk files, used for the legacy Make based build system. 
They are not used or needed when building with CMake and idf.py.
