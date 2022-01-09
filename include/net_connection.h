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
	David Barr, aka javidx9, �OneLoneCoder 2019, 2020

*/

#ifndef NET_CONNECTION
#define NET_CONNECTION

#include "net_common.h"
#include "net_queue.h"
#include "net_message.h"

using boost::asio::ip::tcp;


namespace net {
  template <typename T>
  class connection : public std::enable_shared_from_this<connection<T>> {
  public:
    // A connection is "owned" by either a server or a client, and its
    // behaviour is slightly different bewteen the two.
    enum class owner {
      server,
      client
    };

  public:
    // Constructor: Specify Owner, connect to context, transfer the socket
    //				Provide reference to incoming message queue
    connection(owner parent, boost::asio::io_context &asioContext, boost::asio::ip::tcp::socket socket, ts_queue<owned_message<T>> &qIn)
        : __io_context(asioContext), __socket(std::move(socket)), __q_messages_in(qIn)
    {
      __owerner_type = parent;
    }

    virtual ~connection()
    {
    }

    // This ID is used system wide - its how clients will understand other clients
    // exist across the whole system.
    uint32_t get_id() const
    {
      return id;
    }

  public:
    void connect_to_client(uint32_t uid = 0)
    {
      if (__owerner_type == owner::server) {
        if (__socket.is_open()) {
          id = uid;
          read_header();
        }
      }
    }

    void connect_to_server(const tcp::resolver::results_type &endpoints)
    {
      // Only clients can connect to servers
      if (__owerner_type == owner::client) {
        // Request asio attempts to connect to an endpoint
        boost::asio::async_connect(__socket, endpoints,
                                   [this](std::error_code ec, tcp::endpoint endpoint) {
                                     if (!ec) {
                                       read_header();
                                     }
                                   });
      }
    }


    void disconnect()
    {
      if (is_connected())
        boost::asio::post(__io_context, [this]() { __socket.close(); });
    }

    bool is_connected() const
    {
      return __socket.is_open();
    }

    // Prime the connection to wait for incoming messages
    void start_listening()
    {
    }

  public:
    // ASYNC - send a message, connections are one-to-one so no need to specifiy
    // the target, for a client, the target is the server and vice versa
    void send(const message<T> &msg)
    {
      boost::asio::post(__io_context,
                        [this, msg]() {
                          // If the queue has a message in it, then we must
                          // assume that it is in the process of asynchronously being written.
                          // Either way add the message to the queue to be output. If no messages
                          // were available to be written, then start the process of writing the
                          // message at the front of the queue.
                          bool bWritingMessage = !__q_messages_out.empty();
                          __q_messages_out.push_back(msg);
                          if (!bWritingMessage) {
                            write_header();
                          }
                        });
    }



  private:
    // ASYNC - Prime context to write a message header
    void write_header()
    {
      // If this function is called, we know the outgoing message queue must have
      // at least one message to send. So allocate a transmission buffer to hold
      // the message, and issue the work - asio, send these bytes
      boost::asio::async_write(__socket, boost::asio::buffer(&__q_messages_out.front().header, sizeof(message_header<T>)),
                               [this](std::error_code ec, std::size_t length) {
                                 // asio has now sent the bytes - if there was a problem
                                 // an error would be available...
                                 if (!ec) {
                                   // ... no error, so check if the message header just sent also
                                   // has a message body...
                                   if (__q_messages_out.front().body.size() > 0) {
                                     // ...it does, so issue the task to write the body bytes
                                     write_body();
                                   }
                                   else {
                                     // ...it didnt, so we are done with this message. Remove it from
                                     // the outgoing message queue
                                     __q_messages_out.pop_front();

                                     // If the queue is not empty, there are more messages to send, so
                                     // make this happen by issuing the task to send the next header.
                                     if (!__q_messages_out.empty()) {
                                       write_header();
                                     }
                                   }
                                 }
                                 else {
                                   // ...asio failed to write the message, we could analyse why but
                                   // for now simply assume the connection has died by closing the
                                   // socket. When a future attempt to write to this client fails due
                                   // to the closed socket, it will be tidied up.
                                   std::cout << "[" << id << "] Write Header Fail.\n";
                                   __socket.close();
                                 }
                               });
    }

