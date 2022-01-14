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

#include "net_client.h"

namespace user_detail {
  enum class msg_type : uint32_t {
    JoinServer,
    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage,
    PassString
  };

  class Client : public net::client_interface<msg_type> {
  public:
    void ping_server()
    {
      net::message<msg_type> msg;
      msg.header.id = msg_type::ServerPing;
      msg.header.name = user_name;
      send(msg);
    }

    void message_all()
    {
      net::message<msg_type> msg;
      msg.header.id = msg_type::MessageAll;
      msg.header.name = user_name;
      send(msg);
    }

    void join_server()
    {
      std::cout << "Input Your Name: ";
      std::wcin.get(user_name.data(), user_name.size());
      std::wcin.clear();
      std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

      net::message<msg_type> msg;
      msg.header.id = msg_type::JoinServer;
      msg.header.name = user_name;
      send(msg);
    }

    void send_msg(std::wstring &__data)
    {
      net::message<msg_type> msg;
      msg.header.id = msg_type::PassString;
      msg.header.name = user_name;
      for (unsigned int i{}, j{}; j < __data.size(); ++i, ++j)
        msg.data[i] = __data[j];

      send(msg);
    }

  public:
    std::array<wchar_t, 256> user_name{};
  };
}    // namespace user_detail

int main()
{
  using namespace user_detail;

  Client c;
  c.connect("127.0.0.1", 9030);
  c.join_server();

  if (c.is_connected() && !c.get_in_comming().empty()) {
    auto msg = c.get_in_comming().pop_front().msg;

    switch (msg.header.id) {
    case msg_type::ServerAccept: {
      std::cout << "Server Accepted Connection...\n";
      break;
    }
    }
  }

  bool quit = false;

  while (!quit) {
    std::wstring buf;
    std::cout << "> ";
    getline(std::wcin, buf);
    if (buf == L"exit")
      quit = true;
    else if (buf == L"ping")
      c.ping_server();
    else if (buf == L"message all")
      c.message_all();
    else {
      c.send_msg(buf);
    }
    buf.clear();

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    if (c.is_connected()) {
      if (!c.get_in_comming().empty()) {
        auto msg = c.get_in_comming().pop_front().msg;

        switch (msg.header.id) {
        case msg_type::ServerAccept: {
          std::cout << "Server Accepted Connection\n";
          break;
        }

        case msg_type::ServerPing: {
          // Server has responded to a ping request.
          std::chrono::system_clock::time_point __now_t = std::chrono::system_clock::now();
          std::chrono::system_clock::time_point __other_t = msg.time;
          std::cout << "Ping: " << std::chrono::duration<double>(__now_t - __other_t).count() << '\n';
          break;
        }

        case msg_type::ServerMessage: {
          //uint32_t client_id;
          //msg >> client_id;
          std::cout << "Hello from [" /*<< client_id*/ << "]\n";
          break;
        }
        }
      }
    }
    else {
      // Failed to connect
      std::cout << "Server Down\n";
      quit = true;
    }
  }

  return 0;
}