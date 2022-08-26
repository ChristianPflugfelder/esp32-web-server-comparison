# Rust TCP server

Simple Rust web server for the ESP32 using the BSD Sockets API.

The project is based on [rust-esp32-std-demo](https://github.com/ivmarkov/rust-esp32-std-demo)

## Set up Rust

* Install Rust
    * `curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh`
    * restart

* Install the Rust Espressif compiler toolchain and the Espressif LLVM Clang toolchain
    * Based on https://github.com/esp-rs/rust-build
    * Install Prerequisites `sudo apt install -y git curl gcc clang ninja-build cmake libudev-dev unzip xz-utils python3 python3-pip python3-venv libusb-1.0-0 libssl-dev pkg-config libtinfo5 libpython2.7`
    * install
        ```
        git clone https://github.com/esp-rs/rust-build.git
        cd rust-build
        ./install-rust-toolchain.sh
        ```
    * Add following command to ~/.bashrc
        * `edit ~/.bashrc`
        * add:
            ```
            export LIBCLANG_PATH="/home/cpf/.espressif/tools/xtensa-esp32-elf-clang/esp-14.0.0-20220415-x86_64-unknown-linux-gnu/lib/"

            export PATH="/home/cpf/.espressif/tools/xtensa-esp32-elf-gcc/8_4_0-esp-2021r2-patch3-x86_64-unknown-linux-gnu/bin/:/home/cpf/.espressif/tools/xtensa-esp32s2-elf-gcc/8_4_0-esp-2021r2-patch3-x86_64-unknown-linux-gnu/bin/:/home/cpf/.espressif/tools/xtensa-esp32s3-elf-gcc/8_4_0-esp-2021r2-patch3-x86_64-unknown-linux-gnu/bin/:$PATH"
            ```
    * restart

* Setup 
    * `rustup default esp`
    * `cargo install ldproxy`
    * For Error: Ubuntu 22.04 | libssl.so.1.1: cannot open shared object file: No such file or directory
        * Fix: https://stackoverflow.com/questions/72133316/ubuntu-22-04-libssl-so-1-1-cannot-open-shared-object-file-no-such-file-or-di

## Set up Webserver

* Build
    * `cargo build`
    * [Increase number of connections](#increase-number-of-connections)
    * -> clean and rerun cargo build: `cargo clean`, `cargo build`
    * In case build fails:
        * change `tos: conf.tos` to `i32` line 37 in `~/.cargo/registry/src/github.com-1ecc6299db9ec823/esp-idf-svc-0.41.4/src/ping.rs`

* Flash 
    * `espflash COM3 ./target/xtensa-esp32-espidf/debug/esp_rust_demo`
* Monitor
    * `espmonitor COM3`

## Increase number of connections

* Increase number of file descriptors
    * `edit .embuild/espressif/esp-idf/release-v4.4/CMakeLists.txt` 
    * add `idf_build_set_property(COMPILE_OPTIONS "-DFD_SETSIZE=256" APPEND)` to line 181
* Increase max number of LWIP sockets
    * `edit .embuild/espressif/esp-idf/release-v4.4/components/lwip/Kconfig`
    * Goto `LWIP_MAX_SOCKETS` set `range 1 16` to 255 + adjust default and help
    * Goto `LWIP_MAX_ACTIVE_TCP` set default to 200
    * Goto `LWIP_MAX_LISTENING_TCP` set default to 200
*  rebuild ESP-IDF: `cargo clean` `cargo build` (Without cargo clean the changes will not be applied)