    // ASYNC - Prime context to write a message body
    void write_body()
    {
      // If this function is called, a header has just been sent, and that header
      // indicated a body existed for this message. Fill a transmission buffer
      // with the body data, and send it!
      boost::asio::async_write(__socket, boost::asio::buffer(__q_messages_out.front().body.data(), __q_messages_out.front().body.size()),
                               [this](std::error_code ec, std::size_t length) {
                                 if (!ec) {
                                   // Sending was successful, so we are done with the message
                                   // and remove it from the queue
                                   __q_messages_out.pop_front();

                                   // If the queue still has messages in it, then issue the task to
                                   // send the next messages' header.
                                   if (!__q_messages_out.empty()) {
                                     write_header();
                                   }
                                 }
                                 else {
                                   // Sending failed, see write_header() equivalent for description :P
                                   std::cout << "[" << id << "] Write Body Fail.\n";
                                   __socket.close();
                                 }
                               });
    }

    // ASYNC - Prime context ready to read a message header
    void read_header()
    {
      // If this function is called, we are expecting asio to wait until it receives
      // enough bytes to form a header of a message. We know the headers are a fixed
      // size, so allocate a transmission buffer large enough to store it. In fact,
      // we will construct the message in a "temporary" message object as it's
      // convenient to work with.
      boost::asio::async_read(__socket, boost::asio::buffer(&__temp_msg_in.header, sizeof(message_header<T>)),
                              [this](std::error_code ec, std::size_t length) {
                                if (!ec) {
                                  // A complete message header has been read, check if this message
                                  // has a body to follow...
                                  if (__temp_msg_in.header.size > 0) {
                                    // ...it does, so allocate enough space in the messages' body
                                    // vector, and issue asio with the task to read the body.
                                    __temp_msg_in.body.resize(__temp_msg_in.header.size);
                                    read_body();
                                  }
                                  else {
                                    // it doesn't, so add this bodyless message to the connections
                                    // incoming message queue
                                    add_to_incomming_message_queue();
                                  }
                                }
                                else {
                                  // Reading form the client went wrong, most likely a disconnect
                                  // has occurred. Close the socket and let the system tidy it up later.
                                  std::cout << "[" << id << "] Read Header Fail.\n";
                                  __socket.close();
                                }
                              });
    }

    // ASYNC - Prime context ready to read a message body
    void read_body()
    {
      // If this function is called, a header has already been read, and that header
      // request we read a body, The space for that body has already been allocated
      // in the temporary message object, so just wait for the bytes to arrive...
      boost::asio::async_read(__socket, boost::asio::buffer(__temp_msg_in.body.data(), __temp_msg_in.body.size()),
                              [this](std::error_code ec, std::size_t length) {
                                if (!ec) {
                                  // ...and they have! The message is now complete, so add
                                  // the whole message to incoming queue
                                  add_to_incomming_message_queue();
                                }
                                else {
                                  // As above!
                                  std::cout << "[" << id << "] Read Body Fail.\n";
                                  __socket.close();
                                }
                              });
    }

    // Once a full message is received, add it to the incoming queue
    void add_to_incomming_message_queue()
    {
      // Shove it in queue, converting it to an "owned message", by initialising
      // with the a shared pointer from this connection object
      if (__owerner_type == owner::server)
        __q_messages_in.push_back({ this->shared_from_this(), __temp_msg_in });
      else
        __q_messages_in.push_back({ nullptr, __temp_msg_in });

      // We must now prime the asio context to receive the next message. It
      // wil just sit and wait for bytes to arrive, and the message construction
      // process repeats itself. Clever huh?
      read_header();
    }

  protected:
    // Each connection has a unique socket to a remote
    tcp::socket __socket;

    // This context is shared with the whole asio instance
    boost::asio::io_context &__io_context;

    // This queue holds all messages to be sent to the remote side
    // of this connection
    ts_queue<message<T>> __q_messages_out;

    // This references the incoming queue of the parent object
    ts_queue<owned_message<T>> &__q_messages_in;

    // Incoming messages are constructed asynchronously, so we will
    // store the part assembled message here, until it is ready
    message<T> __temp_msg_in;

    // The "owner" decides how some of the connection behaves
    owner __owerner_type = owner::server;

    uint32_t id = 0;
  };
}    // namespace net
#endif