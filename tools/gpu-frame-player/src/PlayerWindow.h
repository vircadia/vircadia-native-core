//
//  Created by Bradley Austin Davis on 2018/10/21
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#include <QtGui/QWindow>
#include <QtCore/QSettings>

#include <gpu/Forward.h>
#include "RenderThread.h"

// Create a simple OpenGL window that renders text in various ways
class PlayerWindow : public QWindow {
public:

    PlayerWindow();
    virtual ~PlayerWindow();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* ev) override;
    void loadFrame();
    void loadFrame(const QString& path);

private:
    static void textureLoader(const std::vector<uint8_t>& filename, const gpu::TexturePointer& texture, uint16_t layer);
    QSettings _settings;
    RenderThread _renderThread;
};
