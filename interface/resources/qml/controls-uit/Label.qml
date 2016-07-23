//
//  Label.qml
//
//  Created by David Rowe on 26 Feb 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5

import "../styles-uit"

RalewaySemiBold {
    property int colorScheme: hifi.colorSchemes.light

    size: hifi.fontSizes.inputLabel
    color: enabled ? (colorScheme == hifi.colorSchemes.light ? hifi.colors.lightGray : hifi.colors.lightGrayText)
                   : (colorScheme == hifi.colorSchemes.light ? hifi.colors.lightGrayText : hifi.colors.baseGrayHighlight);
}
