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

import "../../prop" as Prop

import "../jet.js" as Jet

Prop.PropGroup {
    
    id: root;
    
    Prop.Global { id: global;}
    
    property var rootConfig : Render
    property var jobPath: ""
    property alias label: root.label
    property alias indentDepth: root.indentDepth

    property var showProps: true
    property var showSubs: true
    property bool jobEnabled: rootConfig.getConfig(jobPath).enabled
    property var jobCpuTime: pullCpuTime()

    function pullCpuTime() {
        if (jobEnabled) {
            return rootConfig.getConfig(jobPath).cpuRunTime.toPrecision(3);
        } else {
            return '.'
        }
    }

    property var toggleJobActivation: function() {
        console.log("the button has been pressed and jobEnabled is " + jobEnabled )
        jobEnabled = !jobEnabled;
        rootConfig.getConfig(jobPath).enabled = jobEnabled;
    }

    // Panel Header Data Component
    panelHeaderData: Component {
        Item {
            id: header
            Prop.PropLabel {
                text: root.label
                horizontalAlignment: Text.AlignHCenter
                anchors.left: parent.left
                anchors.right: cpuTime.left
                anchors.verticalCenter: parent.verticalCenter
            }  
            Prop.PropLabel {
                id: cpuTime
                visible: false // root.jobEnabled 
                width: 50
                text: jobCpuTime
                horizontalAlignment: Text.AlignLeft
                anchors.rightMargin: 5
                anchors.right:enabledIcon.right
                anchors.verticalCenter: parent.verticalCenter
            }         
            Prop.PropCanvasIcon {
                id: enabledIcon
                anchors.right:parent.right
                anchors.verticalCenter: parent.verticalCenter
                filled: true
                fillColor: (root.jobEnabled ? global.colorGreenHighlight : global.colorRedAccent)
                icon: 5
                
                MouseArea{
                    id: mousearea
                    anchors.fill: parent
                    onClicked: { root.toggleJobActivation() }
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
                propsModel.push({"object": rootConfig.getConfig(jobPath), "property":props[p]})
            }
            root.updatePropItems(root.propItemsPanel, propsModel);
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