//
//  TabletsScrollingWindow.qml
//
//  Created by Dante Ruiz on 9 Feb 2017
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtGraphicalEffects 1.0

import "."
import "../../../styles-uit"
import "../../../controls-uit" as HiFiControls
Item {
    
    id: window
    HifiConstants { id: hifi }

    children: [ pane ]
    property var footer: Item {}
    property var pane: Item {
        anchors.fill: parent
        
        Rectangle {
            id: contentBackground
            anchors.fill: parent
            width: 480
            //gradient: Gradient {
                //GradientStop {
                    //position: 0
                    //color: "#2b2b2b"
                    
                //}
                
                //GradientStop {
                    //position: 1
                    //color: "#0f212e"
                //}
            //}
        }
    }
}
