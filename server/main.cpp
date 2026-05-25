#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include "url_shortener_server/server.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;


int main() {
    auto connect_info = Server::loadFileToString("../server/secret.txt");
    if (!connect_info.has_value()) {
        connect_info = Server::loadFileToString("../server/secret_exmpl.txt");
    }
    std::string connect = connect_info.value_or("");
    Server s(connect);
    s.start();
    return 0;
}