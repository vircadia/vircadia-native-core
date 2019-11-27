#include "GLHelpers.h"

#include <mutex>

#include "Config.h"

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QRegularExpression>
#include <QtCore/QProcessEnvironment>

#include <QtGui/QSurfaceFormat>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLDebugLogger>

#include "Context.h"

size_t evalGLFormatSwapchainPixelSize(const QSurfaceFormat& format) {
    size_t pixelSize = format.redBufferSize() + format.greenBufferSize() + format.blueBufferSize() + format.alphaBufferSize();
    // We don't apply the length of the swap chain into this pixelSize since it is not vsible for the Process (on windows).
    // Let s keep this here remember that:
    // if (format.swapBehavior() > 0) {
    //     pixelSize *= format.swapBehavior(); // multiply the color buffer pixel size by the actual swapchain depth
    // }
    pixelSize += format.stencilBufferSize() + format.depthBufferSize();
    return pixelSize;
}

static bool FORCE_DISABLE_OPENGL_45 = false;

void gl::setDisableGl45(bool disable) {
    FORCE_DISABLE_OPENGL_45 = disable;
}

bool gl::disableGl45() {
#if defined(USE_GLES)
    return false;
#else
    static const QString DEBUG_FLAG("HIFI_DISABLE_OPENGL_45");
    static bool disableOpenGL45 = QProcessEnvironment::systemEnvironment().contains(DEBUG_FLAG);
    return FORCE_DISABLE_OPENGL_45 || disableOpenGL45;
#endif
}

#ifdef Q_OS_MAC
#define SERIALIZE_GL_RENDERING
#endif

#ifdef SERIALIZE_GL_RENDERING

// This terrible terrible hack brought to you by the complete lack of reasonable
// OpenGL debugging tools on OSX.  Without this serialization code, the UI textures
// frequently become 'glitchy' and get composited onto the main scene in what looks
// like a partially rendered state.
// This looks very much like either state bleeding across the contexts, or bad
// synchronization for the shared OpenGL textures.  However, previous attempts to resolve
// it, even with gratuitous use of glFinish hasn't improved the situation

static std::mutex _globalOpenGLLock;

void gl::globalLock() {
    _globalOpenGLLock.lock();
}

void gl::globalRelease(bool finish) {
    if (finish) {
        glFinish();
    }
    _globalOpenGLLock.unlock();
}

#else

void gl::globalLock() {}
void gl::globalRelease(bool finish) {}

#endif


uint16_t gl::getTargetVersion() {
    uint8_t major = 0, minor = 0;

#if defined(USE_GLES)
    major = 3;
    minor = 2;
#elif defined(Q_OS_MAC)
    major = 4;
    minor = 1;
#else
    major = 4;
    minor = disableGl45() ? 1 : 5;
#endif
    return GL_MAKE_VERSION(major, minor);
}

uint16_t gl::getRequiredVersion() {
    uint8_t major = 0, minor = 0;
#if defined(USE_GLES)
    major = 3;
    minor = 2;
#else 
    major = 4;
    minor = 1;
#endif
    return GL_MAKE_VERSION(major, minor);
}

#if defined(Q_OS_WIN)

typedef BOOL(APIENTRYP PFNWGLCHOOSEPIXELFORMATARBPROC)(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
typedef HGLRC(APIENTRYP PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC hDC, HGLRC hShareContext, const int *attribList);
GLAPI PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
GLAPI PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;

static bool setupPixelFormatSimple(HDC hdc) {
    // FIXME build the PFD based on the 
    static const PIXELFORMATDESCRIPTOR pfd =    // pfd Tells Windows How We Want Things To Be
    {
        sizeof(PIXELFORMATDESCRIPTOR),         // Size Of This Pixel Format Descriptor
        1,                                      // Version Number
        PFD_DRAW_TO_WINDOW |                    // Format Must Support Window
        PFD_SUPPORT_OPENGL |                    // Format Must Support OpenGL
        PFD_DOUBLEBUFFER,                       // Must Support Double Buffering
        PFD_TYPE_RGBA,                          // Request An RGBA Format
        24,                                     // Select Our Color Depth
        0, 0, 0, 0, 0, 0,                       // Color Bits Ignored
        1,                                      // Alpha Buffer
        0,                                      // Shift Bit Ignored
        0,                                      // No Accumulation Buffer
        0, 0, 0, 0,                             // Accumulation Bits Ignored
        24,                                     // 24 Bit Z-Buffer (Depth Buffer)  
        8,                                      // 8 Bit Stencil Buffer
        0,                                      // No Auxiliary Buffer
        PFD_MAIN_PLANE,                         // Main Drawing Layer
        0,                                      // Reserved
        0, 0, 0                                 // Layer Masks Ignored
    };
    auto pixelFormat = ChoosePixelFormat(hdc, &pfd);
    if (pixelFormat == 0) {
        return false;
    }

    if (SetPixelFormat(hdc, pixelFormat, &pfd) == FALSE) {
        return false;
    }
    return true;
}

#endif


const gl::ContextInfo& gl::ContextInfo::get(bool init) {
    static gl::ContextInfo INSTANCE;
    if (init) {
        static std::once_flag once;
        std::call_once(once, [&] {
            INSTANCE.init();
        });
    }
    return INSTANCE;
}

gl::ContextInfo& gl::ContextInfo::init() {
    if (glGetString) {
        version = (const char*)glGetString(GL_VERSION);
        shadingLanguageVersion = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
        vendor = (const char*)glGetString(GL_VENDOR);
        renderer = (const char*)glGetString(GL_RENDERER);
        GLint n = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &n);
        for (GLint i = 0; i < n; ++i) {
            extensions.emplace_back((const char*)glGetStringi(GL_EXTENSIONS, i));
        }
    }
    return *this;
}

