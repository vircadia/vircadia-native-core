#include "TestCase.h"

#include <qml/OffscreenSurface.h>

class MacQml : public TestCase {
    using Parent = TestCase;
public:
    GLuint _texture{ 0 };
    QmlPtr _surface;
    GLuint _fbo{ 0 };

    MacQml(QWindow* window) : Parent(window) {}
    void update() override;
    void init() override;
    void draw() override;
};
