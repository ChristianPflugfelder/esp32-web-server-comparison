#include "asio.hpp"
using asio::ip::tcp;

class Server
{
  public:
    Server(asio::io_context& io_context, short port);

  private:
    void do_accept();

    tcp::acceptor acceptor_;
    int socket_count = 0;
    asio::io_context& io_context_;
};


// Session -----------------------------------------

class Session
  : public std::enable_shared_from_this<Session>
{
  public:
    Session(tcp::socket socket, asio::io_context& io_context);
    void start(int socket_nr);

  private:
    void read();
    void write();
    void write_delayed(int delay);
    int get_header_value(const char *header);
    char* get_header(const char *header);

    std::string socket_nr_str;
    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
    char send_buf_[200];
    asio::steady_timer timer_;
};