#include "GLShaders.h"

#include "GLLogging.h"

namespace gl {


#ifdef SEPARATE_PROGRAM
    bool compileShader(GLenum shaderDomain, const std::string& shaderSource, const std::string& defines, GLuint &shaderObject, GLuint &programObject, std::string& message) {
#else
    bool compileShader(GLenum shaderDomain, const std::string& shaderSource, const std::string& defines, GLuint &shaderObject, std::string& message) {
#endif
    if (shaderSource.empty()) {
        qCDebug(glLogging) << "GLShader::compileShader - no GLSL shader source code ? so failed to create";
        return false;
    }

    // Create the shader object
    GLuint glshader = glCreateShader(shaderDomain);
    if (!glshader) {
        qCDebug(glLogging) << "GLShader::compileShader - failed to create the gl shader object";
        return false;
    }

    // Assign the source
    const int NUM_SOURCE_STRINGS = 2;
    const GLchar* srcstr[] = { defines.c_str(), shaderSource.c_str() };
    glShaderSource(glshader, NUM_SOURCE_STRINGS, srcstr, NULL);

    // Compile !
    glCompileShader(glshader);

    // check if shader compiled
    GLint compiled = 0;
    glGetShaderiv(glshader, GL_COMPILE_STATUS, &compiled);

    GLint infoLength = 0;
    glGetShaderiv(glshader, GL_INFO_LOG_LENGTH, &infoLength);

    if ((infoLength > 0) || !compiled) {
        char* temp = new char[infoLength];
        glGetShaderInfoLog(glshader, infoLength, NULL, temp);

        message = std::string(temp);

        // if compilation fails
        if (!compiled) {
            // save the source code to a temp file so we can debug easily
            /*
            std::ofstream filestream;
            filestream.open("debugshader.glsl");
            if (filestream.is_open()) {
            filestream << srcstr[0];
            filestream << srcstr[1];
            filestream.close();
            }
            */

            /*
            filestream.open("debugshader.glsl.info.txt");
            if (filestream.is_open()) {
            filestream << std::string(temp);
            filestream.close();
            }
            */

            qCCritical(glLogging) << "GLShader::compileShader - failed to compile the gl shader object:";
            int lineNumber = 0;
            for (auto s : srcstr) {
                QString str(s);
                QStringList lines = str.split("\n");
                for (auto& line : lines) {
                    qCCritical(glLogging).noquote() << QString("%1: %2").arg(lineNumber++, 5, 10, QChar('0')).arg(line);
                }
            }
            qCCritical(glLogging) << "GLShader::compileShader - errors:";
            qCCritical(glLogging) << temp;

            delete[] temp;
            glDeleteShader(glshader);
            return false;
        }

        // Compilation success
        qCWarning(glLogging) << "GLShader::compileShader - Success:";
        qCWarning(glLogging) << temp;
        delete[] temp;
    }

#ifdef SEPARATE_PROGRAM
    GLuint glprogram = 0;
    // so far so good, program is almost done, need to link:
    GLuint glprogram = glCreateProgram();
    if (!glprogram) {
        qCDebug(glLogging) << "GLShader::compileShader - failed to create the gl shader & gl program object";
        return false;
    }

    glProgramParameteri(glprogram, GL_PROGRAM_SEPARABLE, GL_TRUE);
    glAttachShader(glprogram, glshader);
    glLinkProgram(glprogram);

    GLint linked = 0;
    glGetProgramiv(glprogram, GL_LINK_STATUS, &linked);

    if (!linked) {
        /*
        // save the source code to a temp file so we can debug easily
        std::ofstream filestream;
        filestream.open("debugshader.glsl");
        if (filestream.is_open()) {
        filestream << shaderSource->source;
        filestream.close();
        }
        */

        GLint infoLength = 0;
        glGetProgramiv(glprogram, GL_INFO_LOG_LENGTH, &infoLength);

        char* temp = new char[infoLength];
        glGetProgramInfoLog(glprogram, infoLength, NULL, temp);

        qCDebug(glLogging) << "GLShader::compileShader -  failed to LINK the gl program object :";
        qCDebug(glLogging) << temp;

        /*
        filestream.open("debugshader.glsl.info.txt");
        if (filestream.is_open()) {
        filestream << String(temp);
        filestream.close();
        }
        */
        delete[] temp;

        glDeleteShader(glshader);
        glDeleteProgram(glprogram);
        return false;
    }
    programObject = glprogram;
#endif
    shaderObject = glshader;
    return true;
}

GLuint compileProgram(const std::vector<GLuint>& glshaders, std::string& message, std::vector<GLchar>& binary) {
    // A brand new program:
    GLuint glprogram = glCreateProgram();
    if (!glprogram) {
        qCDebug(glLogging) << "GLShader::compileProgram - failed to create the gl program object";
        return 0;
    }

    // glProgramParameteri(glprogram, GL_PROGRAM_, GL_TRUE);
    // Create the program from the sub shaders
    for (auto so : glshaders) {
        glAttachShader(glprogram, so);
    }

    // Link!
    glLinkProgram(glprogram);

    GLint linked = 0;
    glGetProgramiv(glprogram, GL_LINK_STATUS, &linked);

    GLint infoLength = 0;
    glGetProgramiv(glprogram, GL_INFO_LOG_LENGTH, &infoLength);

    if ((infoLength > 0) || !linked) {
        char* temp = new char[infoLength];
        glGetProgramInfoLog(glprogram, infoLength, NULL, temp);

        message = std::string(temp);

        if (!linked) {
            /*
            // save the source code to a temp file so we can debug easily
            std::ofstream filestream;
            filestream.open("debugshader.glsl");
            if (filestream.is_open()) {
            filestream << shaderSource->source;
            filestream.close();
            }
            */

            qCDebug(glLogging) << "GLShader::compileProgram -  failed to LINK the gl program object :";
            qCDebug(glLogging) << temp;

            delete[] temp;

            /*
            filestream.open("debugshader.glsl.info.txt");
            if (filestream.is_open()) {
            filestream << std::string(temp);
            filestream.close();
            }
            */

            glDeleteProgram(glprogram);
            return 0;
        } else {
            qCDebug(glLogging) << "GLShader::compileProgram -  success:";
            qCDebug(glLogging) << temp;
            delete[] temp;
        }
    }

    // If linked get the binaries
    if (linked) {
        GLint binaryLength = 0;
        glGetProgramiv(glprogram, GL_PROGRAM_BINARY_LENGTH, &binaryLength);

        if (binaryLength > 0) {
            GLint numBinFormats = 0;
            glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &numBinFormats);
            if (numBinFormats > 0) {
                binary.resize(binaryLength);
                std::vector<GLint> binFormats(numBinFormats);
                glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, binFormats.data());

                GLenum programBinFormat;
                glGetProgramBinary(glprogram, binaryLength, NULL, &programBinFormat, binary.data());
            }
        }
    }

    return glprogram;
}

}
