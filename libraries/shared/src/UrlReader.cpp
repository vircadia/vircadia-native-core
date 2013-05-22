//
// UrlReader.cpp
// hifi
//
// Created by Tobias Schwinger on 3/21/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "UrlReader.h"

#include <new>

#include <sys/types.h>
#include <sys/stat.h>

#include "Log.h"

#ifndef _WIN32
// (Windows port is incomplete and the build files do not support CURL, yet)

#include <curl/curl.h>


//
// ATTENTION: A certain part of the implementation lives in inlined code
// (see the bottom of the header file).
//
// Why? Because it allows stream parsing without having to call around a
// lot (one static and one dynamic call per character if the parser just
// reads one character at a time).
//
// Here is an overview of the code structure:
// 
// readUrl 
//  -> transferBegin (sets up state)
//  -> perform (starts CURL transfer) 
//      -> (specialized, type-erased) callback_template
//          -> getInfo (fetches HTTP header, eventually initiates caching)
//          -> stream.begin (client code - called once)
//          -> feedBuffered (the buffering logic)
//              -> stream.transfer (client code - called repeatedly)
//  -> stream.end (client code - called when the transfer is done)
//  -> transferEnd (closes cache file, if used)
//
//  "->" means "calls or inlines", here
//

size_t const UrlReader::max_read_ahead = CURL_MAX_WRITE_SIZE;

char const* const UrlReader::success                = "UrlReader: Success!";
char const* const UrlReader::success_cached         = "UrlReader: Using local file.";
char const* const UrlReader::error_init_failed      = "UrlReader: Initialization failed.";
char const* const UrlReader::error_aborted          = "UrlReader: Processing error.";
char const* const UrlReader::error_buffer_overflow  = "UrlReader: Buffer overflow.";
char const* const UrlReader::error_leftover_input   = "UrlReader: Incomplete processing.";

#define _curlPtr static_cast<CURL*>(_curlHandle)

UrlReader::UrlReader()
    : _curlHandle(0l), _xtraBuffer(0l), _errorStr(0l), _cacheReadBuffer(0l) {

    _xtraBuffer = new(std::nothrow) char[max_read_ahead];
    if (! _xtraBuffer) { _errorStr = error_init_failed; return; }
    _curlHandle =  curl_easy_init();
    if (! _curlHandle) { _errorStr = error_init_failed; return; }
    curl_easy_setopt(_curlPtr, CURLOPT_NOSIGNAL, 1l);
    curl_easy_setopt(_curlPtr, CURLOPT_FAILONERROR, 1l);
    curl_easy_setopt(_curlPtr, CURLOPT_FILETIME, 1l);
    curl_easy_setopt(_curlPtr, CURLOPT_ENCODING, ""); 
}

UrlReader::~UrlReader() {

    delete[] _xtraBuffer;
    delete[] _cacheReadBuffer;
    if (! _curlHandle) {
        return;
    }
    curl_easy_cleanup(_curlPtr);
}

void UrlReader::perform(char const* url, transfer_callback* cb) {

    curl_easy_setopt(_curlPtr, CURLOPT_URL, url);
    curl_easy_setopt(_curlPtr, CURLOPT_WRITEFUNCTION, cb);
    curl_easy_setopt(_curlPtr, CURLOPT_WRITEDATA, this);

    CURLcode rc = curl_easy_perform(_curlPtr);

    if (rc == CURLE_OK)
    {
        while (_xtraSize > 0 && _errorStr == success)
            cb(0l, 0, 0, this);
    }
    else if (_errorStr == success)
        _errorStr = curl_easy_strerror(rc);
}

void UrlReader::transferBegin(void* stream, char const* cacheFile) {

    _errorStr = success;
    _streamPtr = stream;
    _cacheFileName = cacheFile;
    _cacheFile = 0l;
    _cacheMode = no_cache; 
    _xtraSize = ~size_t(0);
}

void UrlReader::getInfo(char const*& url,
        char const*& type, int64_t& length, int64_t& stardate) {

    // fetch information from HTTP header
    double clen;
    long time;
    curl_easy_getinfo(_curlPtr, CURLINFO_FILETIME, & time);
    curl_easy_getinfo(_curlPtr, CURLINFO_EFFECTIVE_URL, & url);
    curl_easy_getinfo(_curlPtr, CURLINFO_CONTENT_TYPE, & type);
    curl_easy_getinfo(_curlPtr, CURLINFO_CONTENT_LENGTH_DOWNLOAD, & clen);
    length = static_cast<int64_t>(clen);
    curl_easy_getinfo(_curlPtr, CURLINFO_FILETIME, & time);
    stardate = time;

//    printLog("UrlReader: Ready to transfer from URL '%s'\n", url);

    // check caching file time whether we actually want to download anything
    if (_cacheFileName != 0l) {
        struct stat s;
        stat(_cacheFileName, & s);
        if (time > s.st_mtime) {
            // file on server is newer -> update cache file
            _cacheFile = fopen(_cacheFileName, "wb");
//            printLog("UrlReader: Also writing content to cache file '%s'\n", _cacheFileName);
            if (_cacheFile != 0l) {
                _cacheMode = cache_write;
            }
        } else {
            // file on server is older -> use cache file
            if (! _cacheReadBuffer) {
                _cacheReadBuffer = new (std::nothrow) char[max_read_ahead];
                if (! _cacheReadBuffer) {
                    // out of memory, no caching, have CURL catch it
                    return; 
                }
            }
            _cacheFile = fopen(_cacheFileName, "rb");
//            printLog("UrlReader: Delivering cached content from file '%s'\n", _cacheFileName);
            if (_cacheFile != 0l) {
                _cacheMode = cache_read;
            }
            // override error code returned by CURL when we abort the download
            _errorStr = success_cached;
        }
    }
}

void UrlReader::transferEnd() {

    if (_cacheFile != 0l) {
        fclose(_cacheFile);
    }
}

#else // no-op version for incomplete Windows build:

UrlReader::UrlReader() : _curlHandle(0l) { }
UrlReader::~UrlReader() { }
void UrlReader::perform(char const* url, transfer_callback* cb) { }
void UrlReader::transferBegin(void* stream, char const* cacheFile) { }
void UrlReader::getInfo(char const*& url, char const*& type, 
                        int64_t& length, int64_t& stardate) { }
void UrlReader::transferEnd() { }

#endif


