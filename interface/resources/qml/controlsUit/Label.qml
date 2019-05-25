//
//  Label.qml
//
//  Created by David Rowe on 26 Feb 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7

import "../stylesUit"

RalewaySemiBold {
    HifiConstants { id: hifi }
    property int colorScheme: hifi.colorSchemes.light

    size: hifi.fontSizes.inputLabel
    color: {
        if (colorScheme === hifi.colorSchemes.dark) {
            if (enabled) {
                hifi.colors.lightGrayText
            } else {
                hifi.colors.baseGrayHighlight
            }
        } else {
            if (enabled) {
                hifi.colors.lightGray
            } else {
                hifi.colors.lightGrayText
            }
        }
    }
}
