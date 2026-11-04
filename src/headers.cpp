#include "headers.h"

#include <cctype>
#include <iostream>
#include <ranges>
#include <string_view>

using namespace std::string_view_literals;

using Callback = std::function<void(std::string_view, std::string_view)>;

std::string toLower(std::string_view s) {
    std::string result(s);
    for (auto &c : result) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

bool isValidPort(std::string_view port) {
    if (port.empty() || port.size() > 5) {
        return false;
    }
    for (char c : port) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    int value = std::stoi(std::string(port));
    return value >= 1 && value <= 65535;
}

void iterHeaders(std::string_view req, Callback &&callback) {
    // code here
    auto lines = req | std::views::split(std::string_view("\r\n"));

    bool first_line = true;
    for (auto &&line : lines) {
        std::string_view line_sv(line.begin(), line.end());

        if (line_sv.empty()) {
            break;
        }

        if (first_line) {
            first_line = false;
            continue;
        }

        auto colon_pos = line_sv.find(':');
        if (colon_pos == std::string_view::npos) {
            continue;
        }

        std::string_view name = line_sv.substr(0, colon_pos);
        std::string_view value = line_sv.substr(colon_pos + 1);

        while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
            value.remove_prefix(1);
        }

        callback(name, value);
    }
}

HostPort findHostPort(std::string_view req) {
    // code here
    HostPort result;

    iterHeaders(req, [&](std::string_view name, std::string_view value) {
        if (toLower(name) == "host") {
            // Обработка IPv6 адресов в квадратных скобках: [::1]:8080
            if (!value.empty() && value.front() == '[') {
                auto bracket_end = value.find(']');
                if (bracket_end != std::string_view::npos) {
                    result.host = std::string(value.substr(0, bracket_end + 1));
                    if (bracket_end + 1 < value.size() && value[bracket_end + 1] == ':') {
                        std::string port_str(value.substr(bracket_end + 2));
                        if (isValidPort(port_str)) {
                            result.port = port_str;
                        }
                    } else {
                        result.port = "80";
                    }
                } else {
                    result.host = std::string(value);
                    result.port = "80";
                }
            } else {
                auto colon_pos = value.rfind(':');
                if (colon_pos != std::string_view::npos) {
                    result.host = std::string(value.substr(0, colon_pos));
                    std::string port_str(value.substr(colon_pos + 1));
                    if (isValidPort(port_str)) {
                        result.port = port_str;
                    }
                } else {
                    result.host = std::string(value);
                    result.port = "80";
                }
            }
        }
    });

    return result;
}

std::optional<size_t> findContentLength(std::string_view rsp) {
    // code here
    std::optional<size_t> result = std::nullopt;

    iterHeaders(rsp, [&](std::string_view name, std::string_view value) {
        if (toLower(name) == "content-length") {
            constexpr size_t max_length_digits = 10;
            if (value.size() > max_length_digits) {
                std::cerr << "Content-Length value too long, ignoring" << std::endl;
                return;
            }
            try {
                size_t pos = 0;
                size_t val = std::stoul(std::string(value), &pos);
                result = val;
            } catch (const std::exception &e) {
                std::cerr << "Failed to parse Content-Length: " << e.what() << std::endl;
            }
        }
    });

    return result;
}