uint16_t gl::getAvailableVersion() {
    static uint8_t major = 0, minor = 0;
    static std::once_flag once;
    std::call_once(once, [&] {
#if defined(USE_GLES)
        // FIXME do runtime detection of the available GL version
        major = 3;
        minor = 2;
#elif defined(Q_OS_MAC)
        // Damn it Apple.
        major = 4;
        minor = 1;
#elif defined(Q_OS_WIN)
        // 
        HINSTANCE hInstance = GetModuleHandle(nullptr);
        const auto windowClassName = "OpenGLVersionCheck";
        WNDCLASS wc = { };
        wc.lpfnWndProc   = DefWindowProc;
        wc.hInstance     = hInstance;
        wc.lpszClassName = windowClassName;
        RegisterClass(&wc);

        using Handle = std::shared_ptr<void>;
        HWND rawHwnd = CreateWindowEx(
            WS_EX_APPWINDOW, // extended style 
            windowClassName, // class name
            windowClassName, // title
            WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CS_OWNDC | WS_POPUP, // style
            0, 0, 10, 10,   // position and size
            NULL, NULL, hInstance, NULL);
        auto WindowDestroyer = [](void* handle) {
            DestroyWindow((HWND)handle);
        };
        Handle hWnd = Handle(rawHwnd, WindowDestroyer);
        if (!hWnd) {
            return;
        }
        HDC rawDC = GetDC(rawHwnd);
        auto DCDestroyer = [=](void* handle) {
            ReleaseDC(rawHwnd, (HDC)handle);
        };
        if (!rawDC) {
            return;
        }
        Handle hDC = Handle(rawDC, DCDestroyer);
        if (!setupPixelFormatSimple(rawDC)) {
            return;
        }
        auto GLRCDestroyer = [](void* handle) {
            wglDeleteContext((HGLRC)handle);
        };
        auto rawglrc = wglCreateContext(rawDC);
        if (!rawglrc) {
            return;
        }
        Handle hGLRC = Handle(rawglrc, GLRCDestroyer);
        if (!wglMakeCurrent(rawDC, rawglrc)) {
            return;
        }
        gl::initModuleGl();

        std::string glvendor{ (const char*)glGetString(GL_VENDOR) };
        std::transform(glvendor.begin(), glvendor.end(), glvendor.begin(), ::tolower); 

        // Intel has *notoriously* buggy DSA implementations, especially around cubemaps
        if (std::string::npos != glvendor.find("intel")) {
            gl::setDisableGl45(true);
        }

        wglMakeCurrent(0, 0);
        hGLRC.reset();
        if (!wglChoosePixelFormatARB || !wglCreateContextAttribsARB) {
            return;
        }

        // The only two versions we care about on Windows 
        // are 4.5 and 4.1
        if (GLAD_GL_VERSION_4_5) {
            major = 4;
            minor = disableGl45() ? 1 : 5;
        } else if (GLAD_GL_VERSION_4_1) {
            major = 4;
            minor = 1;
        }
#else
        // FIXME do runtime detection of GL version on non-Mac/Windows/Mobile platforms
        major = 4;
        minor = disableGl45() ? 1 : 5;
#endif
    });
    return GL_MAKE_VERSION(major, minor);
}

