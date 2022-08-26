import uasyncio as asyncio
import uerrno
import micropython
import re


RESPONSSE = "HTTP/1.1 200 OK\r\nDate: Fri, 26 Aug 2022 21:59:59 GMT\r\nServer: ESP32\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: {}\r\n\r\n{}"



# Server ------------------------------------------

async def run_server():    
    await print_memory_usage()
    print("Start server")
    return await asyncio.start_server(handle_client_save, '0.0.0.0', 80)


async def print_memory_usage():
    gc.collect()
    print('Free: {} allocated: {}'.format(gc.mem_free(), gc.mem_alloc()))
    print('\n')


async def handle_client_save(reader, writer):
    try:
        await handle_client(reader, writer)
    except Exception as e:
        if (e.args[0] != uerrno.ECONNRESET      # Connection terminated by the client - both can be ignored
            and e.args[0] != uerrno.ECONNABORTED):
                raise
    finally:
        await writer.aclose()
        await reader.aclose()
        print("Connection closed")



# Client handler ----------------------------------

async def handle_client(reader, writer):
    
    print("New connection")
            
    while True:

        delay = await read_request(reader)
        
        if(delay > 0):
            await asyncio.sleep_ms(delay)
        
        await send_response(writer)
        

async def read_request(reader) :
    
    print("\tRead request")
    
    request = await reader.read(1024)
    request = request.decode()
    return parseHaeder(request, "Delay")


def parseHaeder(data, header):
    reg_value = re.search(header + ": *(\d*)\r\n", data)
    if reg_value == None:
        return 0
    return int(reg_value.group(1))


async def send_response(writer) :
    print("\tSend Response")
    gc.collect()
    body_text = f"free_heap={str(gc.mem_free())};"

    await writer.awrite(RESPONSSE.format(str(len(body_text)), body_text))



# Main --------------------------------------------

server_task = asyncio.create_task(run_server())

loop = asyncio.get_event_loop()
try:
 loop.run_forever()
except KeyboardInterrupt:
  pass
