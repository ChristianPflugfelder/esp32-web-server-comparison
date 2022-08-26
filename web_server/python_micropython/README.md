# Python TCP server

Simple Python web server for the ESP32 using MicroPython.

## Setup

- Install IDF
- Set up Micropython [Guide](https://github.com/micropython/micropython/tree/master/ports/esp32)
    * Clone [repo](https://github.com/micropython/micropython)
    * `git submodule update --init`
    * go to `micropython/ports/esp32`
        * run `idf.py set-target esp32`
        * run `idf.py build` / `flash`


## Programming
* As ide one can use [Thonny](https://thonny.org/)
    * Change settings for ESP32: `Tools > Options > Interpreter > Interpeter = MicroPython (ESP32) & Port = COM6`
* edit Wlan SSID and password in `main/boot.py`
* store `main/boot.py` on esp32
    * `boot.py` is executed at every startup
* run `main/server.py` on esp32
    * The esp32 is now running an HTTP server, you can send requests on port 80 


## Use more than 16 TCP connections

* Modify menu config diaglog so that more than 16 sockets can be opened at the same time
    * `cd ~/esp-idf/components/lwip`
    * `edit Kconfig`
    * `LWIP_MAX_SOCKETS` set `range 1 16` to 256 + adjust default and help
*  In the Project you can set the number of sockets via `idf.py menuconfig`
    * go to `micropython/ports/esp32`
    * `idf.py menuconfig` > Component config > LWIP > Max number of open sockets
    * `idf.py menuconfig` > Component config > LWIP > TCP > Maximum active TCP Connections & Maximum listening TCP Connections
* The number of file descriptors must also be increased to be able to use more sockets.
    * `edit micropython/ports/esp32/CMakeLists.txt` 
    * add Line `idf_build_set_property(COMPILE_OPTIONS "-DFD_SETSIZE=256" APPEND)`

## Build with SPI-RAM / psRAM

* add `-D MICROPY_BOARD=GENERIC_SPIRAM` to flash / build
* or instert `set(MICROPY_BOARD GENERIC_SPIRAM)` before the if statemen in `micropython/ports/esp32/CMakeLists.txt`
* after the board is changed one must set the values for TCP conenctions again

<br>

* In case of error `Build FAILED: IRAM0 segment data does not fit`
    * Can possibly be fixed by seting silicon revision to 3 [source](https://github.com/espressif/esp-idf/issues/4682)
    * if the output of hello-world example contains "silicon revision 3"
    * `idf.py menuconfig` > Component config > ESP32-specific > Minimum Supported ESP32 Revsion > Rev 3
