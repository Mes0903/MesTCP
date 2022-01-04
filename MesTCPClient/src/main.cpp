#include <iostream>
#include <boost/asio.hpp>
#include <string>

using namespace boost::asio::ip;

int main()
{
    boost::asio::io_service io_service;
    tcp::socket socket(io_service);
    socket.connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 9030));

    const std::string msg = "Testing!\n";
    boost::system::error_code error;
    boost::asio::write(socket, boost::asio::buffer(msg), error);

    if (!error)
        std::cout << "success sent msg!\n";
    else
        std::cout << "send failed : " << error.message() << '\n';

    boost::asio::streambuf receive_buffer;
    boost::asio::read(socket, receive_buffer, boost::asio::transfer_all(), error);

    if (error && error != boost::asio::error::eof)
        std::cout << "receive failed : " << error.message() << '\n';
    else
        std::cout << boost::asio::buffer_cast<const char *>(receive_buffer.data());

    return 0;
}