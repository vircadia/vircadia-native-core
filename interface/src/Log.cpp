//
// Log.cpp
// interface
//
// Created by Tobias Schwinger on 4/14/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "Log.h"

#include "InterfaceConfig.h"

#include <string.h>
#include <stdarg.h>

#include "Util.h"
#include "ScopedLock.h"

namespace {
    // anonymous namespace - everything in here only exists within this very .cpp file

    unsigned const CHARACTER_BUFFER_SIZE = 16384; 
    unsigned const LINE_BUFFER_SIZE = 256; 
    unsigned const MAX_MESSAGE_LENGTH = 512;

    bool const TEXT_MONOSPACED = false;

    float const TEXT_RED = 0.7f;
    float const TEXT_GREEN = 0.6f;
    float const TEXT_BLUE = 1.0f;

    // magic constants from the GLUT spec
    // http://www.opengl.org/resources/libraries/glut/spec3/node78.html
    // ultimately this stuff should be handled in Util??
    float const CHAR_UP = 119.05f;
    float const CHAR_DOWN = 33.33f;
    float const CHAR_WIDTH = 104.76f;
    // derived values
    float const CHAR_HEIGHT = CHAR_UP + CHAR_DOWN;
    float const CHAR_FRACT_BASELINE = CHAR_DOWN / CHAR_HEIGHT;

    // unsigned integer division rounded towards infinity
    unsigned divRoundUp(unsigned l, unsigned r) { return (l + r - 1) / r; }
}

Log::Log(FILE* tPipeTo, unsigned bufferedLines,
         unsigned defaultLogWidth, unsigned defaultCharWidth, unsigned defaultCharHeight) :

    _ptrStream(tPipeTo),
    _arrChars(0l),
    _arrLines(0l),
    _valLogWidth(defaultLogWidth) {

    pthread_mutex_init(& _mtx, 0l);

    // allocate twice as much (so we have spare space for a copy not to block 
    // logging from other threads during 'render')
    _arrChars = new char[CHARACTER_BUFFER_SIZE * 2];
    _ptrCharsEnd = _arrChars + CHARACTER_BUFFER_SIZE;
    _arrLines = new char*[LINE_BUFFER_SIZE * 2];
    _ptrLinesEnd = _arrLines + LINE_BUFFER_SIZE;

    // initialize the log to all empty lines
    _arrChars[0] = '\0';
    _itrWritePos = _arrChars;
    _itrWriteLineStart = _arrChars;
    _itrLastLine = _arrLines;
    _valWrittenInLine = 0;
    memset(_arrLines, 0, LINE_BUFFER_SIZE * sizeof(char*));

    setCharacterSize(defaultCharWidth, defaultCharHeight);
}


Log::~Log() {

    delete[] _arrChars;
    delete[] _arrLines;
}

inline void Log::addMessage(char const* ptr) {

    // precondition: mutex is locked so noone gets in our way

    while (*ptr != '\0') {
        char c = *ptr++;

        if (c == '\t') { 

            // found TAB -> write SPACE
            c = ' '; 

        } else if (c == '\n') {

            // found LF -> write NUL (c == '\0' tells us to wrap, below)
            c = '\0';
        }
        *_itrWritePos++ = c;

        if (_itrWritePos == _ptrCharsEnd) {

            // reached the end of the circular character buffer? -> start over
            _itrWritePos = _arrChars;
        }

        if (++_valWrittenInLine >= _valLineLength || c == '\0') {

            // new line? store its start to the line buffer and mark next line as empty
            ++_itrLastLine;
            if (_itrLastLine == _ptrLinesEnd) {
                _itrLastLine = _arrLines;
                _arrLines[1] = 0l;
            } else if (_itrLastLine + 1 == _ptrLinesEnd) {
                _arrLines[0] = 0l;
            } else {
                _itrLastLine[1] = 0l;
            } 
            *_itrLastLine = _itrWriteLineStart;

            // remember start position in character buffer for next line and reset character count
            _itrWriteLineStart = _itrWritePos;
            _valWrittenInLine = 0;
        }
    }

}

void Log::operator()(char const* fmt, ...) {

    va_list args;
    va_start(args,fmt);
    pthread_mutex_lock(& _mtx);

    // print to buffer
    char buf[MAX_MESSAGE_LENGTH];
    if (vsnprintf(buf, MAX_MESSAGE_LENGTH, fmt, args) > 0) {

        // all fine? eventually T-pipe to posix file and add message to log
        if (_ptrStream != 0l) {
            fprintf(_ptrStream, "%s", buf);
        }
        addMessage(buf);

    } else {

        // error? -> mutter on stream or stderr
        fprintf(_ptrStream != 0l ? _ptrStream : stderr,
                "Log: Failed to log message with format string = \"%s\".\n", fmt);
    }

    pthread_mutex_unlock(& _mtx);
    va_end(args);
}

