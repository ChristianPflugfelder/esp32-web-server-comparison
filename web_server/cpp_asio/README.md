# Asio TCP server

Simple C++ web server for the ESP32 using Asio.

The web server is based on [tcp_echo_server example](https://github.com/espressif/esp-protocols/tree/master/components/asio/examples/tcp_echo_server).
Since v5.0 of idf, asio is in a separate [repository](https://github.com/espressif/esp-protocols) and is no longer part of ESP-IDF. Therefore it is included as a component in this project.

## Setup

- Install IDF (we only tested the application with v5.0.0)
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

## Limitations

- ASIO supports only 128 sockets (see asio::socket_base::max_connections)

## Problems during build

- In case of problems during the build (with version >= 5.0.0), it might help to use the full path for the following imports: (the header files are all located in \<esp-idf-folder\>/components/esp_wifi/include/)
    - ./extra_components/application_core/connect.c
        - line 14 #include "esp_wifi.h"
        - line 15 #include "esp_wifi_default.h"
    - ./extra_components/application_core/stdin_out.c
        - line 12 #include "esp_vfs_dev.h"
    - \<esp-idf-folder\>/components/esp_wifi/include/esp_private
        - line 13 #include "esp_wifi_crypto_types.h"
        - line 14 #include "wifi_os_adapter.h"