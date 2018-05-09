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

#include "UIUtil.h"

#include <QStyle>
#include <QStyleOptionTitleBar>

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
// of 96, while OSX uses 72.  In order to get consistent results across platforms, the
// font sizes need to be scaled based on the system DPI.
// This function will scale the font size of widget and all
// of its children recursively.
//
// When creating widgets, if a font size isn't explicitly set Qt will choose a
// reasonable, but often different font size across platforms.  This means
// YOU SHOUD EXPLICITLY SET ALL FONT SIZES and call this function OR not use
// this function at all.  If you mix both you will end up with inconsistent results
// across platforms.
void UIUtil::scaleWidgetFontSizes(QWidget* widget) {
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
    float fontScale = BASE_DPI / NATIVE_DPI;

    internalScaleWidgetFontSizes(widget, fontScale);
}

// Depth-first traversal through widget hierarchy.  It is important to do a depth-first
// traversal because modifying the font size of a parent node can affect children.
void UIUtil::internalScaleWidgetFontSizes(QWidget* widget, float scale) {
    for (auto child : widget->findChildren<QWidget*>()) {
        if (child->parent() == widget) {
            internalScaleWidgetFontSizes(child, scale);
        }
    }

    QFont font = widget->font();
    font.setPointSizeF(font.pointSizeF() * (double)scale);
    widget->setFont(font);
}
