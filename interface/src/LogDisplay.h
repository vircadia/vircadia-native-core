//
// LogDisplay.h
// interface
//
// Created by Tobias Schwinger on 4/14/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__LogDisplay__
#define __interface__LogDisplay__

#include <stdarg.h>
#include <pthread.h>

#include "Log.h"
#include "ui/TextRenderer.h"

class LogDisplay {
public:

    static LogDisplay instance;

    void render(unsigned screenWidth, unsigned screenHeight);

    // settings 

    static unsigned const TEXT_COLOR            = 0xb299ff; // text foreground color (bytes, RGB)

    static FILE*    const DEFAULT_STREAM; //    = stdout;   // stream to also log to (defined in .cpp)
    static unsigned const DEFAULT_CHAR_WIDTH    = 5;        // width of a single character
    static unsigned const DEFAULT_CHAR_HEIGHT   = 16;       // height of a single character
    static unsigned const DEFAULT_CONSOLE_WIDTH = 400;      // width of the (right-aligned) log console

    void setStream(FILE* stream);
    void setLogWidth(unsigned pixels);
    void setCharacterSize(unsigned width, unsigned height);

    // limits

    static unsigned const CHARACTER_BUFFER_SIZE    = 16384; // number of character that are buffered
    static unsigned const LINE_BUFFER_SIZE         = 256;   // number of lines that are buffered
    static unsigned const MAX_MESSAGE_LENGTH       = 512;   // maximum number of characters for a message

private:
    // use static 'instance' to access the single instance
    LogDisplay();
    ~LogDisplay();

    // don't copy/assign
    LogDisplay(LogDisplay const&); // = delete;
    LogDisplay& operator=(LogDisplay const&); // = delete;

    // format and log message - entrypoint used to replace global 'printLog'
    static int printLogHandler(char const* fmt, ...);

    // log formatted message (called by printLogHandler)
    inline void addMessage(char const*);

    TextRenderer    _textRenderer;
    FILE*           _stream;            // FILE as secondary destination for log messages
    char*           _chars;             // character buffer base address
    char*           _charsEnd;          // character buffer, exclusive end
    char**          _lines;             // line buffer base address
    char**          _linesEnd;          // line buffer, exclusive end

    char*           _writePos;          // character position to write to
    char*           _writeLineStartPos; // character position where line being written starts
    char**          _lastLinePos;       // last line in the log
    unsigned        _writtenInLine;     // character counter for line wrapping
    unsigned        _lineLength;        // number of characters before line wrap

    unsigned        _logWidth;          // width of the log in pixels
    unsigned        _charWidth;         // width of a character in pixels
    unsigned        _charHeight;        // height of a character in pixels

    pthread_mutex_t _mutex;
};

#endif

