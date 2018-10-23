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
    this->index = index;
    if (index >= 0) {
        static const GLint NAME_LENGTH = 1024;
        GLchar glname[NAME_LENGTH];
        memset(glname, 0, NAME_LENGTH);
        GLint length = 0;
        glGetActiveUniform(glprogram, index, NAME_LENGTH, &length, &size, &type, glname);
        // Length does NOT include the null terminator
        // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetActiveUniform.xhtml
        name = std::string(glname, length);
        binding = glGetUniformLocation(glprogram, name.c_str());
    }
}

bool isTextureType(GLenum type) {
    switch (type) {
#ifndef USE_GLES
        case GL_SAMPLER_1D:
        case GL_SAMPLER_1D_ARRAY:
        case GL_SAMPLER_1D_SHADOW:
        case GL_SAMPLER_1D_ARRAY_SHADOW:
#endif
        case GL_SAMPLER_2D:
        case GL_SAMPLER_3D:
        case GL_SAMPLER_CUBE:
        case GL_SAMPLER_2D_ARRAY:
        case GL_SAMPLER_CUBE_MAP_ARRAY:
        case GL_SAMPLER_2D_SHADOW:
        case GL_SAMPLER_2D_ARRAY_SHADOW:
        case GL_SAMPLER_BUFFER:
            return true;
        default:
            break;
    }
    return false;
}

Uniforms Uniform::load(GLuint glprogram, const std::function<bool(const Uniform&)>& filter) {
    Uniforms result;
    GLint uniformsCount = 0;
    glGetProgramiv(glprogram, GL_ACTIVE_UNIFORMS, &uniformsCount);
    result.reserve(uniformsCount);
    for (int i = 0; i < uniformsCount; i++) {
        result.emplace_back(glprogram, i);
    }
    result.erase(std::remove_if(result.begin(), result.end(), filter), result.end());
    return result;
}


Uniforms Uniform::loadTextures(GLuint glprogram) {
    return load(glprogram, [](const Uniform& uniform) -> bool {
        if (std::string::npos != uniform.name.find('.')) {
            return true;
        }
        if (std::string::npos != uniform.name.find('[')) {
            return true;
        }
        if (!isTextureType(uniform.type)) {
            return true;
        }
        return false;
    });
}

Uniforms Uniform::load(GLuint glprogram) {
    return load(glprogram, [](const Uniform& uniform) -> bool {
        if (std::string::npos != uniform.name.find('.')) {
            return true;
        }
        if (std::string::npos != uniform.name.find('[')) {
            return true;
        }
        if (isTextureType(uniform.type)) {
            return true;
        }
        return false;
    });
}

Uniforms Uniform::load(GLuint glprogram, const std::vector<GLuint>& indices) {
    Uniforms result;
    result.reserve(indices.size());
    for (const auto& i : indices) {
        if (i == GL_INVALID_INDEX) {
            continue;
        }
        result.emplace_back(glprogram, i);
    }
    return result;
}

Uniform Uniform::loadByName(GLuint glprogram, const std::string& name) {
    GLuint index;
    const char* nameCStr = name.c_str();
    glGetUniformIndices(glprogram, 1, &nameCStr, &index);
    Uniform result;
    if (index != GL_INVALID_INDEX) {
        result.load(glprogram, index);
    }
    return result;
}


Uniforms Uniform::load(GLuint glprogram, const std::vector<const char*>& cnames) {
    GLsizei count = static_cast<GLsizei>(cnames.size());
    if (0 == count) {
        return {};
    }
    std::vector<GLuint> indices;
    indices.resize(count);
    glGetUniformIndices(glprogram, count, cnames.data(), indices.data());
    return load(glprogram, indices);
}


template <typename C, typename F>
std::vector<const char*> toCNames(const C& container, F lambda) {
    std::vector<const char*> result;
    result.reserve(container.size());
    std::transform(container.begin(), container.end(), std::back_inserter(result), lambda);
    return result;
}

Uniforms Uniform::load(GLuint glprogram, const std::vector<std::string>& names) {
    auto cnames = toCNames(names, [](const std::string& name) { return name.c_str(); });
    return load(glprogram, cnames);
}

void UniformBlock::load(GLuint glprogram, int index) {
    this->index = index;
    GLint length = 0;

    // Length DOES include the null terminator
    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetActiveUniformBlock.xhtml
    glGetActiveUniformBlockiv(glprogram, index, GL_UNIFORM_BLOCK_NAME_LENGTH, &length);
    if (length > 1) {
        std::vector<char> nameBuffer;
        nameBuffer.resize(length);
        glGetActiveUniformBlockName(glprogram, index, length, nullptr, nameBuffer.data());
        name = std::string(nameBuffer.data(), length - 1);
    }
    glGetActiveUniformBlockiv(glprogram, index, GL_UNIFORM_BLOCK_BINDING, &binding);
    glGetActiveUniformBlockiv(glprogram, index, GL_UNIFORM_BLOCK_DATA_SIZE, &size);
}

