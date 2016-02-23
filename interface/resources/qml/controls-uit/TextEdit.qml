//
//  TextEdit.qml
//
//  Created by Bradley Austin Davis on 24 Apr 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.3 as Original
import "../styles"

Original.TextEdit {
    HifiConstants { id: hifi }
    font.family: hifi.fonts.fontFamily
    font.pixelSize: hifi.fonts.pixelSize
}
