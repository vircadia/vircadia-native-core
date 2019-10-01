//
//  TextureCacheInspector.qml
//
//  Created by Sam Gateau on 2019-09-17
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.7
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3

import "../../lib/prop" as Prop

ResourceCacheInspector {
    id: root;
    anchors.fill: parent.fill
    cache: TextureCache
    cacheResourceName: "Texture"
}
