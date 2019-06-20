//
//  Color.qml
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

import stylesUit 1.0
import controlsUit 1.0 as HifiControls


Item {
    HifiConstants { id: hifi }
    id: root

    height: 24

    property var _color: Qt.rgba(1.0, 1.0, 1.0, 1.0 );
    property var zoneWidth: width / 3;
    property var hoveredOn: 0.0;
    property var sliderHeight: height / 2;

    signal newColor(color __color)

    function setColor(color) {
        _color = Qt.rgba(color.r, color.g, color.b, 1.0)
        updateColor()
    }  
    function setRed(r) {
        _color.r = r;
        updateColor()
    }
    function setGreen(g) {
        _color.g = g;
        updateColor()
    }
    function setBlue(b) {
        _color.b = b;
        updateColor()
    }

    function updateColor() {
        repaint()
        newColor(_color)
    } 
    function repaint() {
        current.color = _color
    }

    function resetSliders() {
        redZone.set(_color.r) 
        greenZone.set(_color.g) 
        blueZone.set(_color.b)          
    }

    function setXColor(xcolor) {
        setColor(Qt.rgba(xcolor.red/255, xcolor.green/255, color.blue/255, 1.0))
    }
    function getXColor() {
        return {red:_color.r * 255, green:_color.g * 255, blue:_color.b * 255}
    }

    Rectangle {
        id: current
        anchors.fill: root
        color: root._color;
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
    }

    Item {
        id: redZone
        anchors.top: root.top
        anchors.bottom: root.bottom
        anchors.left: root.left
        width: root.zoneWidth  

        function update(r) {
            if (r < 0.0) {
                r = 0.0
            } else if (r > 1.0) {
                r = 1.0
            }
            root.setRed(r)
            set(r)
        }
        function set(r) {
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
                redZone.update(mouse.x / redArea.width)
            }   
        }
    }
    Item {
        id: greenZone
        anchors.top: root.top
        anchors.bottom: root.bottom
        anchors.horizontalCenter: root.horizontalCenter
        width: root.zoneWidth  

        function update(g) {
            if (g < 0.0) {
                g = 0.0
            } else if (g > 1.0) {
                g = 1.0
            }
            root.setGreen(g)
            set(g)
        }
        function set(g) {
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
                greenZone.update(mouse.x / greenArea.width)
            }     
        }
    }
    Item {
        id: blueZone
        anchors.top: root.top
        anchors.bottom: root.bottom
        anchors.right: root.right     
        width: root.zoneWidth  

        function update(b) {
            if (b < 0.0) {
                b = 0.0
            } else if (b > 1.0) {
                b = 1.0
            }
            root.setBlue(b)
            set(b)
        }
        function set(b) {
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
                blueZone.update(mouse.x / blueArea.width)
            }
        }
    }    
}
