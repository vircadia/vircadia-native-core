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
#include "ui/TextRenderer.h"

namespace {
    // anonymous namespace - everything in here only exists within this very .cpp file
    // just as 'static' on every effective line in plain C

    unsigned const CHARACTER_BUFFER_SIZE = 16384;   // number of character that are buffered
    unsigned const LINE_BUFFER_SIZE = 256;          // number of lines that are buffered
    unsigned const MAX_MESSAGE_LENGTH = 512;        // maximum number of characters for a message

    const char* FONT_FAMILY = SANS_FONT_FAMILY;
    
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

    _stream(tPipeTo),
    _chars(0l),
    _lines(0l),
    _logWidth(defaultLogWidth) {

    pthread_mutex_init(& _mutex, 0l);

    // allocate twice as much (so we have spare space for a copy not to block 
    // logging from other threads during 'render')
    _chars = new char[CHARACTER_BUFFER_SIZE * 2];
    _charsEnd = _chars + CHARACTER_BUFFER_SIZE;
    _lines = new char*[LINE_BUFFER_SIZE * 2];
    _linesEnd = _lines + LINE_BUFFER_SIZE;

    // initialize the log to all empty lines
    _chars[0] = '\0';
    _writePos = _chars;
    _writeLineStartPos = _chars;
    _lastLinePos = _lines;
    _writtenInLine = 0;
    memset(_lines, 0, LINE_BUFFER_SIZE * sizeof(char*));

    setCharacterSize(defaultCharWidth, defaultCharHeight);
}


Log::~Log() {

    delete[] _chars;
    delete[] _lines;
}

inline void Log::addMessage(char const* ptr) {

    // precondition: mutex is locked so noone gets in our way

    // T-pipe, if requested
    if (_stream != 0l) {
        fprintf(_stream, "%s", ptr);
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
        *_writePos++ = c;

        if (_writePos == _charsEnd) {
            // reached the end of the circular character buffer? -> start over
            _writePos = _chars;
        }

        if (++_writtenInLine >= _lineLength || c == '\0') {

            // new line? store its start to the line buffer and mark next line as empty
            ++_lastLinePos;
            
            if (_lastLinePos == _linesEnd) {
                _lastLinePos = _lines;
                _lastLinePos[1] = 0l;
            } else if (_lastLinePos + 1 != _linesEnd) {
                _lastLinePos[1] = 0l;
            } else {
                _lines[0] = 0l;
            } 
            *_lastLinePos = _writeLineStartPos;

            // debug mode: make sure all line pointers we write here are valid
            assert(! (_lastLinePos < _lines || _lastLinePos >= _linesEnd));
            assert(! (*_lastLinePos < _chars || *_lastLinePos >= _charsEnd));

            // terminate line, unless done already
            if (c != '\0') {
                *_writePos++ = '\0';

                if (_writePos == _charsEnd) {
                    _writePos = _chars;
                }
            }

            // remember start position in character buffer for next line and reset character count
            _writeLineStartPos = _writePos;
            _writtenInLine = 0;
        }
    }

}

int Log::vprint(char const* fmt, va_list args) {
    pthread_mutex_lock(& _mutex);

    // print to buffer
    char buf[MAX_MESSAGE_LENGTH];
    int n = vsnprintf(buf, MAX_MESSAGE_LENGTH, fmt, args); 
    if (n > 0) {

        // all fine? log the message
        addMessage(buf);

    } else {

        // error? -> mutter on stream or stderr
        fprintf(_stream != 0l ? _stream : stderr,
                "Log: Failed to log message with format string = \"%s\".\n", fmt);
    }

    pthread_mutex_unlock(& _mutex);
    return n;
}

void Log::operator()(char const* fmt, ...) {

    va_list args;
    va_start(args,fmt);
    vprint(fmt, args);
    va_end(args);
}

void Log::setLogWidth(unsigned pixels) {

    pthread_mutex_lock(& _mutex);
    _logWidth = pixels;
    _lineLength = _logWidth / _charWidth;
    pthread_mutex_unlock(& _mutex);
}

void Log::setCharacterSize(unsigned width, unsigned height) {

    pthread_mutex_lock(& _mutex);
    _charWidth = width;
    _charHeight = height;
    _charYoffset = height * CHAR_FRACT_BASELINE;
    _charScale = float(width) / CHAR_WIDTH;
    _charAspect = (height * CHAR_WIDTH) / (width * CHAR_HEIGHT);
    _lineLength = _logWidth / _charWidth;
    pthread_mutex_unlock(& _mutex);
}

static TextRenderer* textRenderer() {
    static TextRenderer* renderer = new TextRenderer(FONT_FAMILY, -1, -1, false, TextRenderer::SHADOW_EFFECT);
    return renderer;
}

void Log::render(unsigned screenWidth, unsigned screenHeight) {

    // rendering might take some time, so create a local copy of the portion we need
    // instead of having to hold the mutex all the time
    pthread_mutex_lock(& _mutex);

    // determine number of visible lines
    unsigned showLines = divRoundUp(screenHeight, _charHeight);

    char** lastLine = _lastLinePos;
    char** firstLine = _lastLinePos;

    if (! *lastLine) {
        // empty log
        pthread_mutex_unlock(& _mutex);
        return;
    }

    // scan for first line
    for (int n = 2; n <= showLines; ++n) {

        char** prevFirstLine = firstLine;
        --firstLine;
        if (firstLine < _lines) {
            firstLine = _linesEnd - 1;
        }
        if (! *firstLine) {
            firstLine = prevFirstLine;
            showLines = n - 1;
            break;
        }

        // debug mode: make sure all line pointers we find here are valid
        assert(! (firstLine < _lines || firstLine >= _linesEnd));
        assert(! (*firstLine < _chars || *firstLine >= _charsEnd));
    }

    // copy the line buffer portion into a contiguous region at _linesEnd
    if (firstLine <= lastLine) {

        memcpy(_linesEnd, firstLine, showLines * sizeof(char*));

    } else {

        unsigned atEnd = _linesEnd - firstLine;
        memcpy(_linesEnd, firstLine, atEnd * sizeof(char*));
        memcpy(_linesEnd + atEnd, _lines, (showLines - atEnd) * sizeof(char*));
    }

    // copy relevant char buffer portion and determine information to remap the pointers
    char* firstChar = *firstLine;
    char* lastChar = *lastLine + strlen(*lastLine) + 1;
    ptrdiff_t charOffset = _charsEnd - firstChar, charOffsetBeforeFirst = 0;
    if (firstChar <= lastChar) {

        memcpy(_charsEnd, firstChar, lastChar - firstChar + 1);

    } else {

        unsigned atEnd = _charsEnd - firstChar;
        memcpy(_charsEnd, firstChar, atEnd);
        memcpy(_charsEnd + atEnd, _chars, lastChar + 1 - _chars);

        charOffsetBeforeFirst = _charsEnd + atEnd - _chars;
    }

    // get values for rendering
    int yStep = textRenderer()->metrics().lineSpacing();
    int yStart = screenHeight - textRenderer()->metrics().descent();

    // render text
    char** line = _linesEnd + showLines;
    int x = screenWidth - _logWidth;

    pthread_mutex_unlock(& _mutex);
    // ok, we got all we need

    for (int y = yStart; y > 0; y -= yStep) {

        // debug mode: check line pointer is valid
        assert(! (line < _linesEnd || line >= _linesEnd + (_linesEnd - _lines)));

        // get character pointer
        if (--line < _linesEnd) {
            break;
        }
        char* chars = *line;

        // debug mode: check char pointer we find is valid
        assert(! (chars < _chars || chars >= _charsEnd));

        // remap character pointer it to copied buffer
        chars += chars >= firstChar ? charOffset : charOffsetBeforeFirst;

        // debug mode: check char pointer is still valid (in new range)
        assert(! (chars < _charsEnd || chars >= _charsEnd + (_charsEnd - _chars)));

        // render the string
        glColor3f(TEXT_RED, TEXT_GREEN, TEXT_BLUE);
        textRenderer()->draw(x, y, chars);

//fprintf(stderr, "Log::render, message = \"%s\"\n", chars);
    }
}

Log logger;

int printLog(char const* fmt, ...) {

    int result;
    va_list args;
    va_start(args,fmt);
    result = logger.vprint(fmt, args);
    va_end(args);
    return result;
}

