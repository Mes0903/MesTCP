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

#include "net_server.h"

namespace server_detail {
  enum class msg_type : uint32_t {
    JoinServer,
    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage,
    PassString
  };

  class Server : public net::server_interface<msg_type> {
  public:
    Server(uint16_t port)
        : net::server_interface<msg_type>(port) {}

  protected:
    virtual bool __on_client_connect(std::shared_ptr<net::connection<msg_type>> client)
    {
      net::message<msg_type> msg;
      msg.header.id = msg_type::ServerAccept;
      client->send(msg);
      return true;
    }

    // Called when a client appears to have disconnected
    virtual void __on_client_disconnect(std::shared_ptr<net::connection<msg_type>> client)
    {
      std::cout << "Removing client [" << client->get_id() << "] \n";
    }

    // Called when a message arrives
    virtual void __on_message(std::shared_ptr<net::connection<msg_type>> client,
                              net::message<msg_type> &msg)
    {
      switch (msg.header.id) {
      case msg_type::ServerPing: {
        std::wcout << "[" << msg.header.name.data() << "]: Ping the server\n";

        // Simply bounce message back to client
        client->send(msg);
        break;
      }

      case msg_type::MessageAll: {
        std::wcout << "[" << msg.header.name.data() << "]: Send the message to all user\n";

        //Construct a new message and send it to all clients
        net::message<msg_type> __msg;
        __msg.header.id = msg_type::ServerMessage;
        __msg.header.name = msg.header.name;
        message_all_clients(__msg, client);
        break;
      }

      case msg_type::JoinServer: {
        std::wcout << "[" << msg.header.name.data() << "] Join the server\n";
        break;
      }

      case msg_type::PassString: {
        std::wcout << "[" << msg.header.name.data() << "]: " << msg.data.data() << '\n';
        break;
      }
      }
    }
  };
}    // namespace server_detail

int main()
{
  using namespace server_detail;
  Server server(9030);
  server.start();

  while (true) {
    server.update(-1, true);
  }

  return 0;
}