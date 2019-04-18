//
//  Prop/style/PiFoldCanvas.qml
//
//  Created by Sam Gateau on 3/9/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

 import QtQuick 2.7
import QtQuick.Controls 2.2
 
Canvas {
    Global { id: global }

    width: global.iconWidth
    height: global.iconHeight

    property var icon: 0 
        
    onIconChanged: function () { //console.log("Icon changed to: " + icon );
                                 requestPaint()
                               }

    property var filled: true 
    onFilledChanged: function () { //console.log("Filled changed to: " + filled );
                                   requestPaint()
                                }

    property var fillColor: global.colorBorderHighight 
    onFillColorChanged: function () { //console.log("fillColor changed to: " + filled );
                                     requestPaint()
                                   }

    property alias iconMouseArea: mousearea

    contextType: "2d" 
    onPaint: {
        context.reset();
        switch (icon) {
        case false:
        case 0:{ // Right Arrow
                context.moveTo(width * 0.25, 0);
                context.lineTo(width * 0.25, height);
                context.lineTo(width, height / 2);
                context.closePath();
            } break;
        case true:
        case 1:{ // Up Arrow
                context.moveTo(0, height * 0.25);
                context.lineTo(width,  height * 0.25);
                context.lineTo(width / 2, height);    
                context.closePath();
            } break;
        case 2:{ // Down Arrow
                context.moveTo(0, height * 0.75);
                context.lineTo(width, height* 0.75);
                context.lineTo(width / 2, 0);
                context.closePath();
            } break;
        case 3:{ // Left Arrow
                context.moveTo(width * 0.75, 0);
                context.lineTo(width * 0.75, height);
                context.lineTo(0, height / 2);
                context.closePath();
            } break;
        case 4:{ // Square
                context.moveTo(0,0);
                context.lineTo(0, height);
                context.lineTo(width, height);
                context.lineTo(width, 0);
                context.closePath();
            } break;
        case 5:{ // Small Square
                context.moveTo(width * 0.25, height * 0.25);
                context.lineTo(width * 0.25, height * 0.75);
                context.lineTo(width * 0.75, height * 0.75);
                context.lineTo(width * 0.75, height * 0.25);
                context.closePath();
            } break;
        default: {// Down Arrow
               /* context.moveTo(0, height * 0.25);
                context.lineTo(width,  height * 0.25);
                context.lineTo(width / 2, height);    
                context.closePath();*/
            }
        }
        if (filled) {
            context.fillStyle = fillColor;
            context.fill();
        } else {
            context.strokeStyle = fillColor;
            context.stroke();
        }
    }

    MouseArea{
        id: mousearea
        anchors.fill: parent
    }
}