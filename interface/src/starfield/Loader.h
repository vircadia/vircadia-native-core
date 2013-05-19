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

#include <locale.h>

#include "Config.h"

#include "starfield/data/InputVertex.h"
#include "starfield/data/BrightnessLevel.h"

namespace starfield {

    class Loader : UrlReader {

        InputVertices*  _vertices;
        unsigned        _limit;

        unsigned        _lineNo;
        char const*     _urlStr;

        unsigned        _recordsRead;
        BrightnessLevel _minBrightness;
    public:

        bool loadVertices(
                InputVertices& destination, char const* url, char const* cacheFile, unsigned limit)
        {
            _vertices = & destination;
            _limit = limit;
#if STARFIELD_SAVE_MEMORY
            if (_limit == 0 || _limit > 60000u)
                _limit = 60000u;
#endif
            _urlStr = url; // in case we fail early

            if (! UrlReader::readUrl(url, *this, cacheFile))
            {
                printLog("%s:%d: %s\n",
                        _urlStr, _lineNo, getError());

                return false;
            }
            printLog("Loaded %u stars.\n", _recordsRead);

            return true;
        }

    protected:

        friend class UrlReader;

        void begin(char const* url,
                   char const* type,
                   int64_t size,
                   int64_t stardate) {

            _lineNo = 0u;
            _urlStr = url; // new value in http redirect

            _recordsRead = 0u;

            _vertices->clear();
            _vertices->reserve(_limit);
// printLog("Stars.cpp: loader begin %s\n", url);
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
                ++_lineNo;
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
                setlocale(LC_NUMERIC, "C");
                if (sscanf(line, " %f %f #%x", & azi, & alt, & c) == 3) {

                    if (spaceFor( getBrightness(c) )) {

                        storeVertex(azi, alt, c);
                    }

                    ++_recordsRead;

                } else {

                    printLog("Stars.cpp:%d: Bad input from %s\n", 
                            _lineNo, _urlStr);
                }

            }
            return consumed;
        }

        void end(bool ok)
        { }

    private:

        bool atLimit() { return _limit > 0u && _recordsRead >= _limit; }

        bool spaceFor(BrightnessLevel b) {

            if (! atLimit()) {
                return true;
            }

            // just reached the limit? -> establish a minimum heap and 
            // remember the brightness at its top
            if (_recordsRead == _limit) {

// printLog("Stars.cpp: vertex limit reached -> heap mode\n");

                make_heap(
                    _vertices->begin(), _vertices->end(),
                    GreaterBrightness() );

                _minBrightness = getBrightness(
                        _vertices->begin()->getColor() );
            }

            // not interested? say so
            if (_minBrightness >= b)
                return false;

            // otherwise free up space for the new vertex
            pop_heap(
                _vertices->begin(), _vertices->end(),
                GreaterBrightness() );
            _vertices->pop_back();
            return true;
        }

        void storeVertex(float azi, float alt, unsigned color) {
  
            _vertices->push_back(InputVertex(azi, alt, color));

            if (atLimit()) {

                push_heap(
                    _vertices->begin(), _vertices->end(),
                    GreaterBrightness() );

                _minBrightness = getBrightness(
                        _vertices->begin()->getColor() );
            }
        }
    };

} // anonymous namespace

#endif

