//
//  AnimDebugDraw.cpp
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "animdebugdraw_vert.h"
#include "animdebugdraw_frag.h"
#include <gpu/Batch.h>

#include "AnimDebugDraw.h"
#include "AbstractViewStateInterface.h"

struct Vertex {
    glm::vec3 pos;
    uint32_t rgba;
};

class AnimDebugDrawData {
public:
    typedef render::Payload<AnimDebugDrawData> Payload;
    typedef Payload::DataPointer Pointer;

    AnimDebugDrawData() {

        _vertexFormat = std::make_shared<gpu::Stream::Format>();
        _vertexBuffer = std::make_shared<gpu::Buffer>();
        _indexBuffer = std::make_shared<gpu::Buffer>();

        _vertexFormat->setAttribute(gpu::Stream::POSITION, 0, gpu::Element::VEC3F_XYZ, 0);
        _vertexFormat->setAttribute(gpu::Stream::COLOR, 0, gpu::Element::COLOR_RGBA_32, offsetof(Vertex, rgba));
    }

    void render(RenderArgs* args) {
        auto& batch = *args->_batch;
        batch.setPipeline(_pipeline);
        auto transform = Transform{};
        batch.setModelTransform(transform);

        batch.setInputFormat(_vertexFormat);
        batch.setInputBuffer(0, _vertexBuffer, 0, sizeof(Vertex));
        batch.setIndexBuffer(gpu::UINT16, _indexBuffer, 0);

        auto numIndices = _indexBuffer->getSize() / sizeof(uint16_t);
        batch.drawIndexed(gpu::LINES, numIndices);
    }

    gpu::PipelinePointer _pipeline;
    render::ItemID _item;
    gpu::Stream::FormatPointer _vertexFormat;
    gpu::BufferPointer _vertexBuffer;
    gpu::BufferPointer _indexBuffer;
};

typedef render::Payload<AnimDebugDrawData> AnimDebugDrawPayload;

namespace render {
    template <> const ItemKey payloadGetKey(const AnimDebugDrawData::Pointer& data) { return ItemKey::Builder::opaqueShape(); }
    template <> const Item::Bound payloadGetBound(const AnimDebugDrawData::Pointer& data) { return Item::Bound(); }
    template <> void payloadRender(const AnimDebugDrawData::Pointer& data, RenderArgs* args) {
        data->render(args);
    }
}

static AnimDebugDraw* instance = nullptr;

AnimDebugDraw& AnimDebugDraw::getInstance() {
    if (!instance) {
        instance = new AnimDebugDraw();
    }
    return *instance;
}

static uint32_t toRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return ((uint32_t)r | (uint32_t)g << 8 | (uint32_t)b << 16 | (uint32_t)a << 24);
}

gpu::PipelinePointer AnimDebugDraw::_pipeline;

AnimDebugDraw::AnimDebugDraw() :
    _itemID(0) {

    auto state = std::make_shared<gpu::State>();
    state->setCullMode(gpu::State::CULL_BACK);
    state->setDepthTest(true, true, gpu::LESS_EQUAL);
    state->setBlendFunction(false, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD,
                            gpu::State::INV_SRC_ALPHA, gpu::State::FACTOR_ALPHA,
                            gpu::State::BLEND_OP_ADD, gpu::State::ONE);
    auto vertShader = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(animdebugdraw_vert)));
    auto fragShader = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(animdebugdraw_frag)));
    auto program = gpu::ShaderPointer(gpu::Shader::createProgram(vertShader, fragShader));
    _pipeline = gpu::PipelinePointer(gpu::Pipeline::create(program, state));

    _animDebugDrawData = std::make_shared<AnimDebugDrawData>();
    _animDebugDrawPayload = std::make_shared<AnimDebugDrawPayload>(_animDebugDrawData);

    _animDebugDrawData->_pipeline = _pipeline;

    render::ScenePointer scene = AbstractViewStateInterface::instance()->getMain3DScene();
    if (scene) {
        _itemID = scene->allocateID();
        render::PendingChanges pendingChanges;
        pendingChanges.resetItem(_itemID, _animDebugDrawPayload);
        scene->enqueuePendingChanges(pendingChanges);
    }

    // HACK: add red, green and blue axis at (1,1,1)
    _animDebugDrawData->_vertexBuffer->resize(sizeof(Vertex) * 6);
    Vertex* data = (Vertex*)_animDebugDrawData->_vertexBuffer->editData();

    data[0].pos = glm::vec3(1.0, 1.0f, 1.0f);
    data[0].rgba = toRGBA(255, 0, 0, 255);
    data[1].pos = glm::vec3(2.0, 1.0f, 1.0f);
    data[1].rgba = toRGBA(255, 0, 0, 255);

    data[2].pos = glm::vec3(1.0, 1.0f, 1.0f);
    data[2].rgba = toRGBA(0, 255, 0, 255);
    data[3].pos = glm::vec3(1.0, 2.0f, 1.0f);
    data[3].rgba = toRGBA(0, 255, 0, 255);

    data[4].pos = glm::vec3(1.0, 1.0f, 1.0f);
    data[4].rgba = toRGBA(0, 0, 255, 255);
    data[5].pos = glm::vec3(1.0, 1.0f, 2.0f);
    data[5].rgba = toRGBA(0, 0, 255, 255);

    _animDebugDrawData->_indexBuffer->resize(sizeof(uint16_t) * 6);
    uint16_t* indices = (uint16_t*)_animDebugDrawData->_indexBuffer->editData();
    for (int i = 0; i < 6; i++) {
        indices[i] = i;
    }

}

AnimDebugDraw::~AnimDebugDraw() {
    render::ScenePointer scene = AbstractViewStateInterface::instance()->getMain3DScene();
    if (scene && _itemID) {
        render::PendingChanges pendingChanges;
        pendingChanges.removeItem(_itemID);
        scene->enqueuePendingChanges(pendingChanges);
    }
}

