#pragma once

#include <string>
#include <vector>

namespace acme_lw_internal
{

std::string       getHeader(const std::string& url, const std::string& header);
std::vector<char> doGet(const std::string& url);

struct Response
{
    std::vector<char>   response_;
    std::string         headerValue_;
};

Response doPost(const std::string& url, const std::string& postBody, const char * headerKey = nullptr);

void initHttp();
void teardownHttp();

}
