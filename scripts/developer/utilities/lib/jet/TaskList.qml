//
//  jet/TaskList.qml
//
//  Created by Sam Gateau, 2018/03/28
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

//import QtQuick 2.7
//import QtQuick.Controls 1.4 as Original
//import QtQuick.Controls.Styles 1.4

import QtQuick 2.5
import QtQuick.Controls 1.4
//import Hifi 1.0 as Hifi

//import "qrc:///qml/styles-uit"
//import "qrc:///qml/controls-uit" as HifiControls

import "jet_qml.js" as Jet

Rectangle {
    id: root
    width: parent ? parent.width : 100
    height: parent ? parent.height : 100
    property var config

    property var renderConfig : Render

    TextArea {
        id: textArea
        width: parent.width
        height: parent.height
        text: ""
    }

    Component.onCompleted: {
        // Connect the signal from Selection when any selection content change and use it to refresh the current selection view
       // Selection.selectedItemsListChanged.connect(resetSelectionView)
        var message = "On Completed: \n"
       var functor = Jet.job_print_functor(function (line) { message += line + "\n"; });
       // Jet.task_traverseTree(Render, functor);
        var lroot = Workload;
        functor(lroot,0,0)
      //  message += Workload["getSubConfigs"]() + '\n'
        
        //var subs = Workload;
        message += " subs size =  " + lroot.getNumSubs()
        for (var i = 0; i < lroot.getNumSubs(); i++) {
            if (functor(lroot.getSubConfig(i), depth, i)) {
             //   task_traverse(subs[i], functor, depth)
            }
        }
        textArea.append(message);
     }
    function fromScript(mope) {
        //print(message)
        //var message = mope + '\n';
        var message ='\n';
        
      //  Jet.task_traverseTree(Render, Jet.job_print_functor(function (line) { message += line + "\n"; }) );
        /*

        Render.findChildren();*/

       // message += (Render.getSubConfigs())
       // Render.getConfig("").findChildren();
/**//*
       var job = Render;
       message +=(job.objectName + " " + (job.enabled ? "on" : "off")) + '\n'; 
        var keys = Jet.job_propKeys(job);
        for (var p=0; p < keys.length;p++) {
            var prop = job[keys[p]]
            message += (typeof prop + " " + keys[p] + " " + prop) + '\n';
        }*/
/*
function task_traverse(root, functor, depth) {
    if (root.isTask()) { 
        var subs = root.getSubConfigs()
        depth++;
        for (var i = 0; i <subs.length; i++) {
            if (functor(subs[i], depth, i)) {
                task_traverse(subs[i], functor, depth)
            }
        }
    }    
}*/
     /*   var functor = Jet.job_print_functor(function (line) { message += line + "\n"; });
       // Jet.task_traverseTree(Render, functor);
        var lroot = Jet.getRender().getConfig("UpdateScene");
        functor(lroot,0,0)
        var subs = lroot.getSubConfigs()
        message += " subs size =  " + subs.length
        for (var i = 0; i <subs.length; i++) {
            if (functor(subs[i], depth, i)) {
             //   task_traverse(subs[i], functor, depth)
            }
        }
       // message += Render.getSubConfigs()
   /*    var subs = root.getSubConfigs()
        depth++;
        for (var i = 0; i <subs.length; i++) {
            if (functor(subs[i], depth, i)) {
                task_traverse(subs[i], functor, depth)
            }
        }
   // }    
}*/     
        /*var depth = 0;
        if (functor(lroot, 0, 0)) {
            var subs = lroot.getSubConfigs()
            depth++;
            for (var i = 0; i <subs.length; i++) {
              //  message += JSON.stringify(subs[i]);
         
                if (functor(subs[i], depth, i)) {
                   // task_traverse(subs[i], functor, depth)
                }
            }
        } */
     //   Jet.task_traverseTree(Render, functor)

     //   message = genTree();
       // print(message);
    }

    function clearWindow() {
        textArea.remove(0,textArea.length);
    }
}
/*
Item {
    HifiConstants { id: hifi;}
    id: render;   
    anchors.margins: hifi.dimensions.contentMargin.x
    
    color: hifi.colors.baseGray;   
    Column {
        spacing: 5
        anchors.left: parent.left
        anchors.right: parent.right       
        anchors.margins: hifi.dimensions.contentMargin.x  
        //padding: hifi.dimensions.contentMargin.x
        HifiControls.Label {
                    text: "Shading"       
        }
    }
}*/