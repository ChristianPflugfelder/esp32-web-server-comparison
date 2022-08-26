# Asynchronous programming on the ESP32
To compare asynchronous programming with different programming languages on the ESP32, we programmed a simple web server with C, C++, Rust, Lua and Python as part of our bachelor thesis. 

<br>

In C and Rust we use freeRTOS tasks, i.e. OS threads for handling connections. In C++ we use Asio and for Lua the NodeMCU firmware. In both cases, callbacks are used for asynchronous operations. In Python, on the other hand, we use the async/await pattern with AsyncIO.

<br>

To compare the performance of the web servers, we measured characteristics such as memory consumption, response time, and the maximum number of parallel connections.
For this we used [Ali](https://github.com/nakabonne/ali) and two custom benchmarktools.

## Repository structure
* The [web server](web_server/) directory contains the sample projects for the web servers of the respective languages. The readme of each project provides more detailed instructions for the build process.
* The [benchmark](benchmark/) directory contains the three benchmarks and the results we measured. 
