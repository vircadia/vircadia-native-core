//
//  Created by Bradley Austin Davis on 2018-01-04
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <QtQuick/QQuickRenderControl>

namespace hifi { namespace qml { namespace impl {

class RenderControl : public QQuickRenderControl {
public:
    RenderControl(QObject* parent = Q_NULLPTR);
    void setRenderWindow(QWindow* renderWindow);

protected:
    QWindow* renderWindow(QPoint* offset) override;

private:
    QWindow* _renderWindow{ nullptr };
};

}}}  // namespace hifi::qml::impl
