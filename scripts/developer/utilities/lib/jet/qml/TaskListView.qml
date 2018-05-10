//
//  jet/TaskListView.qml
//
//  Created by Sam Gateau, 2018/05/09
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtQuick.Controls 1.4 as Original
import QtQuick.Controls.Styles 1.4

import "qrc:///qml/styles-uit"
import "qrc:///qml/controls-uit" as HifiControls

import "../jet.js" as Jet

Rectangle {
    HifiConstants { id: hifi;}
    color: hifi.colors.baseGray;
    id: root

 //   width: parent ? parent.width : 200
 //   height: parent ? parent.height : 400
    property var rootConfig : Workload
    property var myArray : []

    Component.onCompleted: {
        var message = ""
        var functor = Jet.job_print_functor(function (line) { message += line + "\n"; }, false);
        
        functor = Jet.job_list_functor(root.myArray);

        Jet.task_traverseTree(rootConfig, functor);
        //print(JSON.stringify(root.myArray))
     //   theView.model = root.myArray.length
        for (var i = 0; i < root.myArray.length; i++) {
           jobsModel.append({"on": true, "name": root.myArray[i]})
        }
     //   theView.model = root.myArray
    }

    function getJobName(i) {
        return root.myArray[i];
    }

    ListModel {
        id: jobsModel
    }

    Component {
        id: itemDelegate
        //HifiControls.Label { text: "I am item number: " + index }
        HifiControls.CheckBox { text: name + index;
         checked: true; }
    }

    ListView {
        id: theView
        anchors.fill: parent
        model: jobsModel
        delegate: itemDelegate
    }

}