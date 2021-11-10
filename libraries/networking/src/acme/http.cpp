#include "http.h"

#include "acme-exception.h"

#include <curl/curl.h>

#include <cstring>

using namespace std;

using namespace acme_lw;

namespace
{

struct Ptr
{
    Ptr()
        : curl_(curl_easy_init())
    {
        if (!curl_)
        {
            throw acme_lw::AcmeException("Error initializing curl");
        }
    }

    ~Ptr()
    {
        curl_easy_cleanup(curl_);
    }

    CURL * operator * () const
    {
        return curl_;
    }

private:
    CURL * curl_;
};

size_t headerCallback(void * buffer, size_t size, size_t nmemb, void * h)
{
    // header -> 'key': 'value'
    pair<string, string>& header = *reinterpret_cast<pair<string, string> *>(h);

    size_t byteCount = size * nmemb;
    if (byteCount >= header.first.size())
    {
        // Header names are case insensitive per RFC 7230. Let's Encrypt's move
        // to a CDN made the headers lower case.
        if (!strncasecmp(header.first.c_str(), reinterpret_cast<const char *>(buffer), header.first.size()))
        {
            string line(reinterpret_cast<const char *>(buffer), byteCount);

            // Header looks like 'X: Y'. This gets the 'Y'
            auto pos = line.find(": ");
            if (pos != string::npos)
            {
                string value = line.substr(pos + 2, byteCount - pos - 2);

                // Trim trailing whitespace
                header.second = value.erase(value.find_last_not_of(" \n\r") + 1);
            }
        }
    }

    return byteCount;
}

size_t dataCallback(void * buffer, size_t size, size_t nmemb, void * response)
{
    vector<char>& v = *reinterpret_cast<vector<char> *>(response);

    size_t byteCount = size * nmemb;
    string s(reinterpret_cast<const char *>(buffer), byteCount);

    size_t initSize = v.size();
    v.resize(v.size() + byteCount);
    memcpy(&v[initSize], buffer, byteCount);

    return byteCount;
}

string getCurlError(const string& s, CURLcode c)
{
    return s + ": " + curl_easy_strerror(c);
}

}

namespace acme_lw_internal
{

void initHttp()
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void teardownHttp()
{
    curl_global_cleanup();
}

void doCurl(Ptr& curl, const string& url, const vector<char>& response)
{
    auto res = curl_easy_perform(*curl);
    if (res != CURLE_OK)
    {
        throw AcmeException(getCurlError("Failure contacting "s + url +" to read a header.", res));
    }

    long responseCode;
    curl_easy_getinfo(*curl, CURLINFO_RESPONSE_CODE, &responseCode);
    if (responseCode / 100 != 2)
    {
        // If it's not a 2xx response code, throw.
        throw AcmeException("Response code of "s + to_string(responseCode) + " contacting " + url + 
                            " with response of:\n" + string(&response.front(), response.size()));
    }
}

string getHeader(const string& url, const string& headerKey)
{
    Ptr curl;
    curl_easy_setopt(*curl, CURLOPT_URL, url.c_str());

    // Does a HEAD request
    curl_easy_setopt(*curl, CURLOPT_NOBODY, 1);

    curl_easy_setopt(*curl, CURLOPT_HEADERFUNCTION, &headerCallback);

    pair<string, string> header = make_pair(headerKey, ""s);
    curl_easy_setopt(*curl, CURLOPT_HEADERDATA, &header);

    // There will be no response (probably). We just pass this
    // for error handling
    vector<char> response;
    doCurl(curl, url, response);

    return header.second;
}

Response doPost(const string& url, const string& postBody, const char * headerKey)
{
    Response response;

    Ptr curl;

    curl_easy_setopt(*curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(*curl, CURLOPT_POST, 1);
    curl_easy_setopt(*curl, CURLOPT_POSTFIELDS, postBody.c_str());
    curl_easy_setopt(*curl, CURLOPT_WRITEFUNCTION, dataCallback);
    curl_easy_setopt(*curl, CURLOPT_WRITEDATA, &response.response_);

    curl_slist h = { const_cast<char *>("Content-Type: application/jose+json"), nullptr };
    curl_easy_setopt(*curl, CURLOPT_HTTPHEADER, &h);

    pair<string, string> header;
    if (headerKey)
    {
        curl_easy_setopt(*curl, CURLOPT_HEADERFUNCTION, &headerCallback);

        header = make_pair(headerKey, ""s);
        curl_easy_setopt(*curl, CURLOPT_HEADERDATA, &header);
    }

    doCurl(curl, url, response.response_);

    response.headerValue_ = header.second;

    return response;
}

vector<char> doGet(const string& url)
{
    vector<char> response;

    Ptr curl;

    curl_easy_setopt(*curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(*curl, CURLOPT_WRITEFUNCTION, dataCallback);
    curl_easy_setopt(*curl, CURLOPT_WRITEDATA, &response);

    doCurl(curl, url, response);

    return response;
}

}

