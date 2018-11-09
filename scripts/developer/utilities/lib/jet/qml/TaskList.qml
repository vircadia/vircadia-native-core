//
//  jet/TaskList.qml
//
//  Created by Sam Gateau, 2018/03/28
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtQuick.Controls 1.4 as Original
import QtQuick.Controls.Styles 1.4

import stylesUit 1.0
import controlsUit 1.0 as HifiControls

import "../jet.js" as Jet

Rectangle {   
    HifiConstants { id: hifi;}
    color: hifi.colors.baseGray;
    id: root
    property var rootConfig : Workload

    Original.TextArea {
        id: textArea
        width: parent.width
        height: parent.height
        text: ""
    }

    Component.onCompleted: {
        var message = ""
        var functor = Jet.job_print_functor(function (line) { message += line + "\n"; }, false);
        Jet.task_traverseTree(rootConfig, functor);
        textArea.append(message);
    }

    function clearWindow() {
        textArea.remove(0,textArea.length);
    }
}