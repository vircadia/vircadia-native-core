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

import "../../lib/skit/qml" as Skit
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

    function requestResourceDetails(resourceURL) {
        sendToScript({method: "inspectResource", params: {url: resourceURL, semantic: cacheResourceName}}); 
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
            updateItemList(fetchItemsList())
            needFreshList = false
        }
    }
    
    property alias resourceItemsModel: visualModel.model  
    property var currentItemsList: new Array();
  
    function packItemEntry(item, identifier) {
        var entry = { "identifier": identifier, "name": "", "scheme": "", "host": "", "pathDir": "", "url": item}
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
        // At the end of it, force an update
        visualModel.forceUpdate()   
    }

    function updateItemList(newItemList) {
        resetItemList(newItemList) 
    }

    property var itemFields: ['identifier', 'name', 'scheme', 'host', 'pathDir', 'url']
 

    Column {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        
        Item {
            anchors.left: parent.left
            anchors.right: parent.right
            height: totalCount.height
            id: headerTop

            Prop.PropButton {
                id: refreshButton
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                text: "Refresh"
                color: needFreshList ? global.colorOrangeAccent : global.fontColor 
                onPressed: { pullFreshValues() }
            }

            GridLayout {
                anchors.left: refreshButton.right
                anchors.right: parent.right
                id: headerCountLane
                columns: 3

                Item {
                    Layout.fillWidth: true
                    Prop.PropScalar {
                        id: itemCount
                        label: "Count"
                        object: root.cache
                        property: "numTotal" 
                        integral: true
                        readOnly: true
                    }
                }
                Item {
                    Layout.fillWidth: true
                    Prop.PropScalar {
                        id: totalCount
                        label: "Count"
                        object: root.cache
                        property: "numTotal" 
                        integral: true
                        readOnly: true
                        onSourceValueVarChanged: { updateItemListFromCache() }
                    }
                }
                Item {
                    Layout.fillWidth: true
                    Prop.PropScalar {
                        id: cachedCount
                        label: "Cached"
                        object: root.cache
                        property: "numCached" 
                        integral: true
                        readOnly: true
                        onSourceValueVarChanged: { updateItemListFromCache() }
                    }
                }
            }
        }
        Item {
            anchors.left: parent.left
            anchors.right: parent.right
            height: orderSelector.height
            
            Prop.PropText {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                width: 50
                id: orderSelectorLabel
                text: "Sort by"
                horizontalAlignment: Text.AlignHCenter
            }  
            Prop.PropComboBox {
                anchors.left: orderSelectorLabel.right
                width: 80
                id: orderSelector
                model: itemFields
                currentIndex: 1
                
                property var isSchemeVisible: (currentIndex == 2 || filterFieldSelector.currentIndex == 2)
                property var isHostVisible: (currentIndex == 3 || filterFieldSelector.currentIndex == 3)
                property var isPathDirVisible: (currentIndex == 4 || filterFieldSelector.currentIndex == 4)
                property var isURLVisible: (currentIndex == 5 || filterFieldSelector.currentIndex == 5)
            }

            Prop.PropTextField {
                anchors.left: orderSelector.right
                anchors.right: filterFieldSelector.left
                id: nameFilter
                placeholderText: qsTr("Filter by " + itemFields[filterFieldSelector.currentIndex] + "...")
                Layout.fillWidth: true
            }
            Prop.PropComboBox {
                anchors.right: parent.right
                id: filterFieldSelector
                model: itemFields
                currentIndex: 1
                width: 80
                opacity: (nameFilter.text.length > 0) ? 1.0 : 0.5
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
            onDoubleClicked: { requestResourceDetails(model.url) }
            Rectangle {
                id: item
                width: parent.width
                height: global.slimHeight
                color: dragArea.held ? global.colorBackHighlight : (index % 2 ? global.colorBackShadow : global.colorBack)              
                Row {
                    id: itemRow
                    anchors.verticalCenter : parent.verticalCenter
                    Prop.PropText {
                        id: itemIdentifier
                        text: model.identifier
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

    Skit.SortFilterModel {
        id: visualModel
        model: ListModel {}

        property int sortOrder: orderSelector.currentIndex

        property var lessThanArray: [
            function(left, right) { return left.index < right.index },
            function(left, right) { return left.name < right.name },
            function(left, right) { return left.scheme < right.scheme },
            function(left, right) { return left.host < right.host },
            function(left, right) { return left.pathDir < right.pathDir },
            function(left, right) { return left.url < right.url }
        ];
        lessThan: lessThanArray[sortOrder]

        property int filterField: filterFieldSelector.currentIndex
        onFilterFieldChanged: { refreshFilter() }
        property var textFilter: nameFilter.text
        onTextFilterChanged: { refreshFilter() }
        
        function filterToken(itemWord, token) { return  (itemWord.search(token) > -1) }
        property var acceptItemArray: [
            function(item) { return true },
            function(item) { return filterToken(item.identifier.toString(), textFilter) },
            function(item) { return filterToken(item.name, textFilter) },
            function(item) { return filterToken(item.scheme, textFilter) },
            function(item) { return filterToken(item.host, textFilter) },
            function(item) { return filterToken(item.pathDir, textFilter) },
            function(item) { return filterToken(item.url, textFilter) }
        ]

        function refreshFilter() {
            //console.log("refreshFilter! token = " + textFilter + " field = " + filterField)
            acceptItem = acceptItemArray[(textFilter.length != 0) * + (1 + filterField)]
        }

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
