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
#include <stdarg.h>
#include <glm/glm.hpp>
#include <pthread.h>

class Log;

//
// Call it as you would call 'printf'.
//
int printLog(char const* fmt, ...);

//
// Global instance.
//
extern Log logger;


//
// Logging subsystem.
//
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
                 unsigned defaultLogWidth = 400, unsigned defaultCharWidth = 6, unsigned defaultCharHeight = 20);
    ~Log();

    void setLogWidth(unsigned pixels);
    void setCharacterSize(unsigned width, unsigned height);

    void render(unsigned screenWidth, unsigned screenHeight);

    void operator()(char const* fmt, ...);
    int  vprint(char const* fmt, va_list);

private:
    // don't copy/assign
    Log(Log const&); // = delete;
    Log& operator=(Log const&); // = delete;

    inline void addMessage(char const*);

    friend class LogStream; // for optional iostream-style interface that has to be #included separately
};

#endif

