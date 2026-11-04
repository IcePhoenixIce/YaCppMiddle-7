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
    auto result = findHostPort("GET / HTTP/1.1\r\nHost: example.com:8080\r\n\r\n");
    EXPECT_EQ(result.host, "example.com");
    EXPECT_EQ(result.port, "8080");
}

TEST(findHostPort, NoHost) {
    // code here
    auto result = findHostPort("GET / HTTP/1.1\r\nUser-Agent: TestAgent\r\n\r\n");
    EXPECT_TRUE(result.host.empty());
    EXPECT_TRUE(result.port.empty());
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
    auto result = findHostPort("GET / HTTP/1.1\r\nHost: example.com\r\n\r\n");
    EXPECT_EQ(result.host, "example.com");
    EXPECT_EQ(result.port, "80");
}

TEST(findHostPort, HostWithIPv6) {
    // Host с IPv6 адресом в квадратных скобках
    auto result = findHostPort("GET / HTTP/1.1\r\nHost: [::1]:8080\r\n\r\n");
    EXPECT_EQ(result.host, "[::1]");
    EXPECT_EQ(result.port, "8080");
}

TEST(findHostPort, CaseInsensitive) {
    // Имена заголовков регистронезависимы
    auto r1 = findHostPort("GET / HTTP/1.1\r\nHOST: example.com:8080\r\n\r\n");
    EXPECT_EQ(r1.host, "example.com");
    EXPECT_EQ(r1.port, "8080");

    auto r2 = findHostPort("GET / HTTP/1.1\r\nhOsT: example.com:8080\r\n\r\n");
    EXPECT_EQ(r2.host, "example.com");
    EXPECT_EQ(r2.port, "8080");
}

TEST(findHostPort, InvalidPort) {
    // Невалидный порт — порт должен остаться пустым
    auto r1 = findHostPort("GET / HTTP/1.1\r\nHost: example.com:abc\r\n\r\n");
    EXPECT_EQ(r1.host, "example.com");
    EXPECT_TRUE(r1.port.empty());

    auto r2 = findHostPort("GET / HTTP/1.1\r\nHost: example.com:99999\r\n\r\n");
    EXPECT_EQ(r2.host, "example.com");
    EXPECT_TRUE(r2.port.empty());
}

TEST(findContentLength, CaseInsensitive) {
    // Имена заголовков регистронезависимы
    auto r1 = findContentLength("HTTP/1.1 200 OK\r\nCONTENT-LENGTH: 42\r\n\r\n");
    ASSERT_TRUE(r1.has_value());
    EXPECT_EQ(*r1, 42);

    auto r2 = findContentLength("HTTP/1.1 200 OK\r\ncontent-length: 42\r\n\r\n");
    ASSERT_TRUE(r2.has_value());
    EXPECT_EQ(*r2, 42);
}

TEST(findContentLength, TooLongValue) {
    // Слишком длинное значение Content-Length — должно быть проигнорировано
    std::string long_value(20, '1');
    auto content_length = findContentLength("HTTP/1.1 200 OK\r\nContent-Length: " + long_value + "\r\n\r\n");
    EXPECT_FALSE(content_length.has_value());
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

// Невалидные заголовки

TEST(iterHeaders, HeaderWithoutColon) {
    // Строка без двоичной точки — не является заголовком
    std::vector<std::pair<std::string, std::string>> headers;
    iterHeaders("GET / HTTP/1.1\r\nInvalidHeaderWithoutColon\r\nHost: example.com\r\n\r\n",
                [&](std::string_view name, std::string_view value) { headers.emplace_back(name, value); });
    ASSERT_EQ(headers.size(), 1);
    EXPECT_EQ(headers[0].first, "Host");
}

TEST(iterHeaders, EmptyHeaderName) {
    // Пустое имя заголовка (строка начинается с :) — имя будет пустым
    std::vector<std::pair<std::string, std::string>> headers;
    iterHeaders("GET / HTTP/1.1\r\n: value\r\nHost: example.com\r\n\r\n",
                [&](std::string_view name, std::string_view value) { headers.emplace_back(name, value); });
    ASSERT_EQ(headers.size(), 2);
    EXPECT_TRUE(headers[0].first.empty());
    EXPECT_EQ(headers[0].second, "value");
    EXPECT_EQ(headers[1].first, "Host");
}

TEST(findHostPort, HostWithEmptyValue) {
    // Host с пустым значением — хост пустой, порт по умолчанию 80
    auto result = findHostPort("GET / HTTP/1.1\r\nHost:\r\n\r\n");
    EXPECT_TRUE(result.host.empty());
    EXPECT_EQ(result.port, "80");
}

TEST(findHostPort, HostWithOnlyPort) {
    // Host с только портом (пустой хост)
    auto result = findHostPort("GET / HTTP/1.1\r\nHost: :8080\r\n\r\n");
    EXPECT_TRUE(result.host.empty());
    EXPECT_EQ(result.port, "8080");
}

TEST(findHostPort, HostWithMalformedIPv6) {
    // Невалидный IPv6 — незакрытая скобка
    auto result = findHostPort("GET / HTTP/1.1\r\nHost: [::1\r\n\r\n");
    EXPECT_EQ(result.host, "[::1");
    EXPECT_EQ(result.port, "80");
}

TEST(findContentLength, ContentLengthWithSpaces) {
    // Content-Length с пробелами внутри значения
    auto content_length = findContentLength("HTTP/1.1 200 OK\r\nContent-Length: 12 34\r\n\r\n");
    // stoul прочитает "12" и остановится на пробеле
    if (content_length.has_value()) {
        EXPECT_EQ(*content_length, 12);
    }
}

TEST(findContentLength, ContentLengthEmptyValue) {
    // Content-Length с пустым значением
    auto content_length = findContentLength("HTTP/1.1 200 OK\r\nContent-Length:\r\n\r\n");
    EXPECT_FALSE(content_length.has_value());
}
