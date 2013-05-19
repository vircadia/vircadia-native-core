//
// UrlReader.h
// hifi
//
// Created by Tobias Schwinger on 3/21/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__UrlReader__
#define __hifi__UrlReader__

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
 
// 
// UrlReader class that encapsulates a context for sequential data retrieval
// via URLs. Use one per thread.
// 
class UrlReader {
public:

    // 
    // Constructor - performs initialization, never throws.
    // 
    UrlReader();

    // 
    // Destructor - frees resources, never throws.
    // 
    ~UrlReader();

    // 
    // Reads data from an URL and forwards it to the instance of a class
    // fulfilling the ContentStream concept.
    // 
    // The call protocol on the ContentStream is detailed as follows:
    // 
    // 1. begin(char const* url, 
    //      char const* content_type, uint64_t bytes, uint64_t stardate)
    //
    //    All information except 'url' is optional; 'content_type' can
    //    be a null pointer - 'bytes' and 'stardate' can be equal to
    //    to 'unavailable'.
    //
    // 2. transfer(char* buffer, size_t bytes)
    //    
    //    Called until all data has been received. The number of bytes
    //    actually processed should be returned.
    //    Unprocessed data is stored in an extra buffer whose size is
    //    given by the constant UrlReader::max_read_ahead - it can be
    //    assumed to be reasonably large for on-the-fly parsing.
    //
    // 3. end(bool ok)
    // 
    //    Called at the end of the transfer.
    //
    // Returns the same success code 
    //  
    template< class ContentStream >
    bool readUrl(char const* url, ContentStream& s, char const* cacheFile = 0l);

    // 
    // Returns a pointer to a static C-string that describes the error
    // condition.
    // 
    inline char const* getError() const;

    // 
    // Can be called by the stream to set a user-defined error string.
    // 
    inline void setError(char const* static_c_string);

    // 
    // Pointer to the C-string returned by a call to 'readUrl' when no
    // error occurred.
    // 
    static char const* const success;

    // 
    // Pointer to the C-string returned by a call to 'readUrl' when no
    // error occurred and a local file has been read instead of the
    // network stream.
    // 
    static char const* const success_cached;

    // 
    // Pointer to the C-string returned by a call to 'readUrl' when the
    // initialization has failed.
    // 
    static char const* const error_init_failed;

    // 
    // Pointer to the C-string returned by a call to 'readUrl' when the
    // transfer has been aborted by the client.
    // 
    static char const* const error_aborted;

    // 
    // Pointer to the C-string returned by a call to 'readUrl' when 
    // leftover input from incomplete processing caused a buffer
    // overflow.
    // 
    static char const* const error_buffer_overflow;

    // 
    // Pointer to the C-string return by a call to 'readUrl' when the
    // input provided was not completely consumed.
    // 
    static char const* const error_leftover_input;

    // 
    // Constant of the maximum number of bytes that are buffered
    // between invocations of 'transfer'.
    // 
    static size_t const max_read_ahead;

    // 
    // Constant representing absent information in the call to the
    // 'begin' member function of the target stream.
    // 
    static int const unavailable = -1;

    // 
    // Constant for requesting to abort the current transfer when
    // returned by the 'transfer' member function of the target stream.
    // 
    static size_t const abort = ~0u;

private:
    // instances of this class shall not be copied
    UrlReader(UrlReader const&); // = delete;
    UrlReader& operator=(UrlReader const&); // = delete;

    inline bool isSuccess();

    // entrypoints to compiled code

    typedef size_t transfer_callback(char*, size_t, size_t, void*);

    enum CacheMode { no_cache, cache_write, cache_read };

    void transferBegin(void* stream, char const* cacheFile);
    void transferEnd();

    void perform(char const* url, transfer_callback* transfer);

    void getInfo(char const*& url,
            char const*& type, int64_t& length, int64_t& stardate);

    // synthesized callback

    template< class Stream > static size_t callback_template(char *input, size_t size, 
                                                             size_t nmemb, void* thiz);

