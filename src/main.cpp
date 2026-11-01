#include "headers.h"

#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <iostream>
#include <string_view>

using boost::asio::async_connect;
using boost::asio::async_read;
using boost::asio::async_read_until;
using boost::asio::async_write;
using boost::asio::awaitable;
using boost::asio::buffer;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::dynamic_buffer;
using boost::asio::io_service;
using boost::asio::transfer_at_least;
using boost::asio::use_awaitable;
using boost::asio::ip::tcp;
using boost::system::error_code;

constexpr std::string_view delimiter = "\r\n\r\n";

// При написании вдохнавлялся кодом из уроков. Там как раз был асинхронный сервер =)

awaitable<void> session(tcp::socket client_socket, io_service &io_service) {
    // code here
    try {
        std::string client_buf;
        size_t n = co_await async_read_until(client_socket, dynamic_buffer(client_buf), delimiter, use_awaitable);
        std::string_view client_headers(client_buf.data(), n);

        auto [host, port] = findHostPort(client_headers);
        if (host.empty()) {
            throw std::runtime_error("Missing Host header");
        }

        tcp::resolver resolver(io_service);
        auto endpoints = co_await resolver.async_resolve(host, port, use_awaitable);
        tcp::socket server_socket(io_service);
        co_await async_connect(server_socket, endpoints, use_awaitable);

        co_await async_write(server_socket, buffer(client_buf, n), use_awaitable);

        std::string server_buf;
        size_t m = co_await async_read_until(server_socket, dynamic_buffer(server_buf), delimiter, use_awaitable);
        std::string_view server_headers(server_buf.data(), m);

        auto content_length = findContentLength(server_headers);
        size_t body_size = content_length.value_or(0);

        // Записываем клиенту только заголовки (первые m байт).
        // read_until читает из сокета чанками и может уже прочитать в буфер часть тела
        // за заголовками (если они пришли одним пакетом). Эти лишние байты уже "съедены"
        // с сокета сервера, поэтому при передаче тела нужно их учесть, а не читать заново.
        co_await async_write(client_socket, buffer(server_buf.data(), m), use_awaitable);

        if (body_size > 0) {
            // Сколько байтов тела уже находится в буфере после заголовков
            size_t already_read = server_buf.size() - m;
            if (already_read > body_size) {
                already_read = body_size;
            }

            // Сначала отправляем клиенту уже прочитанные байты тела
            if (already_read > 0) {
                co_await async_write(client_socket, buffer(server_buf.data() + m, already_read), use_awaitable);
            }

            // Затем дочитываем оставшуюся часть тела с сервера
            size_t remaining = body_size - already_read;
            if (remaining > 0) {
                std::string body(remaining, '\0');
                co_await async_read(server_socket, buffer(body.data(), remaining), transfer_at_least(remaining),
                                    use_awaitable);
                co_await async_write(client_socket, buffer(body), use_awaitable);
            }
        }

    } catch (const std::exception &e) {
        std::cerr << "Session error: " << e.what() << std::endl;
    }
}

class Server {
public:
    Server(io_service &io_service, short port)
        : io_service_(io_service), acceptor_(io_service, tcp::endpoint(tcp::v4(), port)), socket_(io_service) {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(socket_, [this](error_code ec) {
            // code here
            if (!ec) {
                co_spawn(io_service_, session(std::move(socket_), io_service_), detached);
                socket_ = tcp::socket(io_service_);
            }
            do_accept();
        });
    }

    io_service &io_service_;
    tcp::acceptor acceptor_;
    tcp::socket socket_;
};

int main(int argc, char *argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: proxy_server";
            std::cerr << " <listen_port>\n";
            return 1;
        }
        io_service io_service(1);
        Server server(io_service, std::atoi(argv[1]));
        io_service.run();

    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}