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

namespace {
    // anonymous namespace - everything in here only exists within this very .cpp file
    // just as 'static' on every effective line in plain C

    unsigned const CHARACTER_BUFFER_SIZE = 16384;   // number of character that are buffered
    unsigned const LINE_BUFFER_SIZE = 256;          // number of lines that are buffered
    unsigned const MAX_MESSAGE_LENGTH = 512;        // maximum number of characters for a message

    bool const TEXT_MONOSPACED = true;

    float const TEXT_RED = 0.7f;
    float const TEXT_GREEN = 0.6f;
    float const TEXT_BLUE = 1.0f;

    // magic constants from the GLUT spec
    // http://www.opengl.org/resources/libraries/glut/spec3/node78.html
    // ultimately this stuff should be in Util.h??
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

    // T-pipe, if requested
    if (_ptrStream != 0l) {
        fprintf(_ptrStream, "%s", ptr);
    }

    while (*ptr != '\0') {
        // process the characters
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
                _itrLastLine[1] = 0l;
            } else if (_itrLastLine + 1 != _ptrLinesEnd) {
                _itrLastLine[1] = 0l;
            } else {
                _arrLines[0] = 0l;
            } 
            *_itrLastLine = _itrWriteLineStart;

            // debug mode: make sure all line pointers we write here are valid
            assert(! (_itrLastLine < _arrLines || _itrLastLine >= _ptrLinesEnd));
            assert(! (*_itrLastLine < _arrChars || *_itrLastLine >= _ptrCharsEnd));

            // terminate line, unless done already
            if (c != '\0') {
                *_itrWritePos++ = '\0';

                if (_itrWritePos == _ptrCharsEnd) {
                    _itrWritePos = _arrChars;
                }
            }

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
    int n = vsnprintf(buf, MAX_MESSAGE_LENGTH, fmt, args); 
    if (n > 0) {

        // all fine? log the message
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

        // debug mode: make sure all line pointers we find here are valid
        assert(! (firstLine < _arrLines || firstLine >= _ptrLinesEnd));
        assert(! (*firstLine < _arrChars || *firstLine >= _ptrCharsEnd));
    }

    // copy the line buffer portion into a contiguous region at _ptrLinesEnd
    if (firstLine <= lastLine) {

        memcpy(_ptrLinesEnd, firstLine, showLines * sizeof(char*));

    } else {

        unsigned atEnd = _ptrLinesEnd - firstLine;
        memcpy(_ptrLinesEnd, firstLine, atEnd * sizeof(char*));
        memcpy(_ptrLinesEnd + atEnd, _arrLines, (showLines - atEnd) * sizeof(char*));
    }

    // copy relevant char buffer portion and determine information to remap the pointers
    char* firstChar = *firstLine;
    char* lastChar = *lastLine + strlen(*lastLine) + 1;
    ptrdiff_t charOffset = _ptrCharsEnd - firstChar, charOffsetBeforeFirst = 0;
    if (firstChar <= lastChar) {

        memcpy(_ptrCharsEnd, firstChar, lastChar - firstChar + 1);

    } else {

        unsigned atEnd = _ptrCharsEnd - firstChar;
        memcpy(_ptrCharsEnd, firstChar, atEnd);
        memcpy(_ptrCharsEnd + atEnd, _arrChars, lastChar + 1 - _arrChars);

        charOffsetBeforeFirst = _ptrCharsEnd + atEnd - _arrChars;
    }

    // get values for rendering
    float scaleFactor = _valCharScale;
    int yStart = int((screenHeight - _valCharYoffset) / _valCharAspect);
    int yStep = int(_valCharHeight / _valCharAspect);
    float yScale = _valCharAspect;

    // render text
    char** line = _ptrLinesEnd + showLines;
    int x = screenWidth - _valLogWidth;

    pthread_mutex_unlock(& _mtx);
    // ok, we got all we need

    GLint matrixMode; 
    glGetIntegerv(GL_MATRIX_MODE, & matrixMode);   
    glPushMatrix();
    glScalef(1.0f, yScale, 1.0f);

    for (int y = yStart; y > 0; y -= yStep) {

        // debug mode: check line pointer is valid
        assert(! (line < _ptrLinesEnd || line >= _ptrLinesEnd + (_ptrLinesEnd - _arrLines)));

        // get character pointer
        if (--line < _ptrLinesEnd) {
            break;
        }
        char* chars = *line;

        // debug mode: check char pointer we find is valid
        assert(! (chars < _arrChars || chars >= _ptrCharsEnd));

        // remap character pointer it to copied buffer
        chars += chars >= firstChar ? charOffset : charOffsetBeforeFirst;

        // debug mode: check char pointer is still valid (in new range)
        assert(! (chars < _ptrCharsEnd || chars >= _ptrCharsEnd + (_ptrCharsEnd - _arrChars)));

        // render the string
        drawtext(x, y, scaleFactor, 0.0f, 1.0f, int(TEXT_MONOSPACED), 
                 chars, TEXT_RED, TEXT_GREEN, TEXT_BLUE);

//fprintf(stderr, "Log::render, message = \"%s\"\n", chars);
    }

    glPopMatrix();
    glMatrixMode(matrixMode);
}

Log printLog;




