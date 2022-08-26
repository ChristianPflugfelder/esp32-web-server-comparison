#include "asio.hpp"
#include <string>
#include <iostream>
#include "application_core.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "tcp_server.h"

using asio::ip::tcp;


#define MESSAGE "HTTP/1.1 200 OK\r\nDate: Fri, 26 Aug 2022 21:59:59 GMT\r\nServer: ESP32\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: %i\r\n\r\nfree_heap=%i;"

// Main --------------------------------------------

extern "C" void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    esp_netif_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Connect to Wifi
    ESP_ERROR_CHECK(example_connect());

    // blocking UART I/O
    ESP_ERROR_CHECK(example_configure_stdin_stdout());

    asio::io_context io_context;

    Server s(io_context, std::atoi(CONFIG_EXAMPLE_PORT));

    std::cout << "ASIO engine is up and running" << std::endl;
    std::cout << asio::socket_base::max_connections << std::endl;

    io_context.run();
}



// Server ------------------------------------------

Server::Server(asio::io_context& io_context, short port)
  : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), io_context_(io_context)
{
  do_accept();
}

void Server::do_accept()
{

  acceptor_.async_accept(
      [this](std::error_code ec, tcp::socket socket)
      {
        if (!ec)
        {
          std::shared_ptr<Session> session = std::make_shared<Session>(std::move(socket), io_context_);
          session->start(++socket_count);
        }else{
          std::cout << ">>> Error Accepting Connections" << std::endl;
        }

        do_accept();
      });
}



// Session -----------------------------------------

Session::Session(tcp::socket socket,
    asio::io_context& io_context)
  : socket_(std::move(socket)), timer_(io_context)
{
}

void Session::start(int socket_nr)
{
  socket_nr_str = std::to_string(socket_nr);
  std::cout << socket_nr_str << " - new connection" << std::endl;
  read();
}

auto now()
{
    return std::chrono::high_resolution_clock::now();
}

void Session::write_delayed(int delay)
{  
  auto self(shared_from_this());

  timer_.expires_at(asio::steady_timer::clock_type::now() + asio::chrono::milliseconds(delay));
  timer_.async_wait([this, self](std::error_code ec)
      {
        write();
      });
}

void Session::write()
{
  auto self(shared_from_this());
  
  int free_heap = esp_get_free_heap_size();
  int content_len = 11 + std::to_string(free_heap).length();

  snprintf(send_buf_, sizeof(send_buf_), MESSAGE, content_len, free_heap);

  asio::async_write(socket_, asio::buffer(send_buf_, strlen(send_buf_)),
      [this, self](std::error_code ec, std::size_t length)
      {
        if (!ec)
        {
          std::cout << socket_nr_str << " - \t\tresponse send" << std::endl;
          read();
        }else{
          std::cout << ">>> " << socket_nr_str << " - error sending response" << std::endl;
        }
      });
}


void Session::read()
{
  auto self(shared_from_this());
  socket_.async_read_some(asio::buffer(data_, max_length),
      [this, self](std::error_code ec, std::size_t length)
      {
        if (!ec)
        {
          data_[length] = 0;
          std::cout << socket_nr_str<< " - \tReceive request" << std::endl;

          int delay = get_header_value("Delay");
          
          std::cout << "delay: " << delay << std::endl;
          
          if(delay > 0){
            write_delayed(delay);
          }
          else{
            write();
          }
        }else{
          std::cout << ">>> " << socket_nr_str << " - error receiving request" << std::endl;
        }
      });
}

int Session::get_header_value(const char *header){
  
  int value_as_int;

  char *header_value = get_header(header);

  if(header_value != NULL){
    sscanf(header_value, "%d", &value_as_int);
    delete [] header_value;

    return value_as_int;
  }

  return 0;
}

char* Session::get_header(const char *header)
{
  char *header_start = strstr(data_, header);
  if(header_start == NULL){
    return NULL;
  }

  // +2 for ": " behind header
  char *header_value_start = header_start + strlen(header) + (2 * sizeof(char));
  char *header_value_end = strstr(header_start, "\r\n");
  int value_length = header_value_end - header_value_start;

  if(value_length <= 0){
    std::cout << ">>> Problem reading header"<< std::endl;
    return NULL;
  }
    
  char *value = new char[value_length + 1];

  strncpy(value, header_value_start, value_length);
  value[value_length] = '\0';

  return value;
}