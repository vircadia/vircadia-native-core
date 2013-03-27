//
// UrlReader.cpp
// hifi
//
// Created by Tobias Schwinger on 3/21/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//


#include "UrlReader.h"

#include <new>
#include <curl/curl.h>

size_t const UrlReader::max_read_ahead = CURL_MAX_WRITE_SIZE;

char const* const UrlReader::success = "UrlReader: Success!";
char const* const UrlReader::error_init_failed      = "UrlReader: Initialization failed.";
char const* const UrlReader::error_aborted          = "UrlReader: Processing error.";
char const* const UrlReader::error_buffer_overflow  = "UrlReader: Buffer overflow.";
char const* const UrlReader::error_leftover_input   = "UrlReader: Incomplete processing.";

#define hnd_curl static_cast<CURL*>(ptr_impl)

UrlReader::UrlReader()
    : ptr_impl(0l), ptr_ra(0l), str_error(0l)
{
    ptr_ra = new(std::nothrow) char[max_read_ahead];
    if (! ptr_ra) { str_error = error_init_failed; return; }
    ptr_impl =  curl_easy_init();
    if (! ptr_impl) { str_error = error_init_failed; return; }
    curl_easy_setopt(hnd_curl, CURLOPT_NOSIGNAL, 1l);
    curl_easy_setopt(hnd_curl, CURLOPT_FAILONERROR, 1l);
    curl_easy_setopt(hnd_curl, CURLOPT_FILETIME, 1l);
}

UrlReader::~UrlReader()
{
    delete ptr_ra;
    if (! hnd_curl) return;
    curl_easy_cleanup(hnd_curl);
}

bool UrlReader::perform(char const* url, transfer_callback* cb)
{
    curl_easy_setopt(hnd_curl, CURLOPT_URL, url);
    curl_easy_setopt(hnd_curl, CURLOPT_WRITEFUNCTION, cb);
    curl_easy_setopt(hnd_curl, CURLOPT_WRITEDATA, this);

    CURLcode rc = curl_easy_perform(hnd_curl);

    if (rc == CURLE_OK)
    {
        while (val_ra_size > 0 && str_error == success)
            cb(0l, 0, 0, this);
    }
    else if (str_error == success)
        str_error = curl_easy_strerror(rc);

    return rc == CURLE_OK;
}

void UrlReader::getinfo(char const*& url,
        char const*& type, int64_t& length, int64_t& stardate)
{
    curl_easy_getinfo(hnd_curl, CURLINFO_EFFECTIVE_URL, & url);
    curl_easy_getinfo(hnd_curl, CURLINFO_CONTENT_TYPE, & type);

    double clen;
    curl_easy_getinfo(hnd_curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, & clen);
    length = static_cast<int64_t>(clen);

    long time;
    curl_easy_getinfo(hnd_curl, CURLINFO_FILETIME, & time);
    stardate = time;
}

