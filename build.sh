#!/bin/bash

###############################################################################
# ESP32-S3 Matrix Keyboard Professional Build Script
# Author: Mechatronics Engineer
# Date: August 2025
# 
# Professional build automation for ESP32-S3 matrix keyboard project
# with comprehensive error handling and status reporting.
###############################################################################

set -e  # Exit on any error

# Color codes for professional output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Project configuration
PROJECT_NAME="ESP32-S3 Matrix Keyboard"
TARGET_CHIP="esp32s3"
DEFAULT_PORT="/dev/ttyUSB0"

# Function definitions
print_header() {
    echo -e "${CYAN}===============================================================================${NC}"
    echo -e "${CYAN}  $PROJECT_NAME - Professional Build System${NC}"
    echo -e "${CYAN}  Mechatronics Engineering Implementation${NC}"
    echo -e "${CYAN}===============================================================================${NC}"
}

print_status() {
    echo -e "${BLUE}[STATUS]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_esp_idf() {
    print_status "Checking ESP-IDF environment..."
    
    if [ -z "$IDF_PATH" ]; then
        print_error "ESP-IDF environment not found!"
        print_status "Please run: . \$HOME/esp/esp-idf/export.sh"
        exit 1
    fi
    
    print_success "ESP-IDF found at: $IDF_PATH"
    
    # Check IDF version
    IDF_VERSION=$(idf.py --version 2>/dev/null | grep -oP 'v\d+\.\d+\.\d+' || echo "unknown")
    print_status "ESP-IDF Version: $IDF_VERSION"
}

set_target() {
    print_status "Setting target to $TARGET_CHIP..."
    idf.py set-target $TARGET_CHIP
    print_success "Target set to $TARGET_CHIP"
}

configure_project() {
    print_status "Opening project configuration menu..."
    print_warning "Configure the following if needed:"
    echo "  - Component config -> Log output -> Default log verbosity -> Debug"
    echo "  - Serial flasher config -> Default serial port"
    echo "  - Component config -> ESP32S3-Specific -> Support for external RAM"
    
    read -p "Open menuconfig? (y/N): " choice
    case "$choice" in 
        y|Y ) 
            idf.py menuconfig
            print_success "Configuration updated"
            ;;
        * ) 
            print_status "Skipping configuration"
            ;;
    esac
}

build_project() {
    print_status "Building project..."
    print_status "This may take several minutes for the first build..."
    
    if idf.py build; then
        print_success "Build completed successfully!"
        
        # Show build statistics
        if [ -f "build/project_description.json" ]; then
            print_status "Build Statistics:"
            echo "  - Binary size: $(ls -lh build/*.bin 2>/dev/null | awk '{print $5}' | head -1)"
            echo "  - Build directory: $(du -sh build 2>/dev/null | cut -f1)"
        fi
    else
        print_error "Build failed!"
        exit 1
    fi
}

flash_project() {
    local port=${1:-$DEFAULT_PORT}
    
    print_status "Flashing to device on port $port..."
    
    # Check if device is connected
    if [ ! -e "$port" ]; then
        print_warning "Device not found on $port"
        echo "Available devices:"
        ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || echo "  No serial devices found"
        
        read -p "Enter port (or press Enter to skip flashing): " custom_port
        if [ -n "$custom_port" ]; then
            port=$custom_port
        else
            print_status "Skipping flash operation"
            return 0
        fi
    fi
    
    if idf.py -p $port flash; then
        print_success "Flashing completed successfully!"
    else
        print_error "Flashing failed!"
        exit 1
    fi
}

monitor_device() {
    local port=${1:-$DEFAULT_PORT}
    
    print_status "Starting serial monitor on $port..."
    print_status "Press Ctrl+] to exit monitor"
    print_status "Press EN button on ESP32-S3 to restart"
    
    echo -e "${PURPLE}=== Serial Monitor Output ===${NC}"
    idf.py -p $port monitor
}

clean_project() {
    print_status "Cleaning build artifacts..."
    
    if [ -d "build" ]; then
        rm -rf build
        print_success "Build directory cleaned"
    else
        print_status "No build artifacts to clean"
    fi
}

show_help() {
    echo "Usage: $0 [OPTION] [PORT]"
    echo ""
    echo "Professional build script for ESP32-S3 Matrix Keyboard project"
    echo ""
    echo "Options:"
    echo "  all                 Complete build process (configure, build, flash, monitor)"
    echo "  setup              Check environment and set target"
    echo "  config             Open project configuration menu"
    echo "  build              Build project only"
    echo "  flash [PORT]       Flash to device (default: $DEFAULT_PORT)"
    echo "  monitor [PORT]     Start serial monitor (default: $DEFAULT_PORT)"
    echo "  clean              Clean build artifacts"
    echo "  help               Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 all                    # Complete build and flash process"
    echo "  $0 flash /dev/ttyUSB1     # Flash to specific port"
    echo "  $0 build                  # Build only"
    echo "  $0 monitor /dev/ttyACM0   # Monitor specific port"
}

# Main script logic
print_header

case "${1:-all}" in
    "all")
        check_esp_idf
        set_target
        build_project
        flash_project $2
        monitor_device $2
        ;;
    "setup")
        check_esp_idf
        set_target
        print_success "Setup completed"
        ;;
    "config")
        check_esp_idf
        configure_project
        ;;
    "build")
        check_esp_idf
        build_project
        ;;
    "flash")
        check_esp_idf
        flash_project $2
        ;;
    "monitor")
        monitor_device $2
        ;;
    "clean")
        clean_project
        ;;
    "help")
        show_help
        ;;
    *)
        print_error "Unknown option: $1"
        show_help
        exit 1
        ;;
esac

print_success "Operation completed successfully!"
