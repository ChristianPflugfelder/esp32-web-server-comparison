RESPONSE_STRING = "HTTP/1.1 200 OK\r\nDate: Fri, 26 Aug 2022 21:59:59 GMT\r\nServer: ESP32\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: %d\r\n\r\n"
          

wifi.mode(wifi.STATION)
wifi.start()

station_cfg={}
station_cfg.ssid="<WiFi_SSID>"
station_cfg.pwd="<WiFi_Password>"
wifi.sta.config(station_cfg)
wifi.sta.connect()
print("Connecting to Wi-Fi")

sv = net.createServer(net.TCP, 30) --Timeout = 30


if sv then
  print("Server created")
  sv:listen(80, function(socket)
    socket:on("receive", receiver)
  end)
end


function receiver(socket, data)
  
  delay = parseHeader(data, "Delay")
  memoryUsage = parseHeader(data, "MomoryUsage")

  print("Received Data from: " .. socket:getpeer())
  print("   Delay:" .. delay)

  respond_delayed(socket, delay)
end

function respond_delayed(socket, delay)

  delay = tonumber(delay)

  free_heap = node.heap()
  
  ram_string = string.format("free_heap=%d;", free_heap)
  ram_string_size = string.len(ram_string)
  response = string.format(RESPONSE_STRING, ram_string_size)..ram_string

  if delay >= 10 then
      tmr.create():alarm(delay, tmr.ALARM_SINGLE, function()
          socket:send(response)
      end)
  else
    socket:send(response)
  end
    
end


function parseHeader(data, header)
  value = string.match(data, header..": *(%d*)\r\n")
  if value == nil then
    return 0
  end

  return value
end
