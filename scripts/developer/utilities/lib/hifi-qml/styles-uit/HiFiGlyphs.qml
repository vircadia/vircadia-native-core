//
//  HiFiGlyphs.qml
//
//  Created by David Rowe on 12 Feb 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

Text {
    id: root
    FontLoader { id: hiFiGlyphs; source: pathToFonts + "fonts/hifi-glyphs.ttf"; }
    property int size: 32
    font.pixelSize: size
    width: size
    height: size
    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignLeft
    font.family: hiFiGlyphs.name
}
