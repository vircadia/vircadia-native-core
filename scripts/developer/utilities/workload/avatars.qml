//
//  avatars.qml
//  scripts/developer/utilities/workload
//
//  Created by Sam Gateau on 2018.11.28
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4

import stylesUit 1.0
import controlsUit 1.0 as HifiControls

import "../lib/plotperf"
import "../render/configSlider"

Item {
    id: root
    anchors.fill:parent

    Component.onCompleted: {
    }

    Component.onDestruction: {
    }

    Column {
        id: topHeader
        spacing: 8
        anchors.right: parent.right
        anchors.left: parent.left
    }
      
    Column {
        id: stats
        spacing: 4
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: topHeader.bottom
        anchors.bottom: parent.bottom

        function evalEvenHeight() {
            // Why do we have to do that manually ? cannot seem to find a qml / anchor / layout mode that does that ?
            var numPlots = (children.length + - 2)
            return (height - topLine.height - bottomLine.height - spacing * (numPlots - 1)) / (numPlots)
        }

        Separator {
            id: topLine
        }

        PlotPerf {
            title: "Avatars"
            height: parent.evalEvenHeight()
            object: Stats
            valueScale: 1
            valueUnit: "num"
            plots: [
                {
                    prop: "updatedAvatarCount",
                    label: "updatedAvatarCount",
                    color: "#FFFF00"
                },
                {
                    prop: "notUpdatedAvatarCount",
                    label: "notUpdatedAvatarCount",
                    color: "#00FF00"
                }
            ]
        }
        Separator {
            id: bottomLine
        }
    }
}
