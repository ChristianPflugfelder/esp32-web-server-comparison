# Benchmarks

Benchmarks to compare the performance of the differen web server implementations.

## Throughput
[Ali](https://github.com/nakabonne/ali) is used to measure the max throughput
* run `./ali.exe -r 50 -d 10s http://\<ip-of-esp\>` (replace 192.168.2.123 with ESP32 IP)
* increase request rate per second until not all Responses are received (Status Codes) / the server crashes

## Number of Connections
`connection_benchmark.py` is used to measure the max number of connections
* run `python connection_benchmark.py -h` for list of arguments
* for our test configuration use `python connection_benchmark.py -ip \<ip-of-esp\>`
* the latency of the response as well as the free heap space transmitted by the server is logged in a CSV


## IoT Contex
`connection_benchmark.py` is used to measure the max number of connections with higher load and additional delay on the esp32
* run `python connection_benchmark.py -h` for list of arguments
* for our test configuration use `python connection_benchmark.py -ip \<ip-of-esp\>`
* the latency of the response as well as the free heap space transmitted by the server is logged in a CSV