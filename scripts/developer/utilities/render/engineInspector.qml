//
//  deferredLighting.qml
//
//  Created by Sam Gateau on 6/6/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3

import stylesUit 1.0
import controlsUit 1.0 as HifiControls

import "../lib/jet/qml" as Jet

Item {
    HifiConstants { id: hifi;}
    id: render;   
    anchors.fill: parent

    property var mainViewTask: Render.getConfig("RenderMainView")

    Jet.TaskListView {
        rootConfig: Render
        anchors.fill: render        
    }
}