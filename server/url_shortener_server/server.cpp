#include "server.h"

#include <boost/beast/core.hpp>
#include <boost/json.hpp>

#include <optional>
#include <iostream>
#include <fstream>
#include <thread>
#include <functional>

Server::Server(std::string connect) : acceptor(ioc, tcp::endpoint(tcp::v4(), 8080)), db(connect), db_work(db) {}

void Server::start() {
	while (true) {
		try {
			std::shared_ptr<tcp::socket> socket = std::make_shared<tcp::socket>(ioc);
			acceptor.accept(*socket);
			std::thread t(std::bind(&Server::session, this, socket));
			t.detach();
		}
		catch (std::exception const& e) {
			std::cerr << "Error: " << e.what() << '\n';
		}
	}
}

void Server::session(std::shared_ptr<tcp::socket> socket) {
	beast::flat_buffer buffer;
	http::request<http::string_body> req;
	try {
		http::read(*socket, buffer, req);
	}
	catch (const boost::system::system_error& e) {
		if (e.code() == boost::beast::http::error::end_of_stream) {
			std::cerr << "Client disconnected: " << e.what() << std::endl;
		}
		else {
			std::cerr << "Error: " << e.what() << std::endl;
		}

		return;
	}
	http::response<http::string_body> res;
	const auto& target_str_view = req.target();
	const auto& target_str_view_tr = target_str_view.substr(1);
	pqxx::result existing_row = db_work.exec_params("SELECT * FROM urls WHERE url_short = $1", std::string(target_str_view_tr.data(), target_str_view_tr.size()).c_str());
	db_work.commit();
	if (!existing_row.empty()) {
		res.result(http::status::found);
		res.set(http::field::location, existing_row.front().at(1).view());
		res.prepare_payload();
	}
	else {
		handle_request(req, res);
	}
	try {
		http::write(*socket, res);
	}
	catch (const boost::system::system_error& e) {
		if (e.code() == boost::asio::error::broken_pipe) {
			std::cerr << "Client disconnected: " << e.what() << std::endl;
		}
		else {
			std::cerr << "Error: " << e.what() << std::endl;
		}
	}
}

std::optional<std::string> Server::loadFileToString(const std::string& filePath) {
	std::ifstream file(filePath);
	if (!file.is_open()) return {};
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

void Server::handle_request(http::request<http::string_body> const& req, http::response<http::string_body>& res) {
	res.result(http::status::ok);
	res.set(http::field::content_type, "text/html");
	if (req.method() == http::verb::post) {
		std::string url_long = "";
		std::string user_url_short = "";
		try {
			boost::json::value jv = boost::json::parse(req.body());
			url_long = jv.at("url_long").as_string().c_str();
			user_url_short = jv.at("user_url_short").as_string().c_str();
		}
		catch (std::exception const& e) {
			std::cerr << "Error: " << e.what() << '\n';
			return;
		}
		pqxx::result existing_row = db_work.exec_params("SELECT * FROM urls WHERE url_long = $1", url_long);
		db_work.commit();
		if (!existing_row.empty()) {
			res.body() = existing_row.front().at(2).view();
		} else if (user_url_short != "") {
			pqxx::result existing_row = db_work.exec_params("SELECT * FROM urls WHERE url_short = $1", user_url_short);
			db_work.commit();
			if (existing_row.empty()) {
				db_work.exec_params("INSERT INTO urls (url_long, url_short) values ($1, $2)", url_long, user_url_short);
				db_work.commit();
				res.body() = user_url_short;
			}
			else {
				insertGeneratedURL(url_long, res);
			}
		}
		else
		{
			insertGeneratedURL(url_long, res);
		}
	}
	else {
		res.body() = loadFileToString("../client/index.html").value_or("");
	}

	res.prepare_payload();
}

void Server::insertGeneratedURL(const std::string& url_long, http::response<http::string_body>& res) {
	std::string shortUrl = generateShortUrl(url_long);
	res.body() = shortUrl;
	pqxx::result existing_row = db_work.exec_params("SELECT * FROM urls WHERE url_short = $1", shortUrl.c_str());
	db_work.commit();
	if (existing_row.empty()) {
		db_work.exec_params("INSERT INTO urls (url_long, url_short) values ($1, $2)", url_long.c_str(), shortUrl.c_str());
		db_work.commit();
	}
}

std::string Server::encodeBase64URL(uint64_t num) {
	const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

	uint8_t bytes[8];
	for (int i = 7; i >= 0; i--) {
		bytes[i] = num & 0xFF;
		num >>= 8;
	}

	std::string result;
	result.reserve(12);

	for (int i = 0; i < 8; i += 3) {
		uint32_t triplet = 0;
		int validBytes = 0;

		for (int j = 0; j < 3 && i + j < 8; j++) {
			triplet = (triplet << 8) | bytes[i + j];
			validBytes++;
		}

		triplet <<= (3 - validBytes) * 8;

		for (int j = 0; j < 4; j++) {
			if (j <= validBytes) {
				int index = (triplet >> (18 - j * 6)) & 0x3F;
				result += alphabet[index];
			}
		}
	}


	return result;
}
std::string Server::generateShortUrl(const std::string& url) {
	size_t hash = std::hash<std::string>{}(url);
	return encodeBase64URL(hash);
}