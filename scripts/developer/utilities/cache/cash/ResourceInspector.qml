//
//  ResourceInspector.qml
//
//  Created by Sam Gateau on 2019-09-24
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import QtQml.Models 2.12

import "../../lib/prop" as Prop

Item {
    id: root;
    Prop.Global { id: global }

    anchors.fill: parent.fill
    property var cache: {}
    property string cacheResourceName: "" 

    function fromScript(message) {
        switch (message.method) {
        case "inspectResource":
            inspectResource(message.params.url, message.params.semantic)
            break;
        }
    }

    function inspectResource(url, semantic) {
        console.log("inspectResource :" + url + " as " + semantic)
        info.text = "url: " + url + "\nsemantic: " + semantic + "\n";

        if (semantic == "Texture") {
            var res = TextureCache.prefetch(url, 0)
            info.text += JSON.stringify(res);
        } else if (semantic == "Model") {
            var res = ModelCache.prefetch(url)
            info.text += JSON.stringify(res);
        }
    }

    TextEdit {
            id: info
            anchors.fill: parent
            text: "Click an object to get material JSON"
            width: root.width
            font.pointSize: 10
            color: "#FFFFFF"
            readOnly: true
            selectByMouse: true
            wrapMode: Text.WordWrap
    }
}

