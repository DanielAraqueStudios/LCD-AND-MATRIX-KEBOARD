# 🌡️ ESP32-S3 Hamburger Grill - Temperature Input System

## 🍔 New Operation Mode: Temperature-Based Meat Term Determination

### Overview
This updated system implements a **temperature-first approach** where:
1. **User inputs the current temperature** via the keypad
2. **System automatically determines** the meat cooking term based on that temperature
3. **Displays the result** and monitors sensor for safety warnings

### 🎯 How It Works

#### Step 1: Temperature Input Request
- **LCD displays**: `"Enter Temp (C):"`
- **Second line**: `"Use 0-9, # OK"`
- **User action**: Type temperature using number keys (0-9)

#### Step 2: Temperature Entry
- **LCD displays**: `"Temperature:"`
- **Second line**: Shows entered digits, e.g., `"25 C (# to OK)"`
- **User action**: Continue typing or press `#` to confirm

#### Step 3: Automatic Meat Term Determination
- **System calculates** which cooking level matches the input temperature:
  - **20-25°C** → **BLUE RARE**
  - **26-30°C** → **MEDIUM RARE** 
  - **31-35°C** → **WELL DONE**
  - **36-40°C** → **SOLE RARE**
  - **Other temps** → **Invalid Temp**

#### Step 4: Display Results & Monitor
- **LCD Row 1**: Shows determined meat term (e.g., `"MEDIUM RARE"`)
- **LCD Row 2**: Shows sensor vs input comparison or warning
  - **Normal**: `"T:28.5C (OK 27C)"` (sensor temp vs input temp)
  - **Warning**: `"OH! OH! BE CAREFUL"` (sensor out of range)

### 🎮 Control Interface

#### Keypad Controls

| Key | Function | When Available | Description |
|-----|----------|----------------|-------------|
| **0-9** | Input digits | Temperature entry | Type the temperature value |
| **#** | Confirm/Status | Always | Confirm temp input OR show status |
| ***** | Reset/Clear | Always | Clear input and start over |
| **A-D** | Reserved | Always | Show "Not Available" message |

#### System States

| State | LCD Row 1 | LCD Row 2 | User Action |
|-------|-----------|-----------|-------------|
| **Ask Temperature** | `"Enter Temp (C):"` | `"Use 0-9, # OK"` | Type temperature |
| **Inputting Temperature** | `"Temperature:"` | `"25 C (# to OK)"` | Continue typing or confirm |
| **Showing Meat Term** | `"MEDIUM RARE"` | `"T:28.5C (OK 25C)"` | Monitor or reset |
| **Showing Status** | `"Status Check:"` | `"In:25C Sen:28.5C"` | Temporary (2 seconds) |

### 🔬 Technical Implementation

#### Temperature Determination Logic
```c
// Automatic determination based on input temperature
if (temperature >= 20 && temperature <= 25) {
    return BLUE_RARE;
} else if (temperature >= 26 && temperature <= 30) {
    return MEDIUM_RARE;
} else if (temperature >= 31 && temperature <= 35) {
    return WELL_DONE;
} else if (temperature >= 36 && temperature <= 40) {
    return SOLE_RARE;
} else {
    return NO_DETERMINATION; // Invalid temperature
}
```

#### Safety Monitoring
- **Continuous sensor reading**: Every 500ms from potentiometer
- **Range validation**: Compares sensor temperature with determined meat term range
- **Warning system**: Shows "OH! OH! BE CAREFUL" when sensor is out of range
- **Status display**: Press `#` to see input vs sensor temperature comparison

### 🚀 Usage Examples

#### Example 1: Perfect Match
1. **Input**: User types `28` and presses `#`
2. **Determination**: System shows `"MEDIUM RARE"` (26-30°C range)
3. **Sensor**: Potentiometer reads 29.2°C
4. **Display**: `"T:29.2C (OK 28C)"` ✅

#### Example 2: Out of Range Warning
1. **Input**: User types `27` and presses `#`
2. **Determination**: System shows `"MEDIUM RARE"` (26-30°C range)
3. **Sensor**: Potentiometer reads 35.5°C (too hot!)
4. **Display**: `"OH! OH! BE CAREFUL"` ⚠️

#### Example 3: Invalid Temperature
1. **Input**: User types `15` and presses `#`
2. **Determination**: System shows `"Invalid Temp"`
3. **Display**: `"Try again (* rst)"` 
4. **Action**: User presses `*` to reset and try again

### 🔧 Key Features

#### Professional Advantages
- **Intuitive operation**: Temperature-first approach matches real-world cooking
- **Automatic classification**: No need to memorize cooking level numbers
- **Safety monitoring**: Continuous validation of actual vs target temperature
- **Error recovery**: Easy reset and retry functionality
- **Professional feedback**: Clear status displays and warnings

#### Error Handling
- **Invalid input**: Handles temperatures outside cooking ranges
- **Input clearing**: `*` key clears partial input
- **State management**: Robust state machine prevents confusion
- **Visual feedback**: Clear indication of current system state

### 📊 Temperature Ranges

| Meat Term | Range (°C) | Input Examples | Description |
|-----------|------------|----------------|-------------|
| **BLUE RARE** | 20-25°C | `20`, `22`, `25` | Very rare, cool center |
| **MEDIUM RARE** | 26-30°C | `26`, `28`, `30` | Warm center, pink throughout |
| **WELL DONE** | 31-35°C | `31`, `33`, `35` | No pink, fully cooked |
| **SOLE RARE** | 36-40°C | `36`, `38`, `40` | Custom rare level |
| **Invalid** | <20 or >40°C | `15`, `45`, `99` | Outside cooking ranges |

### 🎯 Real-World Application

This system mimics professional kitchen operations where:
1. **Chef measures** current grill/meat temperature with thermometer
2. **System determines** if temperature matches desired cooking level
3. **Continuous monitoring** ensures food safety and quality
4. **Immediate alerts** prevent overcooking or undercooking

Perfect for culinary education, professional kitchens, and automated food preparation systems!

### 🔄 Operation Flow

```
┌─────────────────┐
│ System Startup  │
└─────────┬───────┘
          │
          ▼
┌─────────────────┐    0-9 keys    ┌──────────────────┐
│ Ask Temperature │ ───────────────▶│ Inputting Temp   │
│ "Enter Temp(C)" │                 │ "25 C (# to OK)" │
└─────────────────┘                 └─────────┬────────┘
          ▲                                   │ # key
          │ * key (reset)                     ▼
          │                         ┌─────────────────┐
          │                         │ Determine Term  │
          │                         │ (Automatic)     │
          │                         └─────────┬───────┘
          │                                   │
          │                                   ▼
          │                         ┌─────────────────┐
          └─────────────────────────│ Show Meat Term  │
                                    │ + Monitor Temp  │
                                    └─────────────────┘
                                            │ # key (status)
                                            ▼
                                    ┌─────────────────┐
                                    │ Show Status     │
                                    │ (2 seconds)     │
                                    └─────────────────┘
```
