//
//  ResourceCacheInspector.qml
//
//  Created by Sam Gateau on 2019-09-17
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.7
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3

import "../../lib/prop" as Prop

Item {
    id: root;
    anchors.fill: parent.fill
    property var cache: {}

    function fromScript(message) {
        switch (message.method) {
        case "setJSON":
           // jsonText.items = message.params.items;
            break;
        case "setItemList":   
            //console.log(message.params.items)
            resouceItemsModel.resetItemList(message.params.items)
            break;
        case "resetItemList":
            resetItemList()
            break;
        }
    }

    Component.onCompleted: {
       // if (cache !== undefined) {
            resetItemList();
       // }
    }

    function resetItemList() {
        var theList = cache.getResourceList();
        resouceItemsModel.resetItemList(theList)
    }

    ListModel {
        id: resouceItemsModel

        function resetItemList(itemList) {
            resouceItemsModel.clear()
            for (var i in itemList) {
                //resouceItemsModel.append({ "name": itemList[i]})
                resouceItemsModel.append({ "name": itemList[i].toString()})
            }   
        }
    }

    Component {
        id: resouceItemDelegate
        Row {
            id: itemRow
            Prop.PropLabel {
                text: model.name
            }
        }
    }


    Item {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right


            Prop.PropButton {
                
                anchors.left: parent.left
                id: refresh
                 text: "Refresh"
                onClicked: {
                    resetItemList()
                }
            }

            Prop.PropScalar {
                id: totalCount
                anchors.right: parent.right
                label: "Count" 
                sourceValueVar: resouceItemsModel.count
                integral: true
                readOnly: true
            }
            
            Prop.PropScalar {
                id: totalCount
                anchors.right: parent.right
                label: "Count" 
                sourceValueVar: resouceItemsModel.count
                integral: true
                readOnly: true
            }

        height: refresh.height
    }


    ScrollView {
        anchors.top: header.bottom 
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        clip: true
        ListView {
            id: listView
            model: resouceItemsModel
            delegate: resouceItemDelegate
        }
    }
}
