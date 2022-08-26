use std::{sync::Arc, thread, time::*};

use embedded_svc::wifi::*;

use esp_idf_svc::netif::*;
use esp_idf_svc::nvs::*;
use esp_idf_svc::sysloop::*;
use esp_idf_svc::wifi::*;

use std::io::prelude::*;
use std::net::TcpListener;
use std::net::TcpStream;
use std::panic;

const SSID: &str = "<WiFi_SSID>";
const PASS: &str = "<WiFi_PASSWORD>";
const RESPONSE_HEADER: &str = "HTTP/1.1 200 OK\r\nDate: Fri, 26 Aug 2022 21:59:59 GMT\r\nServer: ESP32\r\nContent-Type: text/plain; charset=utf-8\r\n";
    

extern "C" {
    fn esp_get_free_heap_size() -> usize;
}

fn main() -> anyhow::Result<()> {

    println!("Connecting to Wifi");
    
    #[allow(unused)]
    let netif_stack = Arc::new(EspNetifStack::new().unwrap());
    #[allow(unused)]
    let sys_loop_stack = Arc::new(EspSysLoopStack::new().unwrap());
    #[allow(unused)]
    let default_nvs = Arc::new(EspDefaultNvs::new().unwrap());

    #[allow(clippy::redundant_clone)]
    #[allow(unused_mut)]
    let mut _wifi = wifi(
        netif_stack.clone(),
        sys_loop_stack.clone(),
        default_nvs.clone(),
    ).unwrap();

    start_webserver();
    Ok(())
}

fn wifi(
    netif_stack: Arc<EspNetifStack>,
    sys_loop_stack: Arc<EspSysLoopStack>,
    default_nvs: Arc<EspDefaultNvs>,
) -> anyhow::Result<Box<EspWifi>> {
    let mut wifi = Box::new(EspWifi::new(netif_stack, sys_loop_stack, default_nvs)?);

    println!("Wifi created, about to scan");

    let ap_infos = wifi.scan()?;

    let ours = ap_infos.into_iter().find(|a| a.ssid == SSID);

    let channel = if let Some(ours) = ours {
        println!(
            "Found configured access point {} on channel {}",
            SSID, ours.channel
        );
        Some(ours.channel)
    } else {
        println!(
            "Configured access point {} not found during scanning, will go with unknown channel",
            SSID
        );
        None
    };

    print!("CONFIG: ssid={}, pw={}", SSID, PASS);

    wifi.set_configuration(&Configuration::Client(
        ClientConfiguration {
            ssid: SSID.into(),
            password: PASS.into(),
            channel,
            ..Default::default()
        }))?;

    println!("Wifi configuration set, about to get status");

    wifi.wait_status_with_timeout(Duration::from_secs(60), |status| !status.is_transitional())
        .map_err(|e| anyhow::anyhow!("Unexpected Wifi status: {:?}", e))?;

    let status = wifi.get_status();

    if let Status(
        ClientStatus::Started(ClientConnectionStatus::Connected(ClientIpStatus::Done(_ip_settings))),
        ApStatus::Stopped,
    ) = status
    {
        println!("Wifi connected");
    } else {
        println!("Unexpected Wifi status: {:?}", status);
    }

    Ok(wifi)
}

fn start_webserver(){
    println!("Listening for new Connections");
    let listener = TcpListener::bind("0.0.0.0:80").unwrap();

    for stream in listener.incoming() {
        println!("New Connection");
        
        let stream = stream.unwrap();
        thread::spawn(|| handle_connection(stream));
    }
}

fn handle_connection(stream: TcpStream) {

    loop{
        let mut err = read_request(&stream);
        if let Err(_err) = err{
            println!(">>> Connection closed");
            break;
        }

        let response = get_response_message();

        err = write_response(&stream, &response);
        if let Err(_err) = err{
            println!(">>> Connection closed");
            break;
        }
    }
}

fn get_response_message() -> String{

    let free_heap = unsafe{esp_get_free_heap_size()};
    let body = format!( "free_heap={};", free_heap);
    
    return format!("{}Content-Length: {}\r\n\r\n{}",
        RESPONSE_HEADER,
        body.len(),
        body
    )
}

fn write_response(mut stream: &TcpStream, text: &String) -> Result<usize, std::io::Error> {
        
    let nr_bytes_written = stream.write(text.as_bytes());
    if let Err(ref _err) = nr_bytes_written{
        println!("<write>Connection closed");
        return nr_bytes_written
    }

    stream.flush().unwrap();

    return nr_bytes_written
}

fn read_request(mut stream: &TcpStream) -> Result<usize, std::io::Error> {
    let get = b"GET";
    let mut buffer = [0; 1024];

    
    loop{
        let nr_bytes_read = stream.read(&mut buffer);
        if let Err(ref _err) = nr_bytes_read{
            println!("<read>Connection closed");
            return nr_bytes_read
        }

        if buffer.starts_with(get){

            let buffer_as_string = match std::str::from_utf8(&buffer) {
                Ok(value) => value,
                Err(error) => panic!("Response has invalid UTF-8 sequence: {}", error),
            };

            let _memory_usage = get_header(buffer_as_string, "MomoryUsage");
            let delay = get_header(buffer_as_string, "Delay");
            println!("Delay: {}", delay);
            if delay > 0 {
                thread::sleep(Duration::from_millis(delay));
            }

            return nr_bytes_read
        }
    }
}

fn get_header(data: &str, header: &str) -> u64{
    let header_start_index = data.find(header).unwrap_or_else(|| return usize::MAX);
    if header_start_index == usize::MAX {
        println!("start not found");
        return 0
    }

    let header_value_start = header_start_index + str_len(header) + 2;
    let header_value_string = &data[header_value_start..str_len(data)];
    let header_value_end = header_value_string.find("\r\n").unwrap_or_else(|| return usize::MAX);
    if header_value_end == usize::MAX {
        println!("end not found");
        return 0
    }
    

    let header_value = &header_value_string[..header_value_end];
    
    return header_value.parse::<u64>().unwrap_or_else(|_err| return 0);
}

fn str_len(string: &str) -> usize{
    return string.chars().count();
}