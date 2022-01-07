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

int main()
{
  std::cout << "Building Client...\n";
  boost::asio::io_service io_service;
  tcp::socket socket(io_service);
  socket.connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 9030));
  std::cout << "Connected...\nEnter the user name: ";

  std::string Name, msg, reply;
  boost::system::error_code error;
  getline(std::cin, Name);
  send_msg(socket, Name, error);

  while (true) {
    reply = getData(socket);
    reply.pop_back();

    if (reply == "exit") {
      std::cout << "Connection terminated, killing Process\n";
      break;
    }
    std::cout << "Server: " << reply << '\n';

    // Reading new message from input stream
    std::cout << Name << ": ";
    getline(std::cin, msg);
    send_msg(socket, msg, error);

    if (msg == "exit")
      break;
  }

  system("pause");
  return 0;
}