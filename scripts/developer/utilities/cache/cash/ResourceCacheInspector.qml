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
import QtQml.Models 2.12

import "../../lib/prop" as Prop

Item {
    id: root;
    Prop.Global { id: global }

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
        interval: 2000; running: true; repeat: true
        onTriggered: pullFreshValues()
    }

    function pullFreshValues() {
        if (needFreshList) {
            console.log("Updating " + cacheResourceName + "cache list")
            updateItemList(fetchItemsList())
            needFreshList = false
        }
    }
    
    property alias resourceItemsModel: visualModel.model  
    property var currentItemsList: new Array();
  
    function packItemEntry(item, index) {
        var entry = { "index": index, "name": "", "scheme": "", "host": "", "pathDir": "", "url": item}
        if (item.length > 0) {
            // Detect scheme:
            var schemePos = item.search(":") 
            entry.scheme = item.substring(0, schemePos)
            if (schemePos < 0) schemePos = 0
            else schemePos += 1

            // path pos is probably after schemePos
            var pathPos = schemePos
            
            // try to detect //userinfo@host:port
            var token = item.substr(schemePos, 2);
            if (token.search("//") == 0) {
                pathPos += 2
            }
            item = item.substring(pathPos, item.length)
            // item is now everything after scheme:[//]
            var splitted = item.split('/')

            // odd ball, the rest of the url has no other'/' ?
            // in theory this means it s just the host info ?
            // we are assuming that path ALWAYS starts with a slash
            entry.host = splitted[0]
            
            if (splitted.length > 1) {           
                entry.name = splitted[splitted.length - 1] 

                // if splitted is longer than 2 then there should be a path dir
                if (splitted.length > 2) {  
                    for (var i = 1; i < splitted.length - 1; i++) {
                        entry.pathDir += '/' +   splitted[i]   
                    }
                }
            }
        }
        return entry
    }


    function resetItemList(itemList) {
        currentItemsList = []
        resourceItemsModel.clear()
        for (var i in itemList) {
            var item = itemList[i]
            currentItemsList.push(item)
            resourceItemsModel.append(packItemEntry(item, currentItemsList.length -1))
        }   
    }

    function updateItemList(newItemList) {
        resetItemList(newItemList) 
    }

    property var itemFields: ['index', 'name', 'scheme', 'host', 'pathDir', 'url']
 

    Column {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        Item {
            anchors.left: parent.left
            anchors.right: parent.right
            height: totalCount.height

            Prop.PropScalar {
                id: totalCount
                anchors.left: parent.left
                anchors.right: parent.horizontalCenter
                label: "Count"
                object: root.cache
                property: "numTotal" 
                integral: true
                readOnly: true
                onSourceValueVarChanged: { updateItemListFromCache() }
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
                onSourceValueVarChanged: { updateItemListFromCache() }
            }
        }

        Item {
            anchors.left: parent.left
            anchors.right: parent.right
            height: orderSelector.height
            
            Prop.PropComboBox {
                anchors.left: parent.left
                id: orderSelector
                model: itemFields
                currentIndex: 1
                property var selectedIndex: currentIndex

                property var isSchemeVisible: (currentIndex == 2)
                property var isHostVisible: (currentIndex == 3)
                property var isPathDirVisible: (currentIndex == 4)
                property var isURLVisible: (currentIndex == 5)
            }

            Prop.PropCheckBox {
                anchors.left: orderSelector.right
                id: listQRC
                checked: false
                text: "list qrc"
            }

            TextField {
                anchors.left: listQRC.right
                id: nameFilter
                placeholderText: qsTr("Search by name...")
                Layout.fillWidth: true
            }

        }
    }

    Component {
        id: resouceItemDelegate
        MouseArea {
            id: dragArea
            property bool held: false
            anchors { left: parent.left; right: parent.right }
            height: item.height
            onPressed: {held = true}
            onReleased: {held = false}

            Rectangle {
                id: item
                width: parent.width
                height: global.slimHeight
                color: dragArea.held ? global.colorBackHighlight : (model.index % 2 ? global.colorBackShadow : global.colorBack)              
                Row {
                    id: itemRow
                    anchors.verticalCenter : parent.verticalCenter
                    Prop.PropText {
                        id: itemIndex
                        text: model.index
                        width: 30
                    }
                    Prop.PropSplitter {
                        visible: orderSelector.isSchemeVisible
                        size:8
                    }
                    Prop.PropLabel {
                        visible: orderSelector.isSchemeVisible
                        text: model.scheme
                        width: 30
                    }
                    Prop.PropSplitter {
                        visible: orderSelector.isHostVisible
                        size:8
                    }
                    Prop.PropLabel {
                        visible: orderSelector.isHostVisible
                        text: model.host
                        width: 150
                    }
                    Prop.PropSplitter {
                        visible: orderSelector.isPathDirVisible
                        size:8
                    }
                    Prop.PropLabel {
                        visible: orderSelector.isPathDirVisible
                        text: model.pathDir
                    }
                    Prop.PropSplitter {
                        size:8
                    }
                    Prop.PropLabel {
                        visible: !orderSelector.isURLVisible
                        text: model.name
                    }
                    Prop.PropLabel {
                        visible: orderSelector.isURLVisible
                        text: model.url
                    }
                }
            }
            
        }
    }

    SortFilterModel {
        id: visualModel
        model: ListModel {}

        property int sortOrder: orderSelector.selectedIndex

        property var lessThanArray: [
            function(left, right) { return left.index < right.index },
            function(left, right) { return left.name < right.name },
            function(left, right) { return left.scheme < right.scheme },
            function(left, right) { return left.host < right.host },
            function(left, right) { return left.pathDir < right.pathDir },
            function(left, right) { return left.url < right.url }
        ];
        lessThan: lessThanArray[sortOrder]

        property int listQRCChecked: listQRC.checked
        property int textFilter: nameFilter.text
        
        property var acceptItemArray: [
            function(item) { return item.scheme != "qrc" },
           // function(item) { return true }
            function(item) { return (item.name.search(textFilter) >= 0) }
        ]
        acceptItem: acceptItemArray[0 + listQRCChecked]


        delegate: resouceItemDelegate
    }

    ListView {
        anchors.top: header.bottom 
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        clip: true
    
        id: listView
        model: visualModel
    }
}
