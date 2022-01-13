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

#ifndef NET_SERVER
#define NET_SERVER

#include "net.h"

using boost::asio::ip::tcp;

namespace net {
  template <typename T>
  class server_interface {
  public:
    server_interface(uint16_t port)
        : __acceptor(__io_context, tcp::endpoint(tcp::v4(), port)) {}

    virtual ~server_interface() { stop(); }

  public:
    bool start()
    {
      try {
        // Issue a task to the asio context - This is important
        // as it will prime the context with "work", and stop it
        // from exiting immediately. Since this is a server, we
        // wnat it primed ready to handle clients trying to connect.
        wait_for_client_connection();
        __context_thread = std::thread([this]() { __io_context.run(); });
      } catch (std::exception &excp) {
        // Something prohibited the server from listening.
        std::cerr << "[SERVER] Exception: " << excp.what() << '\n';
        return false;
      }

      std::cout << "[SERVER MESSAGE] Server started...\n";
      return true;
    }

    void stop()
    {
      __io_context.stop();
      if (__context_thread.joinable())
        __context_thread.join();

      std::cout << "[SERVER] Server stopped...\n";
    }

    void wait_for_client_connection()
    {
      // Prime context with an instruction to wait until a socket connects. This
      // is the purpose of an "acceptor" object. It will provide a unique socket
      // for each incoming connection attempt.
      __acceptor.async_accept([this](std::error_code err, tcp::socket socket) {
        // Trigged by incoming connection request.
        if (!err) {
          std::cout << "[SERVER MESSAGE] Server Get New Connection\n";

          // Create a new connection to handle this client.
          std::shared_ptr<connection<T>> new_connect =
            std::make_shared<connection<T>>(connection<T>::owner::server, __io_context, std::move(socket), __q_messages_in);

          // Give the user server a chance to deny connection.
          if (__on_client_connect(new_connect)) {
            // Connection allowed, so add to container of new connection.
            __connection_deq.push_back(std::move(new_connect));

            // Issue a task to the connection's asio context to sit
            // and wait for bytes to arrive.
            __connection_deq.back()->connect_to_client(__io_counter++);

            //std::cout << "[" << __connection_deq.back()->get_id() << "] Connection Accepted...\n";
          }
          else {
            // Connection will go out of scope with no pending tasks, so will
            // get destroyed automatically due to the wonder of smart pointers.
            std::cout << "[-----] Connection Denied...!\n";
          }
        }
        else {
          // Error has occured during acceptance.
          std::cout << "[SERVER] Connection Error: " << err.message() << '\n';
        }

        wait_for_client_connection();
      });
    }

    // Send a message to a specific client.
    void message_client(std::shared_ptr<connection<T>> client, const message<T> &msg)
    {
      // Check if the client is legitimate...
      if (client && client->is_connect()) {
        // ...and post the message via the connection.
        client->send(msg);
      }
      else {
        // If we can't communicate with the client, then we may as
        // well remove the client - let the server know, it may
        // be tracking it somehow.
        __on_client_disconnect(client);

        // off the client.
        client.reset();

        // Then physically remove it from the containter
        __connection_deq.erase(
          std::remove(__connection_deq.begin(), __connection_deq.end(), client), __connection_deq.end());
      }
    }

    // Send message to all clients
    void message_all_clients(const message<T> &msg, std::shared_ptr<connection<T>> ignored_client = nullptr)
    {
      bool invalid_client_exists = false;

      // Iterate through all clients in container
      for (auto &__client : __connection_deq) {
        // Check if the client is connect...
        if (__client && __client->is_connected()) {
          // ...if yes, and it's not the client been ignored
          if (__client != ignored_client)
            __client->send(msg);
        }
        else {
          // The client couldn't be contacted, so assume it has disconnected.
          __on_client_disconnect(__client);
          __client.reset();

          // Set this flag to then remove dead clients from container.
          invalid_client_exists = true;
        }
      }

      // Remove dead clients, all in one go - this way, we don't invalidate the
      // container as we iterated through it.
      if (invalid_client_exists)
        __connection_deq.erase(
          std::remove(__connection_deq.begin(), __connection_deq.end(), nullptr), __connection_deq.end());
    }

    // Force server to respond to incoming messages
    void update(std::size_t max_messages = -1, bool __wait = false)
    {
      if (__wait)
        __q_messages_in.wait();

      // Process as many messages as you can up to the value specified.
      std::size_t __message_count = 0;
      while (__message_count < max_messages && !__q_messages_in.empty()) {
        // Grab the front message
        auto msg = __q_messages_in.pop_front();

        // Pass to message handler
        __on_message(msg.remote, msg.msg);
        __message_count++;
      }
    }

  protected:
    // This server class should override thse functions to implement
    // customised functionality

    // Called when a client connects, you can veto the connection by returning false
    virtual bool __on_client_connect(std::shared_ptr<connection<T>> client)
    {
      return false;
    }

    // Called when a client appears to have disconnected
    virtual void __on_client_disconnect(std::shared_ptr<connection<T>> client)
    {
    }

    // Called when a message arrives
    virtual void __on_message(std::shared_ptr<connection<T>> client, message<T> &msg)
    {
    }


  protected:
    // Thread Safe Queue for incoming message packets
    ts_queue<owned_message<T>> __q_messages_in;

    // Container of active validated connections
    std::deque<std::shared_ptr<connection<T>>> __connection_deq;

    // Order of declaration is important - it is also the order of initialisation
    boost::asio::io_context __io_context;
    std::thread __context_thread;

    // These things need an asio context
    tcp::acceptor __acceptor;    // Handles new incoming connection attempts...

    // Clients will be identified in the "wider system" via an ID
    uint32_t __io_counter = 0;
  };
}    // namespace net

#endif