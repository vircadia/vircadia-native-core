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

/**
 * UrlReader class that encapsulates a context for sequential data retrieval
 * via URLs. Use one per thread.
 */
class UrlReader
{
        void*       ptr_impl;
        char*       ptr_ra;
        char const* str_error;
        void*       ptr_stream;
        size_t      val_ra_size;

    public:

        /**
         * Constructor - performs initialization, never throws.
         */
        UrlReader();

        /**
         * Destructor - frees resources, never throws.
         */
        ~UrlReader();

        /**
         * Reads data from an URL and forwards it to the instance of a class
         * fulfilling the ContentStream concept.
         * 
         * The call protocol on the ContentStream is detailed as follows:
         * 
         * 1. begin(char const* url, 
         *      char const* content_type, uint64_t bytes, uint64_t stardate)
         *
         *    All information except 'url' is optional; 'content_type' can
         *    be a null pointer - 'bytes' and 'stardate' can be equal to
         *    to 'unavailable'.
         *
         * 2. transfer(char* buffer, size_t bytes)
         *    
         *    Called until all data has been received. The number of bytes
         *    actually processed should be returned.
         *    Unprocessed data is stored in an extra buffer whose size is
         *    given by the constant UrlReader::max_read_ahead - it can be
         *    assumed to be reasonably large for on-the-fly parsing.
         *
         * 3. end(bool ok)
         *
         *    Called at the end of the transfer.
         *
         * Returns the same success code 
         */ 
        template< class ContentStream >
        bool readUrl(char const* url, ContentStream& s);

        /**
         * Returns a pointer to a static C-string that describes the error
         * condition.
         */
        inline char const* getError() const;

        /**
         * Can be called by the stream to set a user-defined error string.
         */
        inline void setError(char const* static_c_string);

        /**
         * Pointer to the C-string returned by a call to 'readUrl' when no
         * error occurred.
         */
        static char const* const success;

        /**
         * Pointer to the C-string returned by a call to 'readUrl' when the
         * initialization has failed.
         */
        static char const* const error_init_failed;

        /**
         * Pointer to the C-string returned by a call to 'readUrl' when the
         * transfer has been aborted by the client.
         */
        static char const* const error_aborted;

        /**
         * Pointer to the C-string returned by a call to 'readUrl' when 
         * leftover input from incomplete processing caused a buffer
         * overflow.
         */
        static char const* const error_buffer_overflow;

        /**
         * Pointer to the C-string return by a call to 'readUrl' when the
         * input provided was not completely consumed.
         */
        static char const* const error_leftover_input;

        /**
         * Constant of the maximum number of bytes that are buffered
         * between invocations of 'transfer'.
         */
        static size_t const max_read_ahead;

        /**
         * Constant representing absent information in the call to the
         * 'begin' member function of the target stream.
         */
        static int const unavailable = -1;

        /**
         * Constant for requesting to abort the current transfer when
         * returned by the 'transfer' member function of the target stream.
         */
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

        template< class Stream >
        static size_t callback_template(
                char *input, size_t size, size_t nmemb, void* thiz);
};

template< class ContentStream >
bool UrlReader::readUrl(char const* url, ContentStream& s)
{
    if (! ptr_impl) return false;
    str_error = success;
    ptr_stream = & s;
    val_ra_size = ~size_t(0);
    this->perform(url, & callback_template<ContentStream>);
    s.end(str_error == success);
    return str_error == success;
}

inline char const* UrlReader::getError() const { return this->str_error; }

inline void UrlReader::setError(char const* static_c_string)
{
    if (this->str_error == success)
        this->str_error = static_c_string;
}

template< class Stream >
size_t UrlReader::callback_template(
        char *input, size_t size, size_t nmemb, void* thiz)
{
    size *= nmemb;

    UrlReader* me = static_cast<UrlReader*>(thiz);
    Stream* stream = static_cast<Stream*>(me->ptr_stream);

    // first call?
    if (me->val_ra_size == ~size_t(0))
    {
        me->val_ra_size = 0u;
        // extract meta information and call 'begin'
        char const* url, * type;
        int64_t length, stardate;
        me->getinfo(url, type, length, stardate);
        stream->begin(url, type, length, stardate);
    }

    size_t input_offset = 0u;

    for (;;)
    {
        char* buffer = input + input_offset;
        size_t bytes = size - input_offset;

        // data in extra buffer?
        if (me->val_ra_size > 0)
        {
            // fill extra buffer with beginning of input
            size_t fill = max_read_ahead - me->val_ra_size;
            if (bytes < fill) fill = bytes;
            memcpy(me->ptr_ra + me->val_ra_size, buffer, fill);
            // use extra buffer for next transfer
            buffer = me->ptr_ra;
            bytes = me->val_ra_size + fill;
            input_offset += fill;
        }

        // call 'transfer'
        size_t processed = stream->transfer(buffer, bytes);
        if (processed == abort)
        {
            me->setError(error_aborted);
            return 0u;
        }
        else if (! processed && ! input)
        {
            me->setError(error_leftover_input);
            return 0u;
        }
        size_t unprocessed = bytes - processed;

        // can switch to input buffer, now?
        if (buffer == me->ptr_ra && unprocessed <= input_offset)
        {
            me->val_ra_size = 0u;
            input_offset -= unprocessed;
        }
        else // no? unprocessed data -> extra buffer
        {
            if (unprocessed > max_read_ahead)
            {
                me->setError(error_buffer_overflow);
                return 0;
            }
            me->val_ra_size = unprocessed;
            memmove(me->ptr_ra, buffer + processed, unprocessed);

            if (input_offset == size || buffer != me->ptr_ra)
            {
                return size;
            }
        }
    } // for
}

#endif /* defined(__hifi__UrlReader__) */

