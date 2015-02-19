//
//  UIUtil.h
//  interface/src
//
//  Created by Ryan Huffman on 09/02/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_UIUtil_h
#define hifi_UIUtil_h

#include <QWidget>

class UIUtil {
public:
    static int getWindowTitleBarHeight(const QWidget* window);

};

#endif // hifi_UIUtil_h