UniformBlocks UniformBlock::load(GLuint glprogram) {
    GLint buffersCount = -1;
    glGetProgramiv(glprogram, GL_ACTIVE_UNIFORM_BLOCKS, &buffersCount);

    // fast exit
    if (buffersCount <= 0) {
        return {};
    }

    UniformBlocks uniformBlocks;
    for (int i = 0; i < buffersCount; ++i) {
        uniformBlocks.emplace_back(glprogram, i);
    }
    return uniformBlocks;
}

void Input::load(GLuint glprogram, int index) {
    const GLint NAME_LENGTH = 256;
    GLchar name[NAME_LENGTH];
    GLint length = 0;
    // Length does NOT include the null terminator
    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetActiveAttrib.xhtml
    glGetActiveAttrib(glprogram, index, NAME_LENGTH, &length, &size, &type, name);
    if (length > 0) {
        this->name = std::string(name, length);
    }
    binding = glGetAttribLocation(glprogram, name);
}

Inputs Input::load(GLuint glprogram) {
    Inputs result;
    GLint count;
    glGetProgramiv(glprogram, GL_ACTIVE_ATTRIBUTES, &count);
    for (int i = 0; i < count; ++i) {
        result.emplace_back(glprogram, i);
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

    getShaderInfoLog(glshader, message);
    // if compilation fails
    if (!compiled) {
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
        qCCritical(glLogging) << message.c_str();
        glDeleteShader(glshader);
        return false;
    }

    if (!message.empty()) {
        // Compilation success
        qCWarning(glLogging) << "GLShader::compileShader - Success:";
        qCWarning(glLogging) << message.c_str();
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

void gl::getShaderInfoLog(GLuint glshader, std::string& message) {
    std::string result;
    GLint infoLength = 0;
    glGetShaderiv(glshader, GL_INFO_LOG_LENGTH, &infoLength);
    if (infoLength > 0) {
        char* temp = new char[infoLength];
        glGetShaderInfoLog(glshader, infoLength, NULL, temp);
        message = std::string(temp);
        delete[] temp;
    } else {
        message.clear();
    }
}

void gl::getProgramInfoLog(GLuint glprogram, std::string& message) {
    std::string result;
    GLint infoLength = 0;
    glGetProgramiv(glprogram, GL_INFO_LOG_LENGTH, &infoLength);
    if (infoLength > 0) {
        char* temp = new char[infoLength];
        glGetProgramInfoLog(glprogram, infoLength, NULL, temp);
        message = std::string(temp);
        delete[] temp;
    } else {
        message.clear();
    }
}

void gl::getProgramBinary(GLuint glprogram, CachedShader& cachedShader) {
    GLint binaryLength = 0;
    glGetProgramiv(glprogram, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
    if (binaryLength > 0) {
        cachedShader.binary.resize(binaryLength);
        glGetProgramBinary(glprogram, binaryLength, NULL, &cachedShader.format, cachedShader.binary.data());
    } else {
        cachedShader.binary.clear();
        cachedShader.format = 0;
    }
}

GLuint gl::buildProgram(const std::vector<GLuint>& glshaders) {
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

    return glprogram;
}


GLuint gl::buildProgram(const CachedShader& cachedShader) {
    // A brand new program:
    GLuint glprogram = glCreateProgram();
    if (!glprogram) {
        qCDebug(glLogging) << "GLShader::compileProgram - failed to create the gl program object";
        return 0;
    }
    glProgramBinary(glprogram, cachedShader.format, cachedShader.binary.data(), (GLsizei)cachedShader.binary.size());
    GLint linked = 0;
    glGetProgramiv(glprogram, GL_LINK_STATUS, &linked);
    if (!linked) {
        glDeleteProgram(glprogram);
        return 0;
    }

    return glprogram;
}


bool gl::linkProgram(GLuint glprogram, std::string& message) {
    glLinkProgram(glprogram);

    GLint linked = 0;
    glGetProgramiv(glprogram, GL_LINK_STATUS, &linked);
    ::gl::getProgramInfoLog(glprogram, message);
    if (!linked) {
        qCDebug(glLogging) << "GLShader::compileProgram -  failed to LINK the gl program object :";
        qCDebug(glLogging) << message.c_str();
        return false;
    }

    if (!message.empty()) {
        qCDebug(glLogging) << "GLShader::compileProgram -  success:";
        qCDebug(glLogging) << message.c_str();
    }

    return true;
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
#if !defined(DISABLE_QML)
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
#endif
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
