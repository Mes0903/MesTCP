#include <boost/asio.hpp>
#include <iostream>

using boost::asio::ip::tcp;

std::string getData(tcp::socket &socket)
{
  boost::asio::streambuf buf;
  boost::asio::read_until(socket, buf, "\n");
  std::string data = boost::asio::buffer_cast<const char *>(buf.data());
  return data;
}

inline void send_msg(tcp::socket &socket, const std::string &msg, boost::system::error_code &error)
{
  boost::asio::write(socket, boost::asio::buffer(msg + '\n'), error);
  if (error)
    std::cout << "send failed: " << error.message() << '\n';
}

int main(int argc, char *argv[])
{
  std::cout << "Building Server...\n";
  boost::asio::io_service io_service;

  tcp::acceptor acceptor_server(io_service, tcp::endpoint(tcp::v4(), 9030));
  tcp::socket socket(io_service);

  acceptor_server.accept(socket);
  std::cout << "Connected...";

  std::string Name = getData(socket);
  Name.pop_back();

  std::string response, reply;
  reply = "Hello " + Name + "!";
  std::cout << "Server: " << reply << '\n';
  boost::system::error_code error;
  send_msg(socket, reply, error);

  while (true) {
    response = getData(socket);
    response.pop_back();

    if (response == "exit") {
      std::cout << Name << " Exit...\n";
      break;
    }
    std::cout << Name << ": " << response << '\n';

    std::cout << "Server: ";
    getline(std::cin, reply);
    send_msg(socket, reply, error);

    if (reply == "exit")
      break;
  }

  system("pause");
  return 0;
}