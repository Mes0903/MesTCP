/*
	MMO Client/Server Framework using ASIO
	"Happy Birthday Mrs Javidx9!" - javidx9
	Videos: 
	Part #1: https://youtu.be/2hNdkYInj4g
	Part #2: https://youtu.be/UbjxGvrDrbw
	License (OLC-3)
	~~~~~~~~~~~~~~~
	Copyright 2018 - 2020 OneLoneCoder.com
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:
	1. Redistributions or derivations of source code must retain the above
	copyright notice, this list of conditions and the following disclaimer.
	2. Redistributions or derivative works in binary form must reproduce
	the above copyright notice. This list of conditions and the following
	disclaimer must be reproduced in the documentation and/or other
	materials provided with the distribution.
	3. Neither the name of the copyright holder nor the names of its
	contributors may be used to endorse or promote products derived
	from this software without specific prior written permission.
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
	A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
	HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
	LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
	Links
	~~~~~
	YouTube:	https://www.youtube.com/javidx9
	Discord:	https://discord.gg/WhwHUMV
	Twitter:	https://www.twitter.com/javidx9
	Twitch:		https://www.twitch.tv/javidx9
	GitHub:		https://www.github.com/onelonecoder
	Homepage:	https://www.onelonecoder.com
	Author
	~~~~~~
	David Barr, aka javidx9, ©OneLoneCoder 2019, 2020
*/

#ifndef NET_CLIENT
#define NET_CLIENT

#include "net.h"

using boost::asio::ip::tcp;

namespace net {
  template <typename T>
  class client_interface {
  public:
    client_interface() {}
    virtual ~client_interface() { disconnect(); }

  public:
    // connect to server with hostname/ip-address and port
    bool connect(const std::string &host, const uint16_t port)
    {
      try {
        // Resolve hostname/ip-address into tangiable physical address
        tcp::resolver resolver(__io_context);
        tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

        // Create connection
        connect_ptr = std::make_unique<connection<T>>(connection<T>::owner::client, __io_context, tcp::socket(__io_context), __q_messages_in);

        // Tell the connection object to connect to server
        connect_ptr->connect_to_server(endpoints);

        // Start Context Thread
        thrContext = std::thread([this]() { __io_context.run(); });
        std::cerr << "leave connect function\n";
      } catch (std::exception &e) {
        std::cerr << "Client Exception: " << e.what() << '\n';
        return false;
      }
      return true;
    }

    // Disconnect from server
    void disconnect()
    {
      // If connection exists, and it's connected then...
      if (is_connected()) {
        // ...disconnect from server gracefully
        connect_ptr->disconnect();
      }

      // Either way, we're also done with the asio context...
      __io_context.stop();
      // ...and its thread
      if (thrContext.joinable())
        thrContext.join();

      // Destroy the connection object
      connect_ptr.release();
    }

    // Check if client is actually connected to a server
    bool is_connected()
    {
      if (connect_ptr)
        return connect_ptr->is_connected();
      else
        return false;
    }

  public:
    // Send message to server
    void send(const message<T> &msg)
    {
      if (is_connected())
        connect_ptr->send(msg);
    }

    // Retrieve queue of messages from server
    ts_queue<owned_message<T>> &get_in_comming()
    {
      return __q_messages_in;
    }

  protected:
    // asio context handles the data transfer...
    boost::asio::io_context __io_context;
    // ...but needs a thread of its own to execute its work commands
    std::thread thrContext;
    // The client has a single instance of a "connection" object, which handles data transfer
    std::unique_ptr<connection<T>> connect_ptr;

  private:
    // This is the thread safe queue of in_comming messages from server
    ts_queue<owned_message<T>> __q_messages_in;
  };
}    // namespace net

#endif