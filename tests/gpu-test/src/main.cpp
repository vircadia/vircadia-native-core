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

#include <QtGui/QResizeEvent>
#include <QtGui/QWindow>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>

#include <gpu/Context.h>
#include <gpu/Batch.h>
#include <gpu/Stream.h>
#include <gpu/gl/GLBackend.h>

#include <gl/QOpenGLContextWrapper.h>
#include <gl/GLHelpers.h>

#include <GLMHelpers.h>
#include <PathUtils.h>
#include <NumericalConstants.h>

#include <GeometryCache.h>
#include <DeferredLightingEffect.h>
#include <FramebufferCache.h>
#include <TextureCache.h>
#include <PerfStat.h>
#include <PathUtils.h>
#include <OctreeRenderer.h>
#include <RenderArgs.h>
#include <ViewFrustum.h>

#include <gpu/Pipeline.h>
#include <gpu/Context.h>

#include <render/Engine.h>
#include <render/Scene.h>
#include <render/CullTask.h>
#include <render/SortTask.h>
#include <render/DrawTask.h>
#include <render/DrawStatus.h>
#include <render/DrawSceneOctree.h>
#include <render/CullTask.h>

#include "TestWindow.h"
#include "TestFbx.h"
#include "TestFloorGrid.h"
#include "TestFloorTexture.h"
#include "TestInstancedShapes.h"
#include "TestShapes.h"

using namespace render;


using TestBuilder = std::function<GpuTestBase*()>;
using TestBuilders = std::list<TestBuilder>;


#define INTERACTIVE

class MyTestWindow : public TestWindow {
    using Parent = TestWindow;
    TestBuilders _testBuilders;
    GpuTestBase* _currentTest { nullptr };
    size_t _currentTestId { 0 };
    size_t _currentMaxTests { 0 };
    glm::mat4 _camera;
    QTime _time;

    void initGl() override {
        Parent::initGl();
#ifdef INTERACTIVE
        _time.start();
#endif
        updateCamera();
        _testBuilders = TestBuilders({
            [this] { return new TestFbx(_shapePlumber); },
            [] { return new TestInstancedShapes(); },
        });
    }

    void updateCamera() {
        float t = 0;
#ifdef INTERACTIVE
        t = _time.elapsed() * 1e-3f;
#endif
        glm::vec3 unitscale { 1.0f };
        glm::vec3 up { 0.0f, 1.0f, 0.0f };

        float distance = 3.0f;
        glm::vec3 camera_position { distance * sinf(t), 0.5f, distance * cosf(t) };

        static const vec3 camera_focus(0);
        static const vec3 camera_up(0, 1, 0);
        _camera = glm::inverse(glm::lookAt(camera_position, camera_focus, up));

        ViewFrustum frustum;
        frustum.setPosition(camera_position);
        frustum.setOrientation(glm::quat_cast(_camera));
        frustum.setProjection(_projectionMatrix);
        _renderArgs->setViewFrustum(frustum);
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

        gpu::doInBatch(_renderArgs->_context, [&](gpu::Batch& batch) {
            batch.setViewTransform(_camera);
            _renderArgs->_batch = &batch;
            _currentTest->renderTest(_currentTestId, _renderArgs);
            _renderArgs->_batch = nullptr;
        });

#ifdef INTERACTIVE

#else 
        // TODO Capture the current rendered framebuffer and save
        // Increment the test ID
        ++_currentTestId;
#endif
        }
};

extern bool needsSparseRectification(const uvec2& size);
extern uvec2 rectifyToSparseSize(const uvec2& size);

void testSparseRectify() {
    std::vector<std::pair<uvec2, bool>> NEEDS_SPARSE_TESTS {{
        // Already sparse
        { {1024, 1024 }, false },
        { { 128, 128 }, false },
        // Too small in one dimension
        { { 127, 127 }, false },
        { { 1, 1 }, false },
        { { 1000, 1 }, false },
        { { 1024, 1 }, false },
        { { 100, 100 }, false },
        // needs rectification
        { { 1000, 1000 }, true },
        { { 1024, 1000 }, true },
    } };

    for (const auto& test : NEEDS_SPARSE_TESTS) {
        const auto& size = test.first;
        const auto& expected = test.second;
        auto result = needsSparseRectification(size);
        Q_ASSERT(expected == result);
        result = needsSparseRectification(uvec2(size.y, size.x));
        Q_ASSERT(expected == result);
    }

    std::vector<std::pair<uvec2, uvec2>> SPARSE_SIZE_TESTS { {
        // needs rectification
        { { 1000, 1000 }, { 1024, 1024 } },
        { { 1024, 1000 }, { 1024, 1024 } },
    } };

    for (const auto& test : SPARSE_SIZE_TESTS) {
        const auto& size = test.first;
        const auto& expected = test.second;
        auto result = rectifyToSparseSize(size);
        Q_ASSERT(expected == result);
        result = rectifyToSparseSize(uvec2(size.y, size.x));
        Q_ASSERT(expected == uvec2(result.y, result.x));
    }
}

int main(int argc, char** argv) {   
    testSparseRectify();

    // FIXME this test appears to be broken
#if 0
    QGuiApplication app(argc, argv);
    MyTestWindow window;
    app.exec();
#endif
    return 0;
}

