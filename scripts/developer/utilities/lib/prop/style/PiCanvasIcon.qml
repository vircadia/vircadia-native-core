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
    property var filled: true 
    onIconChanged: function () { requestPaint() }
    property var fillColor: global.colorBorderHighight 

    contextType: "2d" 
    onPaint: {
        context.reset();
        switch (icon) {
        case 0:
        case 1:
        case 2:
        default: {
            if ((icon % 3) == 0) {
                context.moveTo(width * 0.25, 0);
                context.lineTo(width * 0.25, height);
                context.lineTo(width, height / 2);
            } else if ((icon % 3) == 1) {
                context.moveTo(0, height * 0.25);
                context.lineTo(width,  height * 0.25);
                context.lineTo(width / 2, height);    
            } else {
                context.moveTo(0, height * 0.75);
                context.lineTo(width, height* 0.75);
                context.lineTo(width / 2, 0);
            }
            context.closePath();
            if (filled) {
                context.fillStyle = fillColor;
                context.fill();
            } else {
                context.strokeStyle = fillColor;
                context.stroke();
            }
        }}
    }
}