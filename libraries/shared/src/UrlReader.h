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

        enum CacheMode { no_cache, cache_write, cache_read };

        void*       _ptrImpl;
        char*       _arrXtra;
        char const* _strError;
        void*       _ptrStream;
        char const* _strCacheFile;
        FILE*       _ptrCacheFile;
        char*       _arrCacheRdBuf;
        CacheMode   _valCacheMode; 
        size_t      _valXtraSize;
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

        // entrypoints to compiled code

        typedef size_t transfer_callback(char*, size_t, size_t, void*);

        bool perform(char const* url, transfer_callback* transfer);

        void getinfo(char const*& url,
                char const*& type, int64_t& length, int64_t& stardate);

        // synthesized callback

        template< class Stream > static size_t callback_template(char *input, size_t size, 
                                                                 size_t nmemb, void* thiz);

        template< class Stream > size_t feedBuffered(Stream* stream, 
                                                     char* input, size_t size);
};

template< class ContentStream >
bool UrlReader::readUrl(char const* url, ContentStream& s, char const* cacheFile) {
    if (! _ptrImpl) return false;
    _strCacheFile = cacheFile;
    _ptrCacheFile = 0l;
    _valCacheMode = no_cache; // eventually set later
    _strError = success;
    _ptrStream = & s;
    _valXtraSize = ~size_t(0);
    this->perform(url, & callback_template<ContentStream>);
    s.end(_strError == success);
    if (_ptrCacheFile != 0l) {
        fclose(_ptrCacheFile);
    }
    return _strError == success || _strError == success_cached;
}

inline char const* UrlReader::getError() const { return this->_strError; }

inline void UrlReader::setError(char const* staticCstring) {

    if (this->_strError == success || this->_strError == success_cached)
        this->_strError = staticCstring;
}

template< class Stream >
size_t UrlReader::feedBuffered(Stream* stream, char* input, size_t size) {
    size_t inputOffset = 0u;

    while (true) {

        char* buffer = input + inputOffset;
        size_t bytes = size - inputOffset;

        // data in extra buffer?
        if (_valXtraSize > 0) {

            // fill extra buffer with beginning of input
            size_t fill = max_read_ahead - _valXtraSize;
            if (bytes < fill) fill = bytes;
            memcpy(_arrXtra + _valXtraSize, buffer, fill);
            // use extra buffer for next transfer
            buffer = _arrXtra;
            bytes = _valXtraSize + fill;
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
        if (buffer == _arrXtra && unprocessed <= inputOffset) {

            _valXtraSize = 0u;
            inputOffset -= unprocessed;

        } else { // no? unprocessed data -> extra buffer

            if (unprocessed > max_read_ahead) {

                setError(error_buffer_overflow);
                return 0;
            }
            _valXtraSize = unprocessed;
            memmove(_arrXtra, buffer + processed, unprocessed);

            if (inputOffset == size || buffer != _arrXtra) {

                return size;
            }
        }
    } // while
}

template< class Stream >
size_t UrlReader::callback_template(char *input, size_t size, size_t nmemb, void* thiz) {

    size_t result = 0u;
    UrlReader* me = static_cast<UrlReader*>(thiz);
    Stream* stream = static_cast<Stream*>(me->_ptrStream);
    size *= nmemb;

    // first call?
    if (me->_valXtraSize == ~size_t(0)) {

        me->_valXtraSize = 0u;
        // extract meta information and call 'begin'
        char const* url, * type;
        int64_t length, stardate;
        me->getinfo(url, type, length, stardate);
        if (me->_valCacheMode != cache_read) { 
            stream->begin(url, type, length, stardate);
        }
    }
    do {
        // will have to repeat from here when reading a local file

        // read from cache file?
        if (me->_valCacheMode == cache_read) {
            // change input buffer and start
            input = me->_arrCacheRdBuf;
            size = fread(input, 1, max_read_ahead, me->_ptrCacheFile);
            nmemb = 1;
        } else if (me->_valCacheMode == cache_write) {
            fwrite(input, 1, size, me->_ptrCacheFile);
        }

        result = me->feedBuffered(stream, input, size);

    } while (me->_valCacheMode == cache_read && result != 0 && ! feof(me->_ptrCacheFile));

    return me->_valCacheMode != cache_read ? result : 0;
}

#endif /* defined(__hifi__UrlReader__) */

