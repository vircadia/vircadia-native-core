//
// UrlReader.cpp
// hifi
//
// Created by Tobias Schwinger on 3/21/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//


#include "UrlReader.h"

#include <new>
#ifdef _WIN32
#define NOCURL_IN_WINDOWS
#endif
#ifndef NOCURL_IN_WINDOWS
#include <curl/curl.h>
size_t const UrlReader::max_read_ahead = CURL_MAX_WRITE_SIZE;
#else
size_t const UrlReader::max_read_ahead = 0;
#endif

char const* const UrlReader::success = "UrlReader: Success!";
char const* const UrlReader::error_init_failed      = "UrlReader: Initialization failed.";
char const* const UrlReader::error_aborted          = "UrlReader: Processing error.";
char const* const UrlReader::error_buffer_overflow  = "UrlReader: Buffer overflow.";
char const* const UrlReader::error_leftover_input   = "UrlReader: Incomplete processing.";

#define hnd_curl static_cast<CURL*>(_ptrImpl)

UrlReader::UrlReader()
    : _ptrImpl(0l), _arrXtra(0l), _strError(0l) {

    _arrXtra = new(std::nothrow) char[max_read_ahead];
    if (! _arrXtra) { _strError = error_init_failed; return; }
#ifndef NOCURL_IN_WINDOWS
    _ptrImpl =  curl_easy_init();
    if (! _ptrImpl) { _strError = error_init_failed; return; }
    curl_easy_setopt(hnd_curl, CURLOPT_NOSIGNAL, 1l);
    curl_easy_setopt(hnd_curl, CURLOPT_FAILONERROR, 1l);
    curl_easy_setopt(hnd_curl, CURLOPT_FILETIME, 1l);
#endif
}

UrlReader::~UrlReader() {

    delete _arrXtra;
#ifndef NOCURL_IN_WINDOWS
    if (! hnd_curl) return;
    curl_easy_cleanup(hnd_curl);
#endif
}

bool UrlReader::perform(char const* url, transfer_callback* cb) {
#ifndef NOCURL_IN_WINDOWS

    curl_easy_setopt(hnd_curl, CURLOPT_URL, url);
    curl_easy_setopt(hnd_curl, CURLOPT_WRITEFUNCTION, cb);
    curl_easy_setopt(hnd_curl, CURLOPT_WRITEDATA, this);

    CURLcode rc = curl_easy_perform(hnd_curl);

    if (rc == CURLE_OK)
    {
        while (_valXtraSize > 0 && _strError == success)
            cb(0l, 0, 0, this);
    }
    else if (_strError == success)
        _strError = curl_easy_strerror(rc);

    return rc == CURLE_OK;
#else
    return false;
#endif
}

void UrlReader::getinfo(char const*& url,
        char const*& type, int64_t& length, int64_t& stardate) {
#ifndef NOCURL_IN_WINDOWS

    curl_easy_getinfo(hnd_curl, CURLINFO_EFFECTIVE_URL, & url);
    curl_easy_getinfo(hnd_curl, CURLINFO_CONTENT_TYPE, & type);

    double clen;
    curl_easy_getinfo(hnd_curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, & clen);
    length = static_cast<int64_t>(clen);

    long time;
    curl_easy_getinfo(hnd_curl, CURLINFO_FILETIME, & time);
    stardate = time;
#endif
}