    template< class Stream > size_t feedBuffered(Stream* stream, 
                                                 char* input, size_t size);

    // state

    void*       _curlHandle;
    char*       _xtraBuffer;
    char const* _errorStr;
    void*       _streamPtr;
    char const* _cacheFileName;
    FILE*       _cacheFile;
    char*       _cacheReadBuffer;
    CacheMode   _cacheMode; 
    size_t      _xtraSize;
};

// inline functions

inline char const* UrlReader::getError() const {

    return _errorStr;
}

bool UrlReader::isSuccess() {

    return _errorStr == success || _errorStr == success_cached;
}

template< class ContentStream >
bool UrlReader::readUrl(char const* url, ContentStream& s, char const* cacheFile) {
    if (! _curlHandle) return false;

    this->transferBegin(& s, cacheFile);
    this->perform(url, & callback_template<ContentStream>);
    this->transferEnd();
    bool ok = isSuccess();
    s.end(ok);
    return ok;
}

inline void UrlReader::setError(char const* staticCstring) {

    if (this->isSuccess())
        this->_errorStr = staticCstring;
}

template< class Stream >
size_t UrlReader::feedBuffered(Stream* stream, char* input, size_t size) {

    size_t inputOffset = 0u;

    while (true) {

        char* buffer = input + inputOffset;
        size_t bytes = size - inputOffset;

        // data in extra buffer?
        if (_xtraSize > 0) {

            // fill extra buffer with beginning of input
            size_t fill = max_read_ahead - _xtraSize;
            if (bytes < fill) fill = bytes;
            memcpy(_xtraBuffer + _xtraSize, buffer, fill);
            // use extra buffer for next transfer
            buffer = _xtraBuffer;
            bytes = _xtraSize + fill;
            inputOffset += fill;
        }

        // call 'transfer'
        size_t processed = stream->transfer(buffer, bytes);
        if (processed == abort) {

            setError(error_aborted);
            return 0u;

        } else if (! processed && ! input) {

            setError(error_leftover_input);
            return 0u;
        }
        size_t unprocessed = bytes - processed;

        // can switch to input buffer, now?
        if (buffer == _xtraBuffer && unprocessed <= inputOffset) {

            _xtraSize = 0u;
            inputOffset -= unprocessed;

        } else { // no? unprocessed data -> extra buffer

            if (unprocessed > max_read_ahead) {

                setError(error_buffer_overflow);
                return 0;
            }
            _xtraSize = unprocessed;
            memmove(_xtraBuffer, buffer + processed, unprocessed);

            if (inputOffset == size || buffer != _xtraBuffer) {

                return size;
            }
        }
    } // while
}

template< class Stream >
size_t UrlReader::callback_template(char *input, size_t size, size_t nmemb, void* thiz) {

    size_t result = 0u;
    UrlReader* me = static_cast<UrlReader*>(thiz);
    Stream* stream = static_cast<Stream*>(me->_streamPtr);
    size *= nmemb;

    // first call?
    if (me->_xtraSize == ~size_t(0)) {

        me->_xtraSize = 0u;
        // extract meta information and call 'begin'
        char const* url, * type;
        int64_t length, stardate;
        me->getInfo(url, type, length, stardate);
        if (me->_cacheMode != cache_read) { 
            stream->begin(url, type, length, stardate);
        }
    }
    do {
        // will have to repeat from here when reading a local file

        // read from cache file?
        if (me->_cacheMode == cache_read) {
            // change input buffer and start
            input = me->_cacheReadBuffer;
            size = fread(input, 1, max_read_ahead, me->_cacheFile);
            nmemb = 1;
        } else if (me->_cacheMode == cache_write) {
            fwrite(input, 1, size, me->_cacheFile);
        }

        result = me->feedBuffered(stream, input, size);

    } while (me->_cacheMode == cache_read && result != 0 && ! feof(me->_cacheFile));

    return me->_cacheMode != cache_read ? result : 0;
}

#endif /* defined(__hifi__UrlReader__) */