void AnimDebugDraw::addSkeleton(std::string key, AnimSkeleton::Pointer skeleton, const AnimPose& rootPose) {
    _skeletons[key] = SkeletonInfo(skeleton, rootPose);
}

void AnimDebugDraw::removeSkeleton(std::string key) {
    _skeletons.erase(key);
}

void AnimDebugDraw::addAnimNode(std::string key, AnimNode::Pointer animNode, const AnimPose& rootPose) {
    _animNodes[key] = AnimNodeInfo(animNode, rootPose);
}

void AnimDebugDraw::removeAnimNode(std::string key) {
    _animNodes.erase(key);
}

static const uint32_t red = toRGBA(255, 0, 0, 255);
static const uint32_t green = toRGBA(0, 255, 0, 255);
static const uint32_t blue = toRGBA(0, 0, 255, 255);
static const uint32_t cyan = toRGBA(0, 128, 128, 255);

static void addWireframeSphereWithAxes(const AnimPose& rootPose, const AnimPose& pose, float radius, Vertex*& v) {

    // x-axis
    v->pos = rootPose * pose.trans;
    v->rgba = red;
    v++;
    v->pos = rootPose * (pose.trans + pose.rot * glm::vec3(radius * 2.0f, 0.0f, 0.0f));
    v->rgba = red;
    v++;

    // y-axis
    v->pos = rootPose * pose.trans;
    v->rgba = green;
    v++;
    v->pos = rootPose * (pose.trans + pose.rot * glm::vec3(0.0f, radius * 2.0f, 0.0f));
    v->rgba = green;
    v++;

    // z-axis
    v->pos = rootPose * pose.trans;
    v->rgba = blue;
    v++;
    v->pos = rootPose * (pose.trans + pose.rot * glm::vec3(0.0f, 0.0f, radius * 2.0f));
    v->rgba = blue;
    v++;
}

static void addWireframeBoneAxis(const AnimPose& rootPose, const AnimPose& pose,
                                 const AnimPose& parentPose, float radius, Vertex*& v) {
    v->pos = rootPose * pose.trans;
    v->rgba = cyan;
    v++;
    v->pos = rootPose * parentPose.trans;
    v->rgba = cyan;
    v++;
}


void AnimDebugDraw::update() {

    render::ScenePointer scene = AbstractViewStateInterface::instance()->getMain3DScene();
    if (!scene) {
        return;
    }

    render::PendingChanges pendingChanges;
    pendingChanges.updateItem<AnimDebugDrawData>(_itemID, [&](AnimDebugDrawData& data) {

        // figure out how many verts we will need.
        int numVerts = 0;
        for (auto&& iter : _skeletons) {
            AnimSkeleton::Pointer& skeleton = iter.second.first;
            numVerts += skeleton->getNumJoints() * 6;
            for (int i = 0; i < skeleton->getNumJoints(); i++) {
                if (skeleton->getParentIndex(i) >= 0) {
                    numVerts += 2;
                }
            }
        }

        for (auto&& iter : _animNodes) {
            AnimNode::Pointer& animNode = iter.second.first;
            auto poses = animNode->getPosesInternal();
            numVerts += poses.size() * 6;
            auto skeleton = animNode->getSkeleton();
            for (int i = 0; i < poses.size(); i++) {
                if (skeleton->getParentIndex(i) >= 0) {
                    numVerts += 2;
                }
            }
        }

        data._vertexBuffer->resize(sizeof(Vertex) * numVerts);
        Vertex* verts = (Vertex*)data._vertexBuffer->editData();
        Vertex* v = verts;
        for (auto&& iter : _skeletons) {
            AnimSkeleton::Pointer& skeleton = iter.second.first;
            AnimPose& rootPose = iter.second.second;

            for (int i = 0; i < skeleton->getNumJoints(); i++) {
                AnimPose pose = skeleton->getAbsoluteBindPose(i);

                // draw axes
                const float radius = 1.0f;
                addWireframeSphereWithAxes(rootPose, pose, radius, v);

                // line to parent.
                if (skeleton->getParentIndex(i) >= 0) {
                    AnimPose parentPose = skeleton->getAbsoluteBindPose(skeleton->getParentIndex(i));
                    addWireframeBoneAxis(rootPose, pose, parentPose, radius, v);
                }
            }
        }

        for (auto&& iter : _animNodes) {
            AnimNode::Pointer& animNode = iter.second.first;
            AnimPose& rootPose = iter.second.second;
            auto poses = animNode->getPosesInternal();

            auto skeleton = animNode->getSkeleton();

            std::vector<AnimPose> absAnimPose;
            absAnimPose.reserve(skeleton->getNumJoints());

            for (size_t i = 0; i < poses.size(); i++) {

                auto parentIndex = skeleton->getParentIndex(i);
                if (parentIndex >= 0) {
                    absAnimPose[i] = absAnimPose[parentIndex] * poses[i];
                } else {
                    absAnimPose[i] = poses[i];
                }

                // draw axes
                const float radius = 1.0f;
                addWireframeSphereWithAxes(rootPose, absAnimPose[i], radius, v);

                if (parentIndex >= 0) {
                    // draw line to parent
                    addWireframeBoneAxis(rootPose, absAnimPose[i], absAnimPose[parentIndex], radius, v);
                }
            }
        }

        data._indexBuffer->resize(sizeof(uint16_t) * numVerts);
        uint16_t* indices = (uint16_t*)data._indexBuffer->editData();
        for (int i = 0; i < numVerts; i++) {
            indices[i] = i;
        }

    });
    scene->enqueuePendingChanges(pendingChanges);
}
