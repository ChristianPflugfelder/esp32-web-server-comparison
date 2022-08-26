from ast import arg
import asyncio
import time
import csv
import re
import argparse

# Tests how many connections a server can keep open at the same time

#--------------------------------------------------
# Command line Arguments
#--------------------------------------------------

parser = argparse.ArgumentParser(description='Tests how many connections a server can keep open at the same time')
parser.add_argument('-ip','--ip',                     default="192.168.2.114",    help='IP address of esp32',               required=False)
parser.add_argument('-s','--nr_sessions',             default="500",              help='Max number of created sessions',    required=False)
parser.add_argument('-sd','--session_delay',          default="0.2",              help='Delay between sessions in seconds', required=False)
parser.add_argument('-rd','--request_delay',          default="0",                help='Delay between requests in seconds', required=False)
parser.add_argument('-t','--connection_timeout',      default="5",                help='Timeout of TCP request in seconds', required=False)
parser.add_argument('-c','--write_to_csv',            default="True",             help='Write results to CSV file',         required=False)
parser.add_argument('-n','--csv_name',                default="Connection",       help='CSV Filename Prefix',               required=False)
args = vars(parser.parse_args())


IP = args['ip']
NR_SESSIONS = int(args['nr_sessions'])
DELAY_BETWEEN_SESSIONS = float(args['session_delay'])
DELAY_BETWEEN_REQUESTS = float(args['request_delay'])
CONNECTION_TIMEOUT = float(args['connection_timeout'])
WRITE_TO_CSV = args['write_to_csv'].lower() == 'true'
CSV_NAME = args['csv_name']



MESSAGE = "GET / HTTP/1.1\r\nHost: performance.benchmark\r\n".encode()

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
        await asyncio.sleep(DELAY_BETWEEN_SESSIONS)
      

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
          
          start_time = time.perf_counter()
          writer.write(MESSAGE)
          response = await reader.read(1024)
          elapsed_time_s = time.perf_counter() - start_time
          elapsed_time_ms = int(elapsed_time_s * 1000)

          free_heap_space = pars_result(response.decode())
          print(f"    Session {session_nr}: {elapsed_time_ms} ms\t - {free_heap_space} byte")
          
          await csv_append(session_nr, f"{elapsed_time_ms}", free_heap_space) #writing to CSV takes ca 1 ms

          if DELAY_BETWEEN_REQUESTS == 0:
            response = await reader.read(1) #stay here till benchmark is finished and keep connection allive

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
csv_file_name = "./connection_results/{}_sessionDelay-{}_requestDelay-{}_{}.csv".format(CSV_NAME, DELAY_BETWEEN_SESSIONS, DELAY_BETWEEN_REQUESTS, time_as_string)

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
