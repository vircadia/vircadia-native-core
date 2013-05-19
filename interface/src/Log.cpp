//
// Log.cpp
// interface
//
// Created by Tobias Schwinger on 4/14/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "Log.h"


#include <string.h>
#include <stdarg.h>
#include <pthread.h>

#include "InterfaceConfig.h"

#include "Util.h"
#include "ui/TextRenderer.h"


namespace logdisplay {

    class Logger {
    public:
        Logger();
        ~Logger();

        inline void render(unsigned screenWidth, unsigned screenHeight);

        inline void setStream(FILE* stream);
        inline void setLogWidth(unsigned pixels);
        inline void setCharacterSize(unsigned width, unsigned height);

        // format, eventually forward, and add the log message (called by printLog)
        inline int  vprint(char const* fmt, va_list);

    private:
        // don't copy/assign
        Logger(Logger const&); // = delete;
        Logger& operator=(Logger const&); // = delete;

        // add formatted message for console diplay (called by vprint)
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


    //
    // Initialization / state management
    //

    Logger::Logger() :

        _textRenderer(MONO_FONT_FAMILY, -1, -1, false, TextRenderer::SHADOW_EFFECT),
        _stream(DEFAULT_STREAM),
        _chars(0l),
        _lines(0l),
        _logWidth(DEFAULT_CONSOLE_WIDTH) {

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

        setCharacterSize(DEFAULT_CHAR_WIDTH, DEFAULT_CHAR_HEIGHT);
    }


    Logger::~Logger() {

        delete[] _chars;
        delete[] _lines;
    }

    inline void Logger::setStream(FILE* stream) {

        pthread_mutex_lock(& _mutex);
        _stream = stream;
        pthread_mutex_unlock(& _mutex);
    }

    inline void Logger::setLogWidth(unsigned pixels) {

        pthread_mutex_lock(& _mutex);
        _logWidth = pixels;
        _lineLength = _logWidth / _charWidth;
        pthread_mutex_unlock(& _mutex);
    }

    inline void Logger::setCharacterSize(unsigned width, unsigned height) {

        pthread_mutex_lock(& _mutex);
        _charWidth = width;
        _charHeight = height;
        _lineLength = _logWidth / _charWidth;
        pthread_mutex_unlock(& _mutex);
    }

    //
    // Logging
    //

    inline int Logger::vprint(char const* fmt, va_list args) {
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

    inline void Logger::addMessage(char const* ptr) {

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

    //
    // Rendering
    //

    inline void Logger::render(unsigned screenWidth, unsigned screenHeight) {

        // rendering might take some time, so create a local copy of the portion we need
        // instead of having to hold the mutex all the time
        pthread_mutex_lock(& _mutex);

        // determine number of visible lines (integer division rounded up)
        unsigned showLines = (screenHeight + _charHeight - 1) / _charHeight;

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

        // determine geometry information from font metrics
        QFontMetrics const& fontMetrics = _textRenderer.metrics();
        int yStep = fontMetrics.lineSpacing();
        // scale
        float xScale = float(_charWidth) / fontMetrics.width('*');
        float yScale = float(_charHeight) / yStep;
        // scaled translation
        int xStart = int((screenWidth - _logWidth) / xScale);
        int yStart = screenHeight / yScale - fontMetrics.descent();

        // first line to render
        char** line = _linesEnd + showLines;

        // ok, now the lock can be released - we have all we need
        // and won't hold it while talking to OpenGL
        pthread_mutex_unlock(& _mutex);

        glPushMatrix();
        glScalef(xScale, yScale, 1.0f);
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
            glColor3f(TEXT_COLOR_RED, TEXT_COLOR_GREEN, TEXT_COLOR_BLUE);
            _textRenderer.draw(xStart, y, chars);

    //fprintf(stderr, "Logger::render, message = \"%s\"\n", chars);
        }
        glPopMatrix();
    }


    // 
    // There's one Logger and it exists globally...
    //
    Logger logger;

    // Entrypoints
    void Render(unsigned screenWidth, unsigned screenHeight)    { logger.render(screenWidth, screenHeight); }
    void SetStream(FILE* stream)                                { logger.setStream(stream); }
    void SetLogWidth(unsigned pixels)                           { logger.setLogWidth(pixels); }
    void SetCharacterSize(unsigned width, unsigned height)      { logger.setCharacterSize(width, height); }

} // namespace logdisplay


int printLog(char const* fmt, ...) {

    int result;
    va_list args;
    va_start(args,fmt);
    result = logdisplay::logger.vprint(fmt, args);
    va_end(args);
    return result;
}

