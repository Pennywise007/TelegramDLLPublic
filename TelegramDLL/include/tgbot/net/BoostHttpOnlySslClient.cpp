/*
 * Copyright (c) 2015 Oleg Morozenkov
 * Copyright (c) 2018 JellyBrick
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "stdafx.h"
#include "tgbot/net/BoostHttpOnlySslClient.h"

#include <boost/asio/ssl.hpp>

using namespace std;
using namespace boost::asio;
using namespace boost::asio::ip;

namespace TgBot {

BoostHttpOnlySslClient::BoostHttpOnlySslClient() : _httpParser() {
}

BoostHttpOnlySslClient::~BoostHttpOnlySslClient() {
}

string BoostHttpOnlySslClient::makeRequest(const Url& url, const vector<HttpReqArg>& args) const {
    tcp::resolver resolver(_ioService);
    tcp::resolver::query query(url.host, "443");

    ssl::context context(ssl::context::tlsv12_client);
    context.set_default_verify_paths();

    ssl::stream<tcp::socket> socket(_ioService, context);

    connect(socket.lowest_layer(), resolver.resolve(query));

    #ifdef TGBOT_DISABLE_NAGLES_ALGORITHM
    socket.lowest_layer().set_option(tcp::no_delay(true));
    #endif //TGBOT_DISABLE_NAGLES_ALGORITHM
    #ifdef TGBOT_CHANGE_SOCKET_BUFFER_SIZE
    #if _WIN64 || __amd64__ || __x86_64__ || __MINGW64__ || __aarch64__ || __powerpc64__
    socket.lowest_layer().set_option(socket_base::send_buffer_size(65536));
    socket.lowest_layer().set_option(socket_base::receive_buffer_size(65536));
    #else //for 32-bit
    socket.lowest_layer().set_option(socket_base::send_buffer_size(32768));
    socket.lowest_layer().set_option(socket_base::receive_buffer_size(32768));
    #endif //Processor architecture
    #endif //TGBOT_CHANGE_SOCKET_BUFFER_SIZE
    socket.set_verify_mode(ssl::verify_none);
    socket.set_verify_callback(ssl::rfc2818_verification(url.host));

    socket.handshake(ssl::stream<tcp::socket>::client);

    string requestText = _httpParser.generateRequest(url, args, false);
    write(socket, buffer(requestText.c_str(), requestText.length()));

    string response;

    #ifdef TGBOT_CHANGE_READ_BUFFER_SIZE
    #if _WIN64 || __amd64__ || __x86_64__ || __MINGW64__ || __aarch64__ || __powerpc64__
    char buff[65536];
    #else //for 32-bit
    char buff[32768];
    #endif //Processor architecture
    #else
    char buff[1024];
    #endif //TGBOT_CHANGE_READ_BUFFER_SIZE

    boost::system::error_code error;
    while (!error) {
        size_t bytes = read(socket, buffer(buff), error);
        response += string(buff, bytes);
    }

    return _httpParser.extractBody(response);
}

}
