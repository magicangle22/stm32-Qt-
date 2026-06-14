# STM32 + Qt OLED Image Sender

STM32F103C8 + Qt6 OLED image/video sender with 30fps streaming via HW I2C1 + DMA.

## Hardware
- **MCU**: STM32F103C8T6 (Blue Pill, 72MHz)
- **Display**: SSD1306 128x64 OLED (I2C)
- **UART**: CH340 USB-UART, 921600 bps

### Wiring

| OLED  | STM32 |
|-------|-------|
| SCL   | PB8   |
| SDA   | PB9   |
| VCC   | 3.3V  |
| GND   | GND   |

| CH340 | STM32     |
|-------|-----------|
| TXD   | PA10 (RX) |
| RXD   | PA9  (TX) |

## Features
- **HW I2C1 + DMA**: I2C1 remapped to PB8/PB9, DMA1_Channel6 transfers full 1024-byte frame in one shot
- **921600 baud**: 11ms per frame over UART
- **30fps streaming**: Qt app opens video files and streams frames to OLED in real time
- **Image processing**: threshold binarization, invert color, adjustable FPS (1-60)
- **OLED preview**: Qt side shows live preview of what the OLED displays

## Structure
`
stm32/                  # STM32 Keil MDK project
  Hardware/             # Drivers (OLED, Serial, Key, LED, Buzzer)
  Library/              # STM32F10x Standard Peripheral Library
  Start/                # Startup files
  System/               # Delay
  User/                 # main.c, interrupts, config
qt/                     # Qt6 CMake project
  main.cpp
  mainwindow.h
  mainwindow.cpp
  CMakeLists.txt
`

## Build

### STM32 Firmware
1. Open stm32/project.uvprojx in Keil MDK
2. Build and flash to STM32F103C8

### Qt Desktop App
`ash
cd qt
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/Qt6
cmake --build build
./build/appqt
`
Or open qt/CMakeLists.txt directly in Qt Creator.

## Usage
1. Power on STM32, OLED shows Key1:SendImg
2. In Qt, select COM port and baud rate (default 921600)
3. Click **Open Image** or **Open Video**
4. Click **Send to STM32** (image) or **Start Video Stream** (video)
5. Image appears on OLED

## Performance
| Metric | Value |
|--------|-------|
| UART   | 921600 bps |
| I2C    | 400 kHz Fast Mode |
| UART/frame | ~11 ms |
| I2C+DMA/frame | ~25 ms |
| Max FPS | ~30 fps |

## License
MIT
