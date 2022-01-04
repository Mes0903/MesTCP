#include <boost/asio.hpp>
#include <iostream>

using namespace boost::asio::ip;

std::string read_(tcp::socket &socket)
{
  boost::asio::streambuf buf;
  boost::asio::read_until(socket, buf, "\n");
  std::string data = boost::asio::buffer_cast<const char *>(buf.data());
  return data;
}
void send_(tcp::socket &socket, const std::string &message)
{
  const std::string msg = message + "\n";
  boost::asio::write(socket, boost::asio::buffer(message));
}

int main()
{
  std::cout << "test\n";
  boost::asio::io_service io_service;
  //listen for new connection
  tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), 9030));
  //socket creation
  tcp::socket socket_(io_service);
  //waiting for connection
  acceptor_.accept(socket_);
  //read operation
  std::string message = read_(socket_);
  std::cout << message << '\n';
  //write operation
  send_(socket_, "Hello From Server!");
  std::cout << "Servent sent Hello message to Client!\n";
  return 0;
}