const QSurfaceFormat& getDefaultOpenGLSurfaceFormat() {
    static QSurfaceFormat format;
    static std::once_flag once;
    std::call_once(once, [] {
#if defined(USE_GLES)
        format.setRenderableType(QSurfaceFormat::OpenGLES);
        format.setRedBufferSize(8);
        format.setGreenBufferSize(8);
        format.setBlueBufferSize(8);
        format.setAlphaBufferSize(8);
#else
        format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
#endif
        if (gl::Context::enableDebugLogger()) {
            format.setOption(QSurfaceFormat::DebugContext);
        }
        // Qt Quick may need a depth and stencil buffer. Always make sure these are available.
        format.setDepthBufferSize(DEFAULT_GL_DEPTH_BUFFER_BITS);
        format.setStencilBufferSize(DEFAULT_GL_STENCIL_BUFFER_BITS);
        auto glversion = ::gl::getTargetVersion();
        format.setMajorVersion(GL_GET_MAJOR_VERSION(glversion));
        format.setMinorVersion(GL_GET_MINOR_VERSION(glversion));
    });
    return format;
}

int glVersionToInteger(QString glVersion) {
    QStringList versionParts = glVersion.split(QRegularExpression("[\\.\\s]"));
    int majorNumber = 0, minorNumber = 0;
    if (versionParts.size() >= 2) {
        majorNumber = versionParts[0].toInt();
        minorNumber = versionParts[1].toInt();
    }
    return (majorNumber << 16) | minorNumber;
}

QThread* RENDER_THREAD = nullptr;

bool isRenderThread() {
    return QThread::currentThread() == RENDER_THREAD;
}

#if defined(Q_OS_ANDROID)
#define USE_GLES 1
#endif

namespace gl {
    void withSavedContext(const std::function<void()>& f) {
        // Save the original GL context, because creating a QML surface will create a new context
        QOpenGLContext * savedContext = QOpenGLContext::currentContext();
        QSurface * savedSurface = savedContext ? savedContext->surface() : nullptr;
        f();
        if (savedContext) {
            savedContext->makeCurrent(savedSurface);
        }
    }

    bool checkGLError(const char* name) {
        GLenum error = glGetError();
        if (!error) {
            return false;
        } 
        switch (error) {
            case GL_INVALID_ENUM:
                qCWarning(glLogging) << "GLBackend" << name << ": An unacceptable value is specified for an enumerated argument.The offending command is ignored and has no other side effect than to set the error flag.";
                break;
            case GL_INVALID_VALUE:
                qCWarning(glLogging) << "GLBackend" << name << ": A numeric argument is out of range.The offending command is ignored and has no other side effect than to set the error flag";
                break;
            case GL_INVALID_OPERATION:
                qCWarning(glLogging) << "GLBackend" << name << ": The specified operation is not allowed in the current state.The offending command is ignored and has no other side effect than to set the error flag..";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                qCWarning(glLogging) << "GLBackend" << name << ": The framebuffer object is not complete.The offending command is ignored and has no other side effect than to set the error flag.";
                break;
            case GL_OUT_OF_MEMORY:
                qCWarning(glLogging) << "GLBackend" << name << ": There is not enough memory left to execute the command.The state of the GL is undefined, except for the state of the error flags, after this error is recorded.";
                break;
#if !defined(USE_GLES)
            case GL_STACK_UNDERFLOW:
                qCWarning(glLogging) << "GLBackend" << name << ": An attempt has been made to perform an operation that would cause an internal stack to underflow.";
                break;
            case GL_STACK_OVERFLOW:
                qCWarning(glLogging) << "GLBackend" << name << ": An attempt has been made to perform an operation that would cause an internal stack to overflow.";
                break;
#endif
        }
        return true;
    }


    bool checkGLErrorDebug(const char* name) {
        // Disabling error checking macro on Android debug builds for now, 
        // as it throws off performance testing, which must be done on 
        // Debug builds
#if defined(DEBUG) && !defined(Q_OS_ANDROID)
        return checkGLError(name);
#else
        Q_UNUSED(name);
        return false;
#endif
    }

    // Enables annotation of captures made by tools like renderdoc
    bool khrDebugEnabled() {
        static std::once_flag once;
        static bool khrDebug = false;
        std::call_once(once, [&] {
            khrDebug = nullptr != glPushDebugGroupKHR;
        });
        return khrDebug;
    }

    // Enables annotation of captures made by tools like renderdoc
    bool extDebugMarkerEnabled() {
        static std::once_flag once;
        static bool extMarker = false;
        std::call_once(once, [&] {
            extMarker = nullptr != glPushGroupMarkerEXT;
        });
        return extMarker;
    }

    bool debugContextEnabled() {
#if defined(Q_OS_MAC)
        // OSX does not support GL_KHR_debug or GL_ARB_debug_output
        static bool enableDebugLogger = false;
#elif defined(DEBUG) || defined(USE_GLES)
        //static bool enableDebugLogger = true;
        static bool enableDebugLogger = false;
#else
        static const QString DEBUG_FLAG("HIFI_DEBUG_OPENGL");
        static bool enableDebugLogger = QProcessEnvironment::systemEnvironment().contains(DEBUG_FLAG);
#endif
        return enableDebugLogger;
    }
}
