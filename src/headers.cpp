#include "headers.h"

#include <ranges>
#include <string_view>

using namespace std::string_view_literals;

using Callback = std::function<void(std::string_view, std::string_view)>;

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

std::pair<std::string, std::string> findHostPort(std::string_view req) {
    // code here
    std::pair<std::string, std::string> result{"", ""};

    iterHeaders(req, [&](std::string_view name, std::string_view value) {
        if (name.starts_with("Host")) {
            // Обработка IPv6 адресов в квадратных скобках: [::1]:8080
            if (!value.empty() && value.front() == '[') {
                auto bracket_end = value.find(']');
                if (bracket_end != std::string_view::npos) {
                    result.first = std::string(value.substr(0, bracket_end + 1));
                    if (bracket_end + 1 < value.size() && value[bracket_end + 1] == ':') {
                        result.second = std::string(value.substr(bracket_end + 2));
                    } else {
                        result.second = "80";
                    }
                } else {
                    result.first = std::string(value);
                    result.second = "80";
                }
            } else {
                auto colon_pos = value.rfind(':');
                if (colon_pos != std::string_view::npos) {
                    result.first = std::string(value.substr(0, colon_pos));
                    result.second = std::string(value.substr(colon_pos + 1));
                } else {
                    result.first = std::string(value);
                    result.second = "80";
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
        if (name.starts_with("Content-Length")) {
            try {
                size_t pos = 0;
                size_t val = std::stoul(std::string(value), &pos);
                result = val;
            } catch (...) {
            }
        }
    });

    return result;
}