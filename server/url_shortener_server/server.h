
#ifndef SERVER_H 
#define SERVER_H

#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <string>
#include <pqxx/pqxx>


namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class Server {
public:
    Server(std::string);
    void start();

    static std::optional<std::string> loadFileToString(const std::string&);
private:
    void session(std::shared_ptr<tcp::socket>);
    void handle_request(http::request<http::string_body> const&, http::response<http::string_body>&);  
    static std::string generateShortUrl(const std::string&);
    static std::string encodeBase64URL(uint64_t);
    void insertGeneratedURL(const std::string&, http::response<http::string_body>&);
    
    net::io_context ioc;
    tcp::acceptor acceptor;
    pqxx::connection db;
    pqxx::work db_work;
};

#endif