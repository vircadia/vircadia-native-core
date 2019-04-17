//
//  jet/TaskPropView.qml
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

import stylesUit 1.0
import controlsUit 1.0 as HifiControls

import "../../prop" as Prop

import "../jet.js" as Jet

Prop.PropGroup {
    
    id: root;
    
    property var rootConfig : Render
    property var jobPath: ""
    property alias label: root.label
    property alias indentDepth: root.indentDepth

    property var showProps: true
    property var showSubs: true
    property var jobEnabled: true

    // Panel Header Data Component
    panelHeaderData: Component {
        Item {
            id: header
            Prop.PropLabel {
                text: root.label
                horizontalAlignment: Text.AlignHCenter
                anchors.left: parent.left
                anchors.right: enabledIcon.left
                anchors.verticalCenter: parent.verticalCenter
            }
            Prop.PropCanvasIcon {
                id: enabledIcon
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                fillColor: global.colorOrangeAccent 
                filled: jobEnabled

                MouseArea{
                    id: mousearea
                    anchors.fill: parent
                    onClicked: {
                        root.jobEnabled = !root.jobEnabled
                    }
                }
            }
        }
    }

    function populatePropItems() {
        var propsModel = []
        var props = Jet.job_propKeys(rootConfig.getConfig(jobPath));
      //  console.log(JSON.stringify(props));
        if (showProps) {
            for (var p in props) {
                propsModel.push({"object": rootConfig.getConfig(jobPath), "property":props[p] })
            }
            root.updatePropItems(propsModel);
        }
        
        if (showSubs) {
            Jet.task_traverse(rootConfig.getConfig(jobPath),
                function(job, depth, index) {
                    var component = Qt.createComponent("./TaskPropView.qml");
                    component.createObject(root.propItemsPanel, {
                        "label": job.objectName,
                        "rootConfig": root.rootConfig,
                        "jobPath": root.jobPath + '.' + job.objectName,
                        "showProps": root.showProps,
                        "showSubs": root.showSubs,
                        "indentDepth": root.indentDepth + 1,
                    })
                    /*  var component = Qt.createComponent("../../prop/PropItem.qml");
                    component.createObject(root.propItemsPanel, {
                        "label": root.jobPath + '.' + job.objectName + ' num=' + index,
                    })*/
                    //  propsModel.push({"type": "printLabel", "label": root.jobPath + '.' + job.objectName + ' num=' + index })

                        return (depth < 1);
                }, 0)      
        }
    }

    Component.onCompleted: {
        populatePropItems()
    }
        
 
}