void Log::setLogWidth(unsigned pixels) {

    pthread_mutex_lock(& _mtx);
    _valLogWidth = pixels;
    _valLineLength = _valLogWidth / _valCharWidth;
    pthread_mutex_unlock(& _mtx);
}

void Log::setCharacterSize(unsigned width, unsigned height) {

    pthread_mutex_lock(& _mtx);
    _valCharWidth = width;
    _valCharHeight = height;
    _valCharYoffset = height * CHAR_FRACT_BASELINE;
    _valCharScale = float(width) / CHAR_WIDTH;
    _valCharAspect = (height * CHAR_WIDTH) / (width * CHAR_HEIGHT);
    _valLineLength = _valLogWidth / _valCharWidth;
    pthread_mutex_unlock(& _mtx);
}

void Log::render(unsigned screenWidth, unsigned screenHeight) {

    // rendering might take some time, so create a local copy of the portion we need
    // instead of having to hold the mutex all the time
    pthread_mutex_lock(& _mtx);

    // determine number of visible lines
    unsigned showLines = divRoundUp(screenHeight, _valCharHeight);

    char** lastLine = _itrLastLine;
    char** firstLine = _itrLastLine;

    if (! *lastLine) {
        // empty log
        pthread_mutex_unlock(& _mtx);
        return;
    }

    // scan for first line
    for (int n = 2; n <= showLines; ++n) {

        char** prevFirstLine = firstLine;
        --firstLine;
        if (firstLine < _arrLines) {
            firstLine = _ptrLinesEnd - 1;
        }
        if (! *firstLine) {
            firstLine = prevFirstLine;
            showLines = n - 1;
            break;
        }
    }

    // copy the line buffer portion into a contiguous region at _ptrLinesEnd
    if (firstLine <= lastLine) {

        memcpy(_ptrLinesEnd, firstLine, showLines * sizeof(char*));

    } else {

        unsigned atEnd = _ptrLinesEnd - firstLine;
        memcpy(_ptrLinesEnd, firstLine, atEnd * sizeof(char*));
        memcpy(_ptrLinesEnd + atEnd, _arrLines, (showLines - atEnd) * sizeof(char*));
    }

    // copy relevant char buffer portion and determine information to remap the line pointers
    char* firstChar = *firstLine;
    char* lastChar = *lastLine + strlen(*lastLine) + 1;
    ptrdiff_t charOffset = _ptrCharsEnd - firstChar, charOffsetBeforeFirst = 0;
    if (firstChar <= lastChar) {

        memcpy(_ptrCharsEnd, firstChar, lastChar - firstChar);

    } else {

        unsigned atEnd = _ptrCharsEnd - firstChar;
        memcpy(_ptrCharsEnd, firstChar, atEnd);
        memcpy(_ptrCharsEnd + atEnd, _arrChars, lastChar - _arrChars);

        charOffsetBeforeFirst = _ptrCharsEnd + atEnd - _arrChars;
    }

    // get values for rendering
    unsigned logWidth = _valLogWidth;
    if (logWidth > screenWidth) {
        logWidth = screenWidth;
    }
    int yStart = int((screenHeight - _valCharYoffset) / _valCharAspect);
    int yStep = int(_valCharHeight / _valCharAspect);
    float yScale = _valCharAspect;

    pthread_mutex_unlock(& _mtx);
    // ok, we got all we need

    // render text
    char** line = _ptrLinesEnd + showLines;
    int x = screenWidth - _valLogWidth;
   
    glPushMatrix();
    glScalef(1.0f, yScale, 1.0f);

    for (int y = yStart; y > 0; y -= yStep) {

        // get character pointer
        char* chars = *--line;
        if (! chars) {
            break;
        }

        // remap character pointer it to copied buffer
        chars += chars >= firstChar ? charOffset : charOffsetBeforeFirst;

        // render the string
        drawtext(x, y, _valCharScale, 0.0f, 1.0f, int(TEXT_MONOSPACED), 
                 chars, TEXT_RED, TEXT_GREEN, TEXT_BLUE);

//fprintf(stderr, "Log::render, message = \"%s\"\n", chars);
    }

    glPopMatrix();
}



