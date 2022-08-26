# Lua TCP server

Simple Lua web server for the ESP32 using NodeMCU.

Prebuild firmware for ESP32 is included in the [bin](web_server/) folder. The max number of TCP connections is set to 252 and the socket idle timeout is set to 7200000 ms.

## Flashing
run `esptool.py --chip esp32 --port COM3 --baud 115200 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0x1000 ./bootloader.bin 0x10000 ./NodeMCU.bin 0x8000 ./partitions.bin` in the bin folder (COM3 must be replaced by the proper port of the ESP32.)

## Programming
* For ESPlorer download zip from: https://github.com/4refr0nt/ESPlorer/releases and run ESPlorer.jar
* select COM port of ESP32
* select baud 115200
* click open
* open server.lua
    * edit Wi-Fi credentials
* click save on esp32

## Build firmware manually

### Setup

* `git clone --branch dev-esp32 --recurse-submodules https://github.com/nodemcu/nodemcu-firmware.git nodemcu-firmware-esp32`
* Install Prerequisites
    *   ```
            sudo apt-get update
            sudo apt-get install -y gperf flex bison build-essential libssl-dev libffi-dev libncurses5-dev libncursesw5-dev libreadline-dev
            sudo apt-get install python3 python3-pip python-is-python3
        ```
    *   ```
            cd nodemcu-firmware-esp32
            python -m pip install -r ./sdk/esp32-esp-idf/requirements.txt
        ```
* Build
    * Use `make menuconfig` and `make`
    * Copy: 
        * `/nodemcu-firmware-esp32/build/bootloader/bootloader.bin`
        * `/nodemcu-firmware-esp32/build/NodeMCU.bin`
        * `/nodemcu-firmware-esp32/build/partitions.bin`


### Increase number of connections

- modify IDF so that more than 16 sockets can be set in projects sdkconfig
    * `cd /sdk/esp32-esp-idf/components/lwip`
    * `edit Kconfig`
    * `LWIP_MAX_SOCKETS` set `range 1 16` to 255 + adjust default and help
- increase max number of LWIP sockets
    - edit sdkconfig: `CONFIG_LWIP_MAX_SOCKETS=252`, `CONFIG_LWIP_MAX_ACTIVE_TCP=252`, `CONFIG_LWIP_MAX_LISTENING_TCP=252`
    - Since three filedescriptors are used otherwise, lwIP can use at most 252
- increase max number of file descriptors
    * `cd /sdk/esp32-esp-idf/components/newlib/include/sys/`
    * `edit types.h`
    * set `FD_SETSIZE = 255`

### Increase keep_idle for TCP sockets

* `cd components/modules`
* `edit net.c`
* got to line 1068 and set keep_idle to 7200000

