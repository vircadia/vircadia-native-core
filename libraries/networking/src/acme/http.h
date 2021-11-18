#ifndef VIRCADIA_LIBRARIES_NETWORKING_SRC_ACME_HTTP_H
#define VIRCADIA_LIBRARIES_NETWORKING_SRC_ACME_HTTP_H

#include <string>
#include <vector>

namespace acme_lw_internal
{

template <typename Callback>
void getHeader(Callback, const std::string& url, const char* headerKey);

template <typename Callback>
void doGet(Callback, const std::string& url);

struct Response
{
    // TODO: these are not private, remove underscores
    std::vector<char>   response_;
    std::string         headerValue_;
};

template <typename Callback>
void doPost(Callback, const std::string& url, const std::string& postBody, const char * headerKey = nullptr);

}

#endif /* end of include guard */

