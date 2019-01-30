//
//  main.cpp
//  tests/gpu-test/src
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <unordered_map>
#include <memory>
#include <cstdio>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QLoggingCategory>

#include <QtGui/QDesktopServices>
#include <QtGui/QResizeEvent>
#include <QtGui/QWindow>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtGui/QScreen>

#include <QtWidgets/QApplication>

#include <gl/Config.h>

#include <QtQuick/QQuickWindow>
#include <QtQuick/QQuickView>
#include <QtQml/QQmlContext>

#include <gpu/Context.h>
#include <gpu/Batch.h>
#include <gpu/Stream.h>
#include <gpu/gl/GLBackend.h>

#include <gl/QOpenGLContextWrapper.h>
#include <gl/GLHelpers.h>

#include <GLMHelpers.h>
#include <PathUtils.h>
#include <NumericalConstants.h>

#include <PerfStat.h>
#include <PathUtils.h>
#include <SharedUtil.h>
#include <ViewFrustum.h>

#include <gpu/Pipeline.h>
#include <gpu/Context.h>

#include "TestWindow.h"
#include "TestTextures.h"

using TestBuilder = std::function<GpuTestBase*()>;
using TestBuilders = std::list<TestBuilder>;

#define INTERACTIVE

class MyTestWindow : public TestWindow {
    using Parent = TestWindow;
    TestBuilders _testBuilders;
    GpuTestBase* _currentTest{ nullptr };
    size_t _currentTestId{ 0 };
    size_t _currentMaxTests{ 0 };
    glm::mat4 _camera;
    QTime _time;

    void initGl() override {
        Parent::initGl();
        _time.start();
        updateCamera();
        _testBuilders = TestBuilders({
            [] { return new TexturesTest(); },
        });
    }

    void updateCamera() {
        float t = _time.elapsed() * 1e-3f;
        static const glm::vec3 up{ 0.0f, 1.0f, 0.0f };

        float distance = 3.0f;
        glm::vec3 camera_position{ distance * sinf(t), 0.5f, distance * cosf(t) };

        static const vec3 camera_focus(0);
        _camera = glm::inverse(glm::lookAt(camera_position, camera_focus, up));

        ViewFrustum frustum;
        frustum.setPosition(camera_position);
        frustum.setOrientation(glm::quat_cast(_camera));
        frustum.setProjection(_projectionMatrix);
    }

    void renderFrame() override {
        updateCamera();

        while ((!_currentTest || (_currentTestId >= _currentMaxTests)) && !_testBuilders.empty()) {
            if (_currentTest) {
                delete _currentTest;
                _currentTest = nullptr;
            }

            _currentTest = _testBuilders.front()();
            _testBuilders.pop_front();

            if (_currentTest) {
                auto statsObject = _currentTest->statsObject();
                QUrl url = _currentTest->statUrl();
                if (statsObject) {
                    auto screens = qApp->screens();
                    auto primaryScreen = qApp->primaryScreen();
                    auto targetScreen = primaryScreen;
                    for (const auto& screen : screens) {
                        if (screen == primaryScreen) {
                            continue;
                        }
                        targetScreen = screen;
                        break;
                    }

                    auto destPoint = targetScreen->availableGeometry().topLeft();
                    QQuickView* view = new QQuickView();
                    view->rootContext()->setContextProperty("Stats", statsObject);
                    view->setSource(url);
                    view->show();
                    view->setPosition({ destPoint.x() + 100, destPoint.y() + 100 });
                }
                _currentMaxTests = _currentTest->getTestCount();
                _currentTestId = 0;
            }
        }

        if (!_currentTest && _testBuilders.empty()) {
            qApp->quit();
            return;
        }

        // Tests might need to wait for resources to download
        if (!_currentTest->isReady()) {
            return;
        }

        gpu::doInBatch("main::renderFrame", _renderArgs->_context, [&](gpu::Batch& batch) {
            batch.setViewTransform(_camera);
            _renderArgs->_batch = &batch;
            _currentTest->renderTest(_currentTestId, *_renderArgs);
            _renderArgs->_batch = nullptr;
        });
    }
};

int main(int argc, char** argv) {
    setupHifiApplication("GPU Test");
    qputenv("HIFI_DEBUG_OPENGL", QByteArray("1"));
    QApplication app(argc, argv);
    MyTestWindow window;
    app.exec();

    return 0;
}
