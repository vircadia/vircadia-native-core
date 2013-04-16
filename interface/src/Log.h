//
// Log.h
// interface
//
// Created by Tobias Schwinger on 4/14/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Log__
#define __interface__Log__

#include <stdio.h>
#include <glm/glm.hpp>
#include <pthread.h>

class Log {

    FILE*       _ptrStream;
    char*       _arrChars;
    char*       _ptrCharsEnd;
    char**      _arrLines;
    char**      _ptrLinesEnd;

    char*       _itrWritePos;       // character position to write to
    char*       _itrWriteLineStart; // character position where line being written starts
    char**      _itrLastLine;       // last line in the log
    unsigned    _valWrittenInLine;  // character counter for line wrapping
    unsigned    _valLineLength;     // number of characters before line wrap

    unsigned    _valLogWidth;       // width of the log in pixels
    unsigned    _valCharWidth;      // width of a character in pixels
    unsigned    _valCharHeight;     // height of a character in pixels
    unsigned    _valCharYoffset;    // baseline offset in pixels
    float       _valCharScale;      // scale factor 
    float       _valCharAspect;     // aspect (h/w)

    pthread_mutex_t _mtx;
public:

    explicit Log(FILE* tPipeTo = stdout, unsigned bufferedLines = 1024,
                 unsigned defaultLogWidth = 240, unsigned defaultCharWidth = 10, unsigned defaultCharHeight = 24);
    ~Log();

    void setLogWidth(unsigned pixels);
    void setCharacterSize(unsigned width, unsigned height);

    void operator()(char const* fmt, ...);

    void render(unsigned screenWidth, unsigned screenHeight);

private:

    inline void addMessage(char const*);
    inline void recalcLineLength();
};

#endif

