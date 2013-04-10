//
// starfield/Loader.h
// interface
//
// Created by Tobias Schwinger on 3/29/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__starfield__Loader__
#define __interface__starfield__Loader__

#ifndef __interface__Starfield_impl__
#error "This is an implementation file - not intended for direct inclusion."
#endif

#include "Config.h"

#include "starfield/data/InputVertex.h"
#include "starfield/data/BrightnessLevel.h"

namespace starfield {

    class Loader : UrlReader {

        InputVertices*  _ptrVertices;
        unsigned        _valLimit;

        unsigned        _valLineNo;
        char const*     _strUrl;

        unsigned        _valRecordsRead;
        BrightnessLevel _valMinBrightness;
    public:

        bool loadVertices(
                InputVertices& destination, char const* url, unsigned limit)
        {
            _ptrVertices = & destination;
            _valLimit = limit;
#if STARFIELD_SAVE_MEMORY
            if (_valLimit == 0 || _valLimit > 60000u)
                _valLimit = 60000u;
#endif
            _strUrl = url; // in case we fail early

            if (! UrlReader::readUrl(url, *this))
            {
                fprintf(stderr, "%s:%d: %s\n",
                        _strUrl, _valLineNo, getError());

                return false;
            }
 fprintf(stderr, "Stars.cpp: read %d stars, rendering %ld\n", 
         _valRecordsRead, _ptrVertices->size());

            return true;
        }

    protected:

        friend class UrlReader;

        void begin(char const* url,
                   char const* type,
                   int64_t size,
                   int64_t stardate) {

            _valLineNo = 0u;
            _strUrl = url; // new value in http redirect

            _valRecordsRead = 0u;

            _ptrVertices->clear();
            _ptrVertices->reserve(_valLimit);
// fprintf(stderr, "Stars.cpp: loader begin %s\n", url);
        }
        
        size_t transfer(char* input, size_t bytes) {

            size_t consumed = 0u;
            char const* end = input + bytes;
            char* line, * next = input;

            for (;;) {

                // advance to next line
                for (; next != end && isspace(*next); ++next);
                consumed = next - input;
                line = next;
                ++_valLineNo;
                for (; next != end && *next != '\n' && *next != '\r'; ++next);
                if (next == end)
                    return consumed;
                *next++ = '\0';

                // skip comments
                if (*line == '\\' || *line == '/' || *line == ';')
                    continue;

                // parse
                float azi, alt;
                unsigned c;
                if (sscanf(line, " %f %f #%x", & azi, & alt, & c) == 3) {

                    if (spaceFor( getBrightness(c) )) {

                        storeVertex(azi, alt, c);
                    }

                    ++_valRecordsRead;

                } else {

                    fprintf(stderr, "Stars.cpp:%d: Bad input from %s\n", 
                            _valLineNo, _strUrl);
                }

            }
            return consumed;
        }

        void end(bool ok)
        { }

    private:

        bool atLimit() { return _valLimit > 0u && _valRecordsRead >= _valLimit; }

        bool spaceFor(BrightnessLevel b) {

            if (! atLimit()) {
                return true;
            }

            // just reached the limit? -> establish a minimum heap and 
            // remember the brightness at its top
            if (_valRecordsRead == _valLimit) {

// fprintf(stderr, "Stars.cpp: vertex limit reached -> heap mode\n");

                make_heap(
                    _ptrVertices->begin(), _ptrVertices->end(),
                    GreaterBrightness() );

                _valMinBrightness = getBrightness(
                        _ptrVertices->begin()->getColor() );
            }

            // not interested? say so
            if (_valMinBrightness >= b)
                return false;

            // otherwise free up space for the new vertex
            pop_heap(
                _ptrVertices->begin(), _ptrVertices->end(),
                GreaterBrightness() );
            _ptrVertices->pop_back();
            return true;
        }

        void storeVertex(float azi, float alt, unsigned color) {
  
            _ptrVertices->push_back(InputVertex(azi, alt, color));

            if (atLimit()) {

                push_heap(
                    _ptrVertices->begin(), _ptrVertices->end(),
                    GreaterBrightness() );

                _valMinBrightness = getBrightness(
                        _ptrVertices->begin()->getColor() );
            }
        }
    };

} // anonymous namespace

#endif

