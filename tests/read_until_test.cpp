#include <boost/asio.hpp>

#include <chrono>
#include <gtest/gtest.h>
#include <string>
#include <thread>

using namespace boost::asio;
using boost::asio::ip::tcp;
using boost::system::error_code;

// При усердном имении (в двух смыслах) дела с тестами на python3 и wget был обнаружен данный баг. Пришлось делать
// надстройку и изучать почему так происходит. Это выродилось в исправления в коде, для учета этого кейса и к написанию
// отдельного теста усилиями ИИ.

// Документирует поведение read_until с dynamic_buffer: возвращаемое значение n —
// это только байты до и включая разделитель, но сам буфер может содержать дополнительные
// байты тела, которые были прочитаны из сокета вместе с заголовками (если они пришли
// одним пакетом). Это важно для корректности проксирования: эти лишние байты уже
// "съедены" с сокета, поэтому при передаче тела их нужно учесть, а не читать заново.
TEST(ReadUntil, BufferMayContainExtraBodyBytes) {
    io_context io(1);
    tcp::acceptor acc(io, tcp::endpoint(tcp::v4(), 0));
    auto port = acc.local_endpoint().port();

    // Сервер принимает соединение и отправляет заголовки + тело одним пакетом,
    // чтобы имитировать ситуацию, когда TCP доставляет всё сразу.
    std::thread server_thread([&]() {
        tcp::socket s(io);
        error_code ec;
        acc.accept(s, ec);
        if (ec)
            return;
        std::string resp = "HTTP/1.1 200 OK\r\nContent-Length: 100\r\n\r\n";
        for (int i = 0; i < 100; ++i)
            resp += 'A';
        // Отправляем всё одним вызовом, чтобы данные пришли одним чанком
        boost::asio::write(s, buffer(resp));
        s.close();
    });
    server_thread.detach();

    // Даём серверу время на accept
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    tcp::socket sock(io);
    error_code ec;
    sock.connect(tcp::endpoint(tcp::v4(), port), ec);
    ASSERT_FALSE(ec) << "connect failed: " << ec.message();

    std::string buf;
    size_t n = read_until(sock, dynamic_buffer(buf), "\r\n\r\n");

    // n — количество байт до и включая разделитель (только заголовки)
    EXPECT_EQ(n, 40) << "read_until should return header length";

    // Буфер может содержать дополнительные байты тела (если заголовки и тело пришли
    // одним пакетом). Это нормальное поведение read_until с dynamic_buffer.
    // Именно поэтому в session() нужно учитывать уже прочитанные байты тела.
    EXPECT_GE(buf.size(), n) << "Buffer must contain at least the headers";
}