//
//  UIUtil.cpp
//  library/shared/src
//
//  Created by Ryan Huffman on 09/02/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QStyle>
#include <QStyleOptionTitleBar>

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
