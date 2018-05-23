#include "GLShaders.h"

#include "GLLogging.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonValue>
#include <QtCore/QJsonObject>
#include <QtCore/QFileInfo>
#include <QtCore/QCryptographicHash>

#include <shared/FileUtils.h>

using namespace gl;

void Uniform::load(GLuint glprogram, int index) {
    const GLint NAME_LENGTH = 256;
    GLchar glname[NAME_LENGTH];
    GLint length = 0;
    glGetActiveUniform(glprogram, index, NAME_LENGTH, &length, &size, &type, glname);
    name = std::string(glname, length);
    location = glGetUniformLocation(glprogram, glname);
}

Uniforms gl::loadUniforms(GLuint glprogram) {
    GLint uniformsCount = 0;
    glGetProgramiv(glprogram, GL_ACTIVE_UNIFORMS, &uniformsCount);

    Uniforms result;
    result.resize(uniformsCount);
    for (int i = 0; i < uniformsCount; i++) {
        result[i].load(glprogram, i);
    }
    return result;
}

#ifdef SEPARATE_PROGRAM
bool gl::compileShader(GLenum shaderDomain,
                   const std::string& shaderSource,
                   GLuint& shaderObject,
                   GLuint& programObject,
                   std::string& message) {
    return compileShader(shaderDomain, std::vector<std::string>{ shaderSource }, shaderObject, programObject, message);
}
#else
bool gl::compileShader(GLenum shaderDomain, const std::string& shaderSource, GLuint& shaderObject, std::string& message) {
    return compileShader(shaderDomain, std::vector<std::string>{ shaderSource }, shaderObject, message);
}
#endif

#ifdef SEPARATE_PROGRAM
bool gl::compileShader(GLenum shaderDomain,
                   const std::string& shaderSource,
                   GLuint& shaderObject,
                   GLuint& programObject,
                   std::string& message) {
#else
bool gl::compileShader(GLenum shaderDomain,
                   const std::vector<std::string>& shaderSources,
                   GLuint& shaderObject,
                   std::string& message) {
#endif
    if (shaderSources.empty()) {
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
    std::vector<const GLchar*> cstrs;
    for (const auto& str : shaderSources) {
        cstrs.push_back(str.c_str());
    }
    glShaderSource(glshader, static_cast<GLint>(cstrs.size()), cstrs.data(), NULL);

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
            for (const auto& s : cstrs) {
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

GLuint gl::compileProgram(const std::vector<GLuint>& glshaders, std::string& message, CachedShader& cachedShader) {
    // A brand new program:
    GLuint glprogram = glCreateProgram();
    if (!glprogram) {
        qCDebug(glLogging) << "GLShader::compileProgram - failed to create the gl program object";
        return 0;
    }

    bool binaryLoaded = false;

    if (glshaders.empty() && cachedShader) {
        glProgramBinary(glprogram, cachedShader.format, cachedShader.binary.data(), (GLsizei)cachedShader.binary.size());
        binaryLoaded = true;
    } else {
        // glProgramParameteri(glprogram, GL_PROGRAM_, GL_TRUE);
        // Create the program from the sub shaders
        for (auto so : glshaders) {
            glAttachShader(glprogram, so);
        }

        // Link!
        glLinkProgram(glprogram);
    }

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
    if (linked && !binaryLoaded) {
        GLint binaryLength = 0;
        glGetProgramiv(glprogram, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
        if (binaryLength > 0) {
            cachedShader.binary.resize(binaryLength);
            glGetProgramBinary(glprogram, binaryLength, NULL, &cachedShader.format, cachedShader.binary.data());
        }
    }

    return glprogram;
}

const QString& getShaderCacheFile() {
    static const QString SHADER_CACHE_FOLDER{ "shaders" };
    static const QString SHADER_CACHE_FILE_NAME{ "cache.json" };
    static const QString SHADER_CACHE_FILE = FileUtils::standardPath(SHADER_CACHE_FOLDER) + SHADER_CACHE_FILE_NAME;
    return SHADER_CACHE_FILE;
}

static const char* SHADER_JSON_TYPE_KEY = "type";
static const char* SHADER_JSON_SOURCE_KEY = "source";
static const char* SHADER_JSON_DATA_KEY = "data";

void gl::loadShaderCache(ShaderCache& cache) {
    QString shaderCacheFile = getShaderCacheFile();
    if (QFileInfo(shaderCacheFile).exists()) {
        QString json = FileUtils::readFile(shaderCacheFile);
        auto root = QJsonDocument::fromJson(json.toUtf8()).object();
        for (const auto& qhash : root.keys()) {
            auto programObject = root[qhash].toObject();
            QByteArray qbinary = QByteArray::fromBase64(programObject[SHADER_JSON_DATA_KEY].toString().toUtf8());
            std::string hash = qhash.toStdString();
            auto& cachedShader = cache[hash];
            cachedShader.binary.resize(qbinary.size());
            memcpy(cachedShader.binary.data(), qbinary.data(), qbinary.size());
            cachedShader.format = (GLenum)programObject[SHADER_JSON_TYPE_KEY].toInt();
            cachedShader.source = programObject[SHADER_JSON_SOURCE_KEY].toString().toStdString();
        }
    }
}

void gl::saveShaderCache(const ShaderCache& cache) {
    QByteArray json;
    {
        QVariantMap variantMap;
        for (const auto& entry : cache) {
            const auto& key = entry.first;
            const auto& type = entry.second.format;
            const auto& binary = entry.second.binary;
            QVariantMap qentry;
            qentry[SHADER_JSON_TYPE_KEY] = QVariant(type);
            qentry[SHADER_JSON_SOURCE_KEY] = QString(entry.second.source.c_str());
            qentry[SHADER_JSON_DATA_KEY] = QByteArray{ binary.data(), (int)binary.size() }.toBase64();
            variantMap[key.c_str()] = qentry;
        }
        json = QJsonDocument::fromVariant(variantMap).toJson(QJsonDocument::Indented);
    }

    if (!json.isEmpty()) {
        QString shaderCacheFile = getShaderCacheFile();
        QFile saveFile(shaderCacheFile);
        saveFile.open(QFile::WriteOnly | QFile::Text | QFile::Truncate);
        saveFile.write(json);
        saveFile.close();
    }
}

std::string gl::getShaderHash(const std::string& shaderSource) {
    return QCryptographicHash::hash(QByteArray(shaderSource.c_str()), QCryptographicHash::Md5).toBase64().toStdString();
}
