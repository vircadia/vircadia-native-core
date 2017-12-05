//
//  XColor.qml
//
//  Created by Sam Gateau 12/4/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtQuick.Controls 1.4 as Original
import QtQuick.Controls.Styles 1.4

import "qrc:///qml/styles-uit"
import "qrc:///qml/controls-uit" as HifiControls


Item {
    HifiConstants { id: hifi }
    id: root

    anchors.left: parent.left
    anchors.right: parent.right    
    height: 24
    property var color
    property var zoneWidth: width / 3;
    property var hoveredOn: 0.0;
    property var sliderHeight: height / 3;

    signal newColor( color la_color)
    function getColor() {
        return Qt.rgba(color.red / 255.0, color.green / 255.0, color.blue / 255.0, 1.0 );
    }

    function repaint() {
        current.color = getColor()
    }
    function setRed(r) {
        color.red = r * 255;
        repaint()
        print("set red " + r)
    }
    function setGreen(g) {
        color.green = g * 255;
        repaint()
        print("set green " + g)
    }
    function setBlue(b) {
        color.blue = b * 255;
        repaint()
        print("set blue " + b)
    }

    function resetSliders() {
        redZone.set(color.red / 255) 
        greenZone.set(color.green / 255) 
        blueZone.set(color.blue / 255)          
    }

    Rectangle {
        id: current
        anchors.fill: root
        color: root.getColor();
    }
    Rectangle {
        id: sliderBack
        height: root.sliderHeight
        anchors.bottom: root.bottom
        anchors.left: root.left
        anchors.right: root.right
        color: Qt.rgba(0.2, 0.2, 0.2, 1)
        opacity: root.hoveredOn * 0.5
    }

    MouseArea {
        id: all
        anchors.fill: root
        hoverEnabled: true
        onEntered: {
            root.hoveredOn = 1.0;
            resetSliders();
        }
        onExited: {
            root.hoveredOn = 0.0;
        }
    } 

    Component.onCompleted: {
        // Binding favors qml value, so set it first
        bindingControl.when = true;
    }

    Item {
        id: redZone
        anchors.top: root.top
        anchors.bottom: root.bottom
        anchors.left: root.left
        width: root.zoneWidth  

        function set(r) {
            if (r < 0.0) {
                r = 0.0
            } else if (r > 1.0) {
                r = 1.0
            }
            root.setRed(r)
            redRect.width = r * redZone.width
            redRect.color = Qt.rgba(r, 0, 0, 1) 
        }

        Rectangle {
            id: redRect
            anchors.bottom: parent.bottom
            anchors.left: parent.left   
            height: root.sliderHeight
            opacity: root.hoveredOn
        }

        MouseArea {
            id: redArea
            anchors.fill: parent
            onPositionChanged: {
                redZone.set(mouse.x / redArea.width)
            }   
        }
    }
    Item {
        id: greenZone
        anchors.top: root.top
        anchors.bottom: root.bottom
        anchors.left: redZone.right
        
        width: root.zoneWidth  

        function set(g) {
            if (g < 0.0) {
                g = 0.0
            } else if (g > 1.0) {
                g = 1.0
            }
            root.setGreen(g)
            greenRect.width = g * greenZone.width
            greenRect.color = Qt.rgba(0, g, 0, 1) 
        }

        Rectangle {
            id: greenRect
            anchors.bottom: parent.bottom
            anchors.left: parent.left   
            height: root.sliderHeight
            opacity: root.hoveredOn
        }

        MouseArea {
            id: greenArea
            anchors.fill: parent
            onPositionChanged: {
                greenZone.set(mouse.x / greenArea.width)
            }     
        }
    }
    Item {
        id: blueZone
        anchors.top: root.top
        anchors.bottom: root.bottom
        anchors.right: root.right
       // anchors.left: greenZone.right
        
        width: root.zoneWidth  

        function set(b) {
            if (b < 0.0) {
                b = 0.0
            } else if (b > 1.0) {
                b = 1.0
            }
            root.setBlue(b)
            blueRect.width = b * blueZone.width
            blueRect.color = Qt.rgba(0, 0, b, 1) 
        }

        Rectangle {
            id: blueRect
            anchors.bottom: parent.bottom
            anchors.left: parent.left   
            height: root.sliderHeight
            opacity: root.hoveredOn
        }

        MouseArea {
            id: blueArea
            anchors.fill: parent
            onPositionChanged: {
                blueZone.set(mouse.x / blueArea.width)
            }
        }
    }    
}
