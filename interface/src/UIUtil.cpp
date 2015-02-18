//
//  UIUtil.cpp
//  interface/src
//
//  Created by Ryan Huffman on 09/02/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QStyle>
#include <QStyleOptionTitleBar>

#include "DependencyManager.h"
#include "GLCanvas.h"

#include "UIUtil.h"

int UIUtil::getWindowTitleBarHeight(const QWidget* window) {
    QStyleOptionTitleBar options;
    options.titleBarState = 1;
    options.titleBarFlags = Qt::Window;
    int titleBarHeight = window->style()->pixelMetric(QStyle::PM_TitleBarHeight, &options, window);

#if defined(Q_OS_MAC)
    // The height on OSX is 4 pixels too tall
    titleBarHeight -= 4;
#endif

    return titleBarHeight;
}

// When setting the font size of a widget in points, the rendered text will be larger
// on Windows and Linux than on Mac OSX.  This is because Windows and Linux use a DPI
// of 96, while OSX uses 72.
// In order to get consistent results across platforms, the font sizes need to be scaled
// based on the system DPI.
// This function will scale the font size of widget and all
// of its children recursively.
void UIUtil::scaleWidgetFontSizes(QWidget* widget) {
    auto glCanvas = DependencyManager::get<GLCanvas>();

    // This is the base dpi that we are targetting.  This is based on Mac OSXs default DPI,
    // and is the basis for all font sizes.
    const float BASE_DPI = 72.0f;

#ifdef Q_OS_MAC
    const float NATIVE_DPI = 72.0f;
#else
    const float NATIVE_DPI = 96.0f;
#endif

    // Scale fonts based on the native dpi.  On Windows, where the native DPI is 96,
    // the scale will be: 72.0 / 96.0 = 0.75
    float fontScale = (BASE_DPI / NATIVE_DPI);

    // Scale the font further by the system's DPI settings.  If using a 2x high-dpi screen
    // on Windows, for example, the font will be further scaled by: 192.0 / 96.0 = 2.0
    // This would give a final scale of: 0.75 * 2.0 = 1.5
    fontScale *= (glCanvas->logicalDpiX() / NATIVE_DPI);

    qDebug() << "Scaling widgets by: " << fontScale;

    internalScaleWidgetFontSizes(widget, fontScale);
}

void UIUtil::internalScaleWidgetFontSizes(QWidget* widget, float scale) {
    for (auto child : widget->findChildren<QWidget*>()) {
        if (child->parent() == widget) {
            internalScaleWidgetFontSizes(child, scale);
        }
    }

    QFont font = widget->font();
    font.setPointSizeF(font.pointSizeF() * scale);
    widget->setFont(font);
}
