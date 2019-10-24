#include "TestCase.h"

#include <array>

#include <qml/OffscreenSurface.h>

#define DIVISIONS_X 5
#define DIVISIONS_Y 5

class StressWeb : public TestCase {
    using Parent = TestCase;
public:
    using QmlPtr = QSharedPointer<hifi::qml::OffscreenSurface>;

    struct QmlInfo {
        QmlPtr surface;
        GLuint texture{ 0 };
        uint64_t lifetime{ 0 };
    };

    size_t _surfaceCount{ 0 };
    uint64_t _createStopTime{ 0 };
    const QSize _qmlSize{ 640, 480 };
    std::array<std::array<QmlInfo, DIVISIONS_Y>, DIVISIONS_X> _surfaces;
    GLuint _fbo{ 0 };

    StressWeb(QWindow* window) : Parent(window) {}
    static QString getSourceUrl(bool video);
    void buildSurface(QmlInfo& qmlInfo, bool video);
    void destroySurface(QmlInfo& qmlInfo);
    void update() override;
    void init() override;
    void draw() override;
};
