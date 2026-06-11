# Solar battery
Solar powered battery project made from recycled parts.

## Hardware
Most of the hardware for this project has been dumpster dived at my university.
The hardware consists of four HR 1234W 12V lead acid batteries connected in parallel.
These batteries are charged with a Victron SmartSolar 75/15 MPPT charge controller.
The solar panel is a 30W portable generic model like [this](https://www.biltema.dk/en-dk/car---mc/electrical-system/solar-panels/portable-solar-panel-30-w-2000045321).
The builtin PWM regulator was broken so I took it off and connected the panel to the MPPT.
An ESP32 is connected via UART to the MPPT charge controller.
The MPPT periodically writes data on UART which the ESP32 reads and display on an LCD.
The LCD is a old NHD-0420E2Z-NSW-BBW.
It looks really cool but is ancient technology at this point.
I wants 5V for both power and the logic however the ESP32 only has 3V3 logic outputs.
From my testing so far the LCD works fine even with 3V3 logic.
I was not able to power the display using 3V3 even though the LCDs driver chip ST7066U supports it.
I could not find any ESP-IDF library for the ST7066U however the chip is compatible the HD44780.
I managed to find a [library](https://github.com/esp-idf-lib/hd44780) for the HD44780 which works well.

## Firmware
ESP32 firmware for fetching data from Victron MPPT and displaying it on ST7066U LCD.
You need ESP-IDF to compile and flash the firmware.
I use Nix to provide a development environment with ESP-IDF:
```shell
nix develop
```
You can build the firmware like this:
```shell
idf.py build
```
Flash it like this:
```shell
idf.py flash
```
Monitor the serial outout:
```shell
idf.py monitor
```
Or all in one go:
```shell
idf.py build flash monitor
```

