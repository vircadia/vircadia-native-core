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

//
// Log function. Call it as you would call 'printf'.
//
int printLog(char const* fmt, ...);

//
// Logging control.
//
namespace logdisplay {

    void Render(unsigned screenWidth, unsigned screenHeight);

    // settings 

    static float const TEXT_COLOR_RED           = 0.7f;     // text foreground color, red component
    static float const TEXT_COLOR_GREEN         = 0.6f;     // text foreground color, green component
    static float const TEXT_COLOR_BLUE          = 1.0f;     // text foregdound color, blue component

    static FILE*          DEFAULT_STREAM        = stdout;   // stream to also log to
    static unsigned const DEFAULT_CHAR_WIDTH    = 7;        // width of a single character
    static unsigned const DEFAULT_CHAR_HEIGHT   = 16;       // height of a single character
    static unsigned const DEFAULT_CONSOLE_WIDTH = 400;      // width of the (right-aligned) log console

    void SetStream(FILE* stream);
    void SetLogWidth(unsigned pixels);
    void SetCharacterSize(unsigned width, unsigned height);

    // limits

    unsigned const CHARACTER_BUFFER_SIZE    = 16384;        // number of character that are buffered
    unsigned const LINE_BUFFER_SIZE         = 256;          // number of lines that are buffered
    unsigned const MAX_MESSAGE_LENGTH       = 512;          // maximum number of characters for a message
}

// 
// Macro to log OpenGL errors.
// Example: printGlError( glPushMatrix() );
//
#define printLogGlError(stmt) \
    stmt; \
    { \
        GLenum e = glGetError(); \
        if (e != GL_NO_ERROR) { \
            printLog(__FILE__ ":"  printLogGlError_stringize(__LINE__) \
                     " [OpenGL] %s\n", gluErrorString(e)); \
        } \
    } \
    (void) 0

#define printLogGlError_stringize(x) printLogGlError_stringize_i(x)
#define printLogGlError_stringize_i(x) # x

#endif

