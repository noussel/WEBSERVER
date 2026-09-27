#include <iostream>
#include <string>
#include <vector>
#include "../includes/request-parse.hpp"

struct TestCase
{
    std::string name;
    std::string raw;
    int expectedStatus;
    std::string expectedMethod;
    std::string expectedPath;
    std::string expectedVersion;
    std::string expectedBody;
    size_t maxBodySize;
};

static TestCase makeTestCase(const std::string &name,
                             const std::string &raw,
                             int expectedStatus,
                             const std::string &expectedMethod,
                             const std::string &expectedPath,
                             const std::string &expectedVersion,
                             const std::string &expectedBody,
                             size_t maxBodySize)
{
    TestCase testCase;
    testCase.name = name;
    testCase.raw = raw;
    testCase.expectedStatus = expectedStatus;
    testCase.expectedMethod = expectedMethod;
    testCase.expectedPath = expectedPath;
    testCase.expectedVersion = expectedVersion;
    testCase.expectedBody = expectedBody;
    testCase.maxBodySize = maxBodySize;
    return testCase;
}

static bool runCase(const TestCase &testCase)
{
    HttpRequestParser parser;
    parser.info.client_max_body_size = testCase.maxBodySize;

    std::string raw = testCase.raw;
    bool parsed = parser.parseRequest(raw);

    bool ok = true;
    if (parser.getStatus() != testCase.expectedStatus)
    {
        std::cerr << "[FAIL] " << testCase.name << ": expected status "
                  << testCase.expectedStatus << ", got " << parser.getStatus() << std::endl;
        ok = false;
    }
    if (!testCase.expectedMethod.empty() && parser.getMethod() != testCase.expectedMethod)
    {
        std::cerr << "[FAIL] " << testCase.name << ": expected method '"
                  << testCase.expectedMethod << "', got '" << parser.getMethod() << "'" << std::endl;
        ok = false;
    }
    if (!testCase.expectedPath.empty() && parser.getPath() != testCase.expectedPath)
    {
        std::cerr << "[FAIL] " << testCase.name << ": expected path '"
                  << testCase.expectedPath << "', got '" << parser.getPath() << "'" << std::endl;
        ok = false;
    }
    if (!testCase.expectedVersion.empty() && parser.getVersion() != testCase.expectedVersion)
    {
        std::cerr << "[FAIL] " << testCase.name << ": expected version '"
                  << testCase.expectedVersion << "', got '" << parser.getVersion() << "'" << std::endl;
        ok = false;
    }
    if (!testCase.expectedBody.empty() && parser.getBody() != testCase.expectedBody)
    {
        std::cerr << "[FAIL] " << testCase.name << ": expected body '"
                  << testCase.expectedBody << "', got '" << parser.getBody() << "'" << std::endl;
        ok = false;
    }
    if (!parsed && testCase.expectedStatus == 200)
    {
        std::cerr << "[FAIL] " << testCase.name << ": parseRequest returned false for a valid request" << std::endl;
        ok = false;
    }
    if (parsed && testCase.expectedStatus != 200 && parser.getStatus() == 0)
    {
        std::cerr << "[FAIL] " << testCase.name << ": parsed request but status stayed 0" << std::endl;
        ok = false;
    }
    if (ok)
        std::cout << "[PASS] " << testCase.name << std::endl;
    return ok;
}

int main()
{
    std::vector<TestCase> tests;

    tests.push_back(makeTestCase("valid GET request",
                                 "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n",
                                 200,
                                 "GET",
                                 "/",
                                 "HTTP/1.1",
                                 "",
                                 100000));

    tests.push_back(makeTestCase("valid POST with body",
                                 "POST /upload HTTP/1.1\r\nHost: localhost\r\nContent-Length: 11\r\nContent-Type: text/plain\r\n\r\nhello world",
                                 200,
                                 "POST",
                                 "/upload",
                                 "HTTP/1.1",
                                 "hello world",
                                 100000));

    tests.push_back(makeTestCase("POST with no body",
                                 "POST /upload HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0\r\nContent-Type: text/plain\r\n\r\n",
                                 200,
                                 "POST",
                                 "/upload",
                                 "HTTP/1.1",
                                 "",
                                 100000));

    tests.push_back(makeTestCase("POST missing content length",
                                 "POST /upload HTTP/1.1\r\nHost: localhost\r\nContent-Type: text/plain\r\n\r\n",
                                 411,
                                 "POST",
                                 "/upload",
                                 "HTTP/1.1",
                                 "",
                                 100000));

    tests.push_back(makeTestCase("GET with body rejected",
                                 "GET / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\n\r\nhello",
                                 400,
                                 "GET",
                                 "/",
                                 "HTTP/1.1",
                                 "",
                                 100000));

    tests.push_back(makeTestCase("unsupported http version",
                                 "GET / HTTP/2.0\r\nHost: localhost\r\n\r\n",
                                 505,
                                 "GET",
                                 "/",
                                 "HTTP/2.0",
                                 "",
                                 100000));

    tests.push_back(makeTestCase("missing content length on post",
                                 "POST /upload HTTP/1.1\r\nHost: localhost\r\nContent-Type: text/plain\r\n\r\nhello",
                                 411,
                                 "POST",
                                 "/upload",
                                 "HTTP/1.1",
                                 "",
                                 100000));

    tests.push_back(makeTestCase("body too large",
                                 "POST /upload HTTP/1.1\r\nHost: localhost\r\nContent-Length: 11\r\nContent-Type: text/plain\r\n\r\nhello world",
                                 413,
                                 "POST",
                                 "/upload",
                                 "HTTP/1.1",
                                 "",
                                 5));

    size_t passed = 0;
    for (size_t i = 0; i < tests.size(); ++i)
    {
        if (runCase(tests[i]))
            ++passed;
    }

    std::cout << "\n" << passed << "/" << tests.size() << " tests passed" << std::endl;
    return (passed == tests.size()) ? 0 : 1;
}
