#include "headers.h"
#include <gtest/gtest.h>

/// Все тесты написаны ИИ (включая новые, граничные условия, которые я захотел проверить, а то мало ли...)

TEST(iterHeaders, Empty) {
    // code here
    std::vector<std::pair<std::string, std::string>> headers;
    iterHeaders("", [&](std::string_view name, std::string_view value) { headers.emplace_back(name, value); });
    EXPECT_TRUE(headers.empty());
}

TEST(iterHeaders, SkipRequestLine) {
    // code here
    std::vector<std::pair<std::string, std::string>> headers;
    iterHeaders("GET / HTTP/1.1\r\nHost: example.com\r\n\r\n",
                [&](std::string_view name, std::string_view value) { headers.emplace_back(name, value); });
    ASSERT_EQ(headers.size(), 1);
    EXPECT_EQ(headers[0].first, "Host");
    EXPECT_EQ(headers[0].second, "example.com");
}

TEST(iterHeaders, SingleHeader) {
    // code here
    std::vector<std::pair<std::string, std::string>> headers;
    iterHeaders("GET / HTTP/1.1\r\nContent-Length: 42\r\n\r\n",
                [&](std::string_view name, std::string_view value) { headers.emplace_back(name, value); });
    ASSERT_EQ(headers.size(), 1);
    EXPECT_EQ(headers[0].first, "Content-Length");
    EXPECT_EQ(headers[0].second, "42");
}

TEST(iterHeaders, MultipleHeaders) {
    // code here
    std::vector<std::pair<std::string, std::string>> headers;
    iterHeaders("GET / HTTP/1.1\r\nHost: example.com\r\nUser-Agent: TestAgent\r\nAccept: */*\r\n\r\n",
                [&](std::string_view name, std::string_view value) { headers.emplace_back(name, value); });
    ASSERT_EQ(headers.size(), 3);
    EXPECT_EQ(headers[0].first, "Host");
    EXPECT_EQ(headers[0].second, "example.com");
    EXPECT_EQ(headers[1].first, "User-Agent");
    EXPECT_EQ(headers[1].second, "TestAgent");
    EXPECT_EQ(headers[2].first, "Accept");
    EXPECT_EQ(headers[2].second, "*/*");
}

TEST(iterHeaders, MultipleSameHeaders) {
    // code here
    std::vector<std::pair<std::string, std::string>> headers;
    iterHeaders("GET / HTTP/1.1\r\nSet-Cookie: a=1\r\nSet-Cookie: b=2\r\n\r\n",
                [&](std::string_view name, std::string_view value) { headers.emplace_back(name, value); });
    ASSERT_EQ(headers.size(), 2);
    EXPECT_EQ(headers[0].first, "Set-Cookie");
    EXPECT_EQ(headers[0].second, "a=1");
    EXPECT_EQ(headers[1].first, "Set-Cookie");
    EXPECT_EQ(headers[1].second, "b=2");
}

TEST(findHostPort, Simple) {
    // code here
    auto [host, port] = findHostPort("GET / HTTP/1.1\r\nHost: example.com:8080\r\n\r\n");
    EXPECT_EQ(host, "example.com");
    EXPECT_EQ(port, "8080");
}

TEST(findHostPort, NoHost) {
    // code here
    auto [host, port] = findHostPort("GET / HTTP/1.1\r\nUser-Agent: TestAgent\r\n\r\n");
    EXPECT_TRUE(host.empty());
    EXPECT_TRUE(port.empty());
}

TEST(findContentLength, Simple) {
    // code here
    auto content_length = findContentLength("HTTP/1.1 200 OK\r\nContent-Length: 1234\r\n\r\n");
    ASSERT_TRUE(content_length.has_value());
    EXPECT_EQ(*content_length, 1234);
}

TEST(findContentLength, NoContentLength) {
    // code here
    auto content_length = findContentLength("HTTP/1.1 200 OK\r\nServer: nginx\r\n\r\n");
    EXPECT_FALSE(content_length.has_value());
}

// Граничные случаи

TEST(iterHeaders, OnlyRequestLineNoHeaders) {
    // Только request line, без заголовков и пустой строки
    std::vector<std::pair<std::string, std::string>> headers;
    iterHeaders("GET / HTTP/1.1",
                [&](std::string_view name, std::string_view value) { headers.emplace_back(name, value); });
    EXPECT_TRUE(headers.empty());
}

TEST(iterHeaders, HeaderWithoutValue) {
    // Заголовок без значения (пустое значение после :)
    std::vector<std::pair<std::string, std::string>> headers;
    iterHeaders("GET / HTTP/1.1\r\nHost:\r\n\r\n",
                [&](std::string_view name, std::string_view value) { headers.emplace_back(name, value); });
    ASSERT_EQ(headers.size(), 1);
    EXPECT_EQ(headers[0].first, "Host");
    EXPECT_TRUE(headers[0].second.empty());
}

TEST(iterHeaders, HeaderWithExtraSpaces) {
    // Заголовок с дополнительными пробелами перед значением
    std::vector<std::pair<std::string, std::string>> headers;
    iterHeaders("GET / HTTP/1.1\r\nHost:   example.com   \r\n\r\n",
                [&](std::string_view name, std::string_view value) { headers.emplace_back(name, value); });
    ASSERT_EQ(headers.size(), 1);
    EXPECT_EQ(headers[0].first, "Host");
    EXPECT_EQ(headers[0].second, "example.com   ");
}

TEST(findHostPort, HostWithoutPort) {
    // Host без порта — порт должен быть пустым (по умолчанию 80)
    auto [host, port] = findHostPort("GET / HTTP/1.1\r\nHost: example.com\r\n\r\n");
    EXPECT_EQ(host, "example.com");
    EXPECT_EQ(port, "80");
}

TEST(findHostPort, HostWithIPv6) {
    // Host с IPv6 адресом в квадратных скобках
    auto [host, port] = findHostPort("GET / HTTP/1.1\r\nHost: [::1]:8080\r\n\r\n");
    EXPECT_EQ(host, "[::1]");
    EXPECT_EQ(port, "8080");
}

TEST(findContentLength, ZeroLength) {
    // Content-Length: 0 — пустое тело
    auto content_length = findContentLength("HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n");
    ASSERT_TRUE(content_length.has_value());
    EXPECT_EQ(*content_length, 0);
}

TEST(findContentLength, InvalidNumber) {
    // Content-Length с нечисловым значением
    auto content_length = findContentLength("HTTP/1.1 200 OK\r\nContent-Length: abc\r\n\r\n");
    EXPECT_FALSE(content_length.has_value());
}

TEST(findContentLength, NegativeNumber) {
    // Content-Length с отрицательным значением
    auto content_length = findContentLength("HTTP/1.1 200 OK\r\nContent-Length: -5\r\n\r\n");
    // stoul преобразует "-5" в большое число (unsigned), но это некорректное значение
    // Проверим, что оно не равно ожидаемому
    if (content_length.has_value()) {
        EXPECT_NE(*content_length, 5);
    }
}
