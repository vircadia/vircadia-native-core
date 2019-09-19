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
    property string cacheResourceName: "" 

    function fromScript(message) {
        switch (message.method) {
        case "resetItemList":
            resetItemListFromCache()
            break;
        }
    }

    Component.onCompleted: {
        resetItemListFromCache();
    }

    function fetchItemsList() {
        var theList;
        if (cache !== undefined) {
            theList = cache.getResourceList();
        } else {
            theList = [{"name": "ResourceCacheInspector.cache is undefined"}];
        }
        return theList;
    }

    function resetItemListFromCache() {  
        resourceItemsModel.resetItemList(fetchItemsList())
    }
    function updateItemListFromCache() {  
        resourceItemsModel.updateItemList(fetchItemsList())
    }

    Column {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right


        /*Prop.PropButton {
            anchors.left: parent.left
            id: refresh
                text: "Refresh"
            onClicked: {
                resetItemListFromCache()
            }
        }*/
        Item {
            anchors.left: parent.left
            anchors.right: parent.right

            Prop.PropScalar {
                id: totalCount
                anchors.left: parent.left
                anchors.right: parent.horizontalCenter
                label: "Count"
                object: root.cache
                property: "numTotal" 
                integral: true
                readOnly: true
                onSourceValueVarChanged: { console.log( root.cacheResourceName + " NumResource Value Changed!!!!") ;updateItemListFromCache() }
            }
            Prop.PropScalar {
                id: cachedCount
                anchors.left: parent.horizontalCenter
                anchors.right: parent.right
                label: "Cached"
                object: root.cache
                property: "numCached" 
                integral: true
                readOnly: true
                onSourceValueVarChanged: { console.log( root.cacheResourceName + " NumResource Value Changed!!!!") ;updateItemListFromCache() }
            }
            height: totalCount.height
        }
    }


    ListModel {
        id: resourceItemsModel

        function packItemEntry(item) {
            var some_uri = Qt.resolvedUrl(item)

            return { "name": item, "url": some_uri}
        }
 
        function resetItemList(itemList) {
            resourceItemsModel.clear()
            for (var i in itemList) {
                resourceItemsModel.append(packItemEntry(itemList[i]))
            }   
        }

        function updateItemList(itemList) {
            resourceItemsModel.clear()
            for (var i in itemList) {
                resourceItemsModel.append(packItemEntry(itemList[i]))
            }   
        }
    }

    Component {
        id: resouceItemDelegate
        Row {
            id: itemRow
            Prop.PropText {
                text: model.index
                width: 30
            }
            Prop.PropSplitter {
                size:8
            }
            Prop.PropLabel {
                text: JSON.stringify(model.url)
            }
           /* Prop.PropLabel {
                text: model.url
            }*/
        }
    }

    ScrollView {
        anchors.top: header.bottom 
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        clip: true
        ListView {
            id: listView
            model: resourceItemsModel
            delegate: resouceItemDelegate
        }
    }
}
