# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# (Not part of the boilerplate)
# This example uses an extra component for common functions such as Wi-Fi and Ethernet connection.
set(EXTRA_COMPONENT_DIRS ./extra_components/application_core)

include(C:/Espressif/frameworks/esp-idf-master/tools/cmake/project.cmake)
project(asio_tcp_echo_server)
idf_build_set_property(COMPILE_OPTIONS "-DFD_SETSIZE=255" APPEND)