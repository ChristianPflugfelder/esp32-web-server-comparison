# C TCP server

Simple C web server for the ESP32 using the BSD Sockets API.

The [Wi-Fi setup](extra_components/application_core/) is based on [esp-protocols/common_components](https://github.com/espressif/esp-protocols/tree/master/common_components/protocol_examples_common).

## Setup

- Install IDF
- Open the project configuration menu (`idf.py menuconfig`) to configure Wi-Fi in the `Example configuration` submenu

### Increase number of connections

- Required changes already made in this project:
    - increase max number of file descriptors
        - add `idf_build_set_property(COMPILE_OPTIONS "-DFD_SETSIZE=255" APPEND)` to toplevel CMakeLists.txt in project
    - increase max number of LWIP sockets
        - edit sdkconfig: `CONFIG_LWIP_MAX_SOCKETS=200`, `CONFIG_LWIP_MAX_ACTIVE_TCP=200`, `CONFIG_LWIP_MAX_LISTENING_TCP=200`
- Required changes that must be made manually
    - modify IDF so that more than 16 sockets can be set in projects sdkconfig
        * `cd \<esp-idf-folder\>/components/lwip`
        * `edit Kconfig`
        * `LWIP_MAX_SOCKETS` set `range 1 16` to 255 + adjust default and help

## Run

- The server port number can be changed in the project configuration menu (deault is 80)
- Build and flash the webserver using `idf.py flash`
- Connect to the serial monitor of the ESP32 using `idf.py monitor`
- After the ESP21 is connected to Wi-Fi, the IP address is printed to the serial monitor
- As soon as the web server is running one can send TCP requests