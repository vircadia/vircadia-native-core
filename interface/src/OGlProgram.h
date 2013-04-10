#ifndef __interface__OpenGlSupport__
#define __interface__OpenGlSupport__

#include "InterfaceConfig.h"

/**
 * Macro to log OpenGl errors to stderr.
 * Example: oglLog( glPushMatrix() );
 */
#define oGlLog(stmt) \
    stmt; \
    { \
        GLenum e = glGetError(); \
        if (e != GL_NO_ERROR) { \
            fprintf(stderr, __FILE__ ":"  oGlLog_stringize(__LINE__) \
                    " [OpenGL] %s\n", gluErrorString(e)); \
        } \
    } \
    (void) 0

#define oGlLog_stringize(x) oGlLog_stringize_i(x)
#define oGlLog_stringize_i(x) # x

/**
 * Encapsulation of the otherwise lengthy call sequence to compile
 * and link shading pipelines.
 */
class OGlProgram {

    GLuint _hndProg;

public:

    OGlProgram() : _hndProg(0) { }

    ~OGlProgram() { if (_hndProg != 0u) { glDeleteProgram(_hndProg); } }

    // no copy/assign
    OGlProgram(OGlProgram const&); // = delete;
    OGlProgram& operator=(OGlProgram const&); // = delete;

#if 0 // let's keep this commented, for now (C++11)

    OGlProgram(OGlProgram&& disposable) : _hndProg(disposable._hndProg) {

        disposable._hndProg = 0;
    }

    OGlProgram& operator=(OGlProgram&& disposable) {

        GLuint tmp = _hndProg;
        _hndProg = disposable._hndProg;
        disposable._hndProg = tmp;
    }
#endif

    /**
     * Activates the executable for rendering.
     * Shaders must be added and linked before this will work.
     */
    void activate() const {

        if (_hndProg != 0u)
            oGlLog( glUseProgram(_hndProg) );
    }

    /**
     * Adds a shader to the program.
     */
    bool addShader(GLenum type, GLchar const* cString) { 

        return addShader(type, 1, & cString);        
    }

    /**
     * Adds a shader to the program and logs to stderr.
     */
    bool addShader(GLenum type, GLsizei nStrings, GLchar const** strings) {

        if (! _hndProg) { _hndProg = glCreateProgram(); }

        GLuint s = glCreateShader(type);
        glShaderSource(s, nStrings, strings, 0l);
        glCompileShader(s);
        GLint status; 
        glGetShaderiv(s, GL_COMPILE_STATUS, & status);
        if (!! status)
            glAttachShader(_hndProg, s);
#ifdef NDEBUG
        else
#endif
            fetchLog(s, glGetShaderiv, glGetShaderInfoLog);
        glDeleteShader(s);
        return !! status;
    }

    /**
     * Links the program and logs to stderr.
     */
    bool link() {

        if (! _hndProg) { return false; }

        glLinkProgram(_hndProg);
        GLint status; 
        glGetProgramiv(_hndProg, GL_LINK_STATUS, & status);
#ifdef NDEBUG
        if (status == 0)
#endif
            fetchLog(_hndProg, glGetProgramiv, glGetProgramInfoLog);

        return status != 0;
    }

private:

    template< typename ParamFunc, typename GetLogFunc >
    void fetchLog(GLint handle, ParamFunc getParam, GetLogFunc getLog) {
        GLint logLength = 0;
        getParam(handle, GL_INFO_LOG_LENGTH, &logLength);
        if (!! logLength) {
            GLchar* message = new GLchar[logLength];
            getLog(handle, logLength, 0l, message);
            fprintf(stderr, "%s\n", message);
            delete[] message;
        }
    }
};

#endif
