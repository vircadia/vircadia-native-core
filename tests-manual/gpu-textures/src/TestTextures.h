//
//  Created by Bradley Austin Davis on 2016/05/16
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "TestHelpers.h"

#define STATS_PROPERTY(type, name, initialValue) \
    Q_PROPERTY(type name READ name NOTIFY name##Changed) \
public: \
    type name() { return _##name; }; \
private: \
    type _##name{ initialValue };


class TextureTestStats : public QObject {
    Q_OBJECT;
    STATS_PROPERTY(int, pending, 0)
    STATS_PROPERTY(int, total, 0)
    STATS_PROPERTY(int, populated, 0)
    STATS_PROPERTY(int, allocated, 0)
    STATS_PROPERTY(int, index, 0)

    STATS_PROPERTY(QString, source, QString())
    STATS_PROPERTY(QSize, rez, QSize(0, 0))

public:
    void update(int index, const gpu::TexturePointer& texture);

signals:
    void pendingChanged();
    void totalChanged();
    void populatedChanged();
    void allocatedChanged();
    void changeTextures();
    void rezChanged();
    void indexChanged();
    void sourceChanged();
    void maxTextureMemory(int);

    void nextTexture();
    void prevTexture();
};


class TexturesTest : public GpuTestBase {
    Q_OBJECT

    gpu::Stream::FormatPointer vertexFormat { std::make_shared<gpu::Stream::Format>() };
    std::vector<gpu::TexturePointer> textures;
    std::vector<gpu::TexturePointer> oldTextures;
    gpu::PipelinePointer pipeline;
    TextureTestStats stats;
    size_t index{ 0 };

public:
    TexturesTest();
    QObject* statsObject() override { return &stats; }
    QUrl statUrl() override { return QUrl::fromLocalFile(projectRootDir() + "/qml/textureStats.qml"); }
    void renderTest(size_t testId, const RenderArgs& args) override;

protected slots:
    void onChangeTextures();
    void onMaxTextureMemory(int newValue);
    void onNextTexture();
    void onPrevTexture();

};


