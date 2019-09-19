//
//  ResourceCacheInspector.qml
//
//  Created by Sam Gateau on 2019-09-17
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.12
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
            theList = ["ResourceCacheInspector.cache is undefined"];
        }
        var theListString = new Array(theList.length)
        for (var i in theList) {
            theListString[i] = (theList[i].toString())
        }
        return theListString;
    }

    function resetItemListFromCache() {  
        resetItemList(fetchItemsList())
    }

    property var needFreshList : false

    function updateItemListFromCache() { 
        needFreshList = true
    }

    Timer {
        interval: 1000; running: true; repeat: true
        onTriggered: pullFreshValues()
    }

    function pullFreshValues() {
        if (needFreshList) {
            console.log("Updating " + cacheResourceName + "cache list")
            updateItemList(fetchItemsList())
            needFreshList = false
        }
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
                onSourceValueVarChanged: { /*console.log( root.cacheResourceName + " NumResource Value Changed!!!!") ;*/updateItemListFromCache() }
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
                onSourceValueVarChanged: { /*console.log( root.cacheResourceName + " NumCached Value Changed!!!!");*/updateItemListFromCache() }
            }
            height: totalCount.height
        }
    }


    ListModel {
        id: resourceItemsModel
    }
    property var currentItemsList: new Array();
  
    function packItemEntry(item) {
        var entry = { "name": "", "root": "", "path": "", "base": "", "url": item}
        if (item.length > 0) {
            var rootPos = item.search("://")
            entry.root = item.substring(0, rootPos)
            if (rootPos >= 0) rootPos += 3
            entry.path = item.substring(rootPos, item.length)     
            var splitted = entry.path.split('/')
            entry.name = splitted[splitted.length - 1] 
            entry.base = splitted[0] 
        }
        return entry
    }

    function resetItemList(itemList) {
        currentItemsList = []
        resourceItemsModel.clear()
        for (var i in itemList) {
            var item = itemList[i]
            currentItemsList.push(item)
            resourceItemsModel.append(packItemEntry(item))
        }   
    }

    function updateItemList(newItemList) {
        resetItemList(newItemList)
/*
        var nextListLength = (currentItemList.length < newItemList.length ? newItemList.length : currentItemList.length )
        var nextList = new Array(nextListLength)

        var addedList = []
        var removedList = []
        var movedList = []

        for (var i in currentItemList) {
            var item = currentItemList[i]
            var foundPos = newItemList.findIndex(item)
            if (foundPos == i) {
                newList[i] = item
                newItemList[i] = 0
            } else if (foundPos == -1) {
                removedList.push(i)
            } else {
                movedList.push([i,foundPos])
            }
        }

        for (var i in newItemList) {
            var item = newItemList[i]
            if (item != 0) {
                var foundPos = currentItemList.findIndex(item)
                if (foundPos == -1) {
                    addedList.push(item)
                }
            }
        }


        for (var i in itemList) {
            newList[i] = itemList[i]
        }




*/
        /*currentItemsList.clear()
        resourceItemsModel.clear()
        for (var i in itemList) {
            currentItemsList.append(itemList[i].toString())
            resourceItemsModel.append(packItemEntry(currentItemsList[i]))
        } */   
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
                text: model.root
                width: 30
            }
            Prop.PropSplitter {
                size:8
            }
            Prop.PropLabel {
                text: model.base
                width: 60
            }
            Prop.PropSplitter {
                size:8
            }
            Prop.PropLabel {
                text: model.name
            }

        }
    }

    ListView {
        anchors.top: header.bottom 
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        clip: true
    
        id: listView
        model: resourceItemsModel
        delegate: resouceItemDelegate
    }
}
