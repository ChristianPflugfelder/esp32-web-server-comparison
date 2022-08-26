from ast import arg
import asyncio
import time
import csv
import re
import argparse
import random

# Tests how many connections a server can keep open at the same time while continuously sending requests

#--------------------------------------------------
# Command line Arguments
#--------------------------------------------------

parser = argparse.ArgumentParser(description='Tests how many connections a server can keep open at the same time while continuously sending requests')
parser.add_argument('-ip','--ip',                           default="192.168.2.114",  help='IP address of esp32',                 required=False)
parser.add_argument('-t','--connection_timeout',            default="5",              help='Timeout of TCP request in seconds',   required=False)

parser.add_argument('-s','--nr_sessions',                   default="500",            help='Max number of created sessions',      required=False)
parser.add_argument('-sd','--session_delay',                default="3",              help='Delay between sessions in seconds',   required=False)
parser.add_argument('-rd','--request_delay',                default="1",              help='Delay between requests in seconds',   required=False)

parser.add_argument('-ss','--nr_simultaneous_sessions',     default="5",              help='Number of sessions that send a request at the same time', required=False)
parser.add_argument('-ssd','--simultaneous_session_delay',  default="0.05",           help='Delay between simultaneous sessions',                     required=False)

parser.add_argument('-ed','--esp_delay',                    default="500",            help='Delay on the ESP before it sends a response (in ms)',       required=False)
parser.add_argument('-er','--esp_delay_randomness',         default="0.2",            help='Randomness of the esp_delay (0.1 -> 10%)',                required=False)

parser.add_argument('-c','--write_to_csv',                  default="True",           help='Write results to CSV file',   required=False)
parser.add_argument('-n','--csv_name',                      default="IoT",            help='CSV Filename Prefix',         required=False)
args = vars(parser.parse_args())


IP = args['ip']
CONNECTION_TIMEOUT = float(args['connection_timeout'])

NR_SESSIONS = int(args['nr_sessions'])
DELAY_BETWEEN_SESSIONS = float(args['session_delay'])
DELAY_BETWEEN_REQUESTS = float(args['request_delay'])

SIMULTANEOUS_SESSIONS= int(args['nr_simultaneous_sessions'])
DELAY_BETWEEN_SIMULTANEOUS_SESSIONS = float(args['simultaneous_session_delay'])

DELAY_ON_ESP = int(args['esp_delay'])
RANDOMNESS = float(args['esp_delay_randomness'])
ESP_DELAY_MIN = int(DELAY_ON_ESP * (1 - RANDOMNESS))
ESP_DELAY_MAX = int(DELAY_ON_ESP * (1 + RANDOMNESS))

WRITE_TO_CSV = args['write_to_csv'].lower() == 'true'
CSV_NAME = args['csv_name']



MESSAGE_DELAY = "GET / HTTP/1.1\r\nHost: performance.benchmark\r\nDelay: {}\r\n"

background_tasks = set() #event loop only keeps weak references to tasks -> they could get garbage-collected if not referenced


#--------------------------------------------------
# Benchmark
#--------------------------------------------------

async def benchmark():
  
    await csv_append("Connenctions", "Response time [ms]", "Free heap space [byte]")

    for i in range(1, NR_SESSIONS + 1):
        print(f"Start {i}/{NR_SESSIONS}")
        task = loop.create_task(start_session(i))
        background_tasks.add(task)

        if i % SIMULTANEOUS_SESSIONS == 0:
          await asyncio.sleep(DELAY_BETWEEN_SESSIONS)
        else:
          await asyncio.sleep(DELAY_BETWEEN_SIMULTANEOUS_SESSIONS)
      

async def start_session(session_nr):

    try:
      wait = asyncio.open_connection(IP, 80)
      reader, writer = await asyncio.wait_for(wait, timeout=CONNECTION_TIMEOUT)
      local_socket = writer.get_extra_info('sockname')[1]

    except Exception as ex:
      print(f"Session {session_nr} could not be started")
      print(ex)
      print("Timeout")
      loop.stop()

    try:
      while True:
          
          if local_socket != writer.get_extra_info('sockname')[1]:
            raise ValueError('New Socket used -> new connection')
          

          delay = random.randint(ESP_DELAY_MIN, ESP_DELAY_MAX)
          message = MESSAGE_DELAY.format(delay).encode()

          start_time = time.perf_counter()
          writer.write(message)
          response = await reader.read(1024)
          elapsed_time_s = time.perf_counter() - start_time
          elapsed_time_ms = int(elapsed_time_s * 1000) - delay

          free_heap_space = pars_result(response.decode())
          print(f"    Session {session_nr}: {elapsed_time_ms} ms\t - {free_heap_space} byte")
          
          await csv_append(session_nr, elapsed_time_ms, free_heap_space) #writing to CSV takes ca 1 ms

          await asyncio.sleep(DELAY_BETWEEN_REQUESTS)

    except Exception as ex:
      print(f"Session {session_nr} could not be started")
      print(ex)
      loop.stop()
        

def pars_result(text):
  return re.search(r"free_heap=(\d+);", text).group(1)


#--------------------------------------------------
# CSV
#--------------------------------------------------

time_as_string = time.strftime("%Y-%m-%d_%H-%M-%S", time.gmtime())
csv_file_name = "./iot_results/{}_sessionDelay-{}_requestDelay-{}_{}.csv".format(CSV_NAME, DELAY_BETWEEN_SESSIONS, DELAY_BETWEEN_REQUESTS, time_as_string)

async def csv_append(session_nr, response_time, ram_usage):
  if not WRITE_TO_CSV:
      return

  with open(csv_file_name, 'a', encoding='UTF8', newline='') as csv_file:
    writer = csv.writer(csv_file)
    writer.writerow([session_nr, response_time, "", session_nr, ram_usage])


#--------------------------------------------------
# Run
#--------------------------------------------------

loop = asyncio.get_event_loop()
benchmark_task = loop.create_task(benchmark())
background_tasks.add(benchmark_task)

try:
 loop.run_forever()
except KeyboardInterrupt:
  pass
