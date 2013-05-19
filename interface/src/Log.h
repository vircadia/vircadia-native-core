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
// Macro to log OpenGl errors.
// Example: oglLog( glPushMatrix() );
//
#define oGlLog(stmt) \
    stmt; \
    { \
        GLenum e = glGetError(); \
        if (e != GL_NO_ERROR) { \
            printLog(__FILE__ ":"  oGlLog_stringize(__LINE__) \
                     " [OpenGL] %s\n", gluErrorString(e)); \
        } \
    } \
    (void) 0

#define oGlLog_stringize(x) oGlLog_stringize_i(x)
#define oGlLog_stringize_i(x) # x

//
// Global instance.
//
extern Log logger;

//
// Logging subsystem.
//
class Log {
    FILE*       _stream;
    char*       _chars;
    char*       _charsEnd;
    char**      _lines;
    char**      _linesEnd;

    char*       _writePos;       // character position to write to
    char*       _writeLineStartPos; // character position where line being written starts
    char**      _lastLinePos;       // last line in the log
    unsigned    _writtenInLine;  // character counter for line wrapping
    unsigned    _lineLength;     // number of characters before line wrap

    unsigned    _logWidth;       // width of the log in pixels
    unsigned    _charWidth;      // width of a character in pixels
    unsigned    _charHeight;     // height of a character in pixels
    unsigned    _charYoffset;    // baseline offset in pixels
    float       _charScale;      // scale factor 
    float       _charAspect;     // aspect (h/w)

    pthread_mutex_t _mutex;

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

