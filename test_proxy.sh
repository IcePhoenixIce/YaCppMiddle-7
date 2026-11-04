#!/bin/bash
# Интеграционные тесты прокси-сервера с использованием wget и python3
#
# Перед запуском:
# 1. Собрать проект: cmake --build build
# 2. Запустить локальный HTTP-сервер (для тестов без интернета):
#    ./local_test_server.sh &
# 3. Запустить прокси-сервер в отдельном терминале: ./build/AsyncHttpProxy 8080
# 4. Запустить этот скрипт: ./test_proxy.sh
#
# Альтернатива: если есть доступ к интернету, можно изменить USE_LOCAL_SERVER=false
# и тестировать напрямую к http://example.com без локального сервера.

PROXY_PORT=8080
LOCAL_PORT=9999
USE_LOCAL_SERVER=true  # false - если есть доступ к интернету

if [ "$USE_LOCAL_SERVER" = true ]; then
    TARGET="http://127.0.0.1:${LOCAL_PORT}"
else
    TARGET="http://example.com"
fi

# Тест 1: Обычный запрос через wget
test_wget() {
    echo "=== Test 1: wget - ${TARGET}/ ==="
    local result_file=$(mktemp)
    http_proxy=127.0.0.1:${PROXY_PORT} timeout 10 wget -q -O "${result_file}" "${TARGET}/" 2>&1
    local exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        local size=$(wc -c < "${result_file}")
        if [ "$size" -gt 0 ]; then
            echo "PASS: Got response (${size} bytes)"
        else
            echo "FAIL: Empty response"
            rm -f "${result_file}"
            return 1
        fi
    else
        echo "FAIL: wget exited with code $exit_code"
        rm -f "${result_file}"
        return 1
    fi
    
    rm -f "${result_file}"
    return 0
}

# Тест 2: Обычный запрос через python3
test_python() {
    echo "=== Test 2: python3 - ${TARGET}/ ==="
    local result=$(python3 -c "
import urllib.request, sys
proxy_handler = urllib.request.ProxyHandler({'http': 'http://127.0.0.1:${PROXY_PORT}'})
opener = urllib.request.build_opener(proxy_handler)
try:
    response = opener.open('${TARGET}/', timeout=10)
    data = response.read()
    print(f'Status: {response.status}')
    print(f'Content size: {len(data)} bytes')
    print('SUCCESS' if len(data) > 0 else 'FAIL')
except Exception as e:
    print(f'FAIL: {e}')
" 2>&1)
    
    if [[ "$result" == *"SUCCESS"* ]]; then
        echo "PASS: Python3 test passed"
    else
        echo "FAIL: Python3 test failed"
        echo "$result"
        return 1
    fi
    
    return 0
}

# Основной сценарий теста
main() {
    echo "Starting integration tests..."
    echo ""
    echo "Note: Make sure proxy is running on port ${PROXY_PORT} before running this script."
    if [ "$USE_LOCAL_SERVER" = true ]; then
        echo "Note: Make sure local test server is running on port ${LOCAL_PORT}."
    fi
    echo ""
    
    local passed=0
    local failed=0
    
    if test_wget; then
        passed=$((passed + 1))
    else
        failed=$((failed + 1))
    fi
    echo ""
    if test_python; then
        passed=$((passed + 1))
    else
        failed=$((failed + 1))
    fi
    echo ""
    
    echo "=================================="
    echo "Test Results: ${passed} passed, ${failed} failed"
    echo "=================================="
    
    if [ $failed -gt 0 ]; then
        exit 1
    fi
}

main "$@"
