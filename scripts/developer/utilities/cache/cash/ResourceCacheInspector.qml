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
   
    property var itemFields: ['name', 'root', 'base', 'path', 'index']
    property var itemFieldsVisibility: {'name': true, 'root': false, 'base':false, 'path':false, 'index':false}


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
        }

        RowLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            height: orderSelector.height
            
            Prop.PropComboBox {
                anchors.left: parent.left
                id: orderSelector
                model: [ "name", "base", "root" ]
                property var selectedIndex: currentIndex
            }

            Prop.PropCheckBox {
                id: showName
                text: 'name'
                checked: itemFieldsVisibility['name']
            }
            Prop.PropCheckBox {
                id: showRoot
                text: 'root'
                checked: itemFieldsVisibility['root']
            }
            Prop.PropCheckBox {
                id: showBase
                text: 'base'
                checked: itemFieldsVisibility['base']
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
                        visible: showRoot.checked
                        size:8
                    }
                    Prop.PropLabel {
                        visible: showRoot.checked
                        text: model.root
                        width: 30
                    }
                    Prop.PropSplitter {
                        visible: showBase.checked
                        size:8
                    }
                    Prop.PropLabel {
                        visible: showBase.checked
                        text: model.base
                        width: 60
                    }
                    Prop.PropSplitter {
                        visible: showName.checked
                        size:8
                    }
                    Prop.PropLabel {
                        visible: showName.checked
                        text: model.name
                    }
                }
            }
            
        }
    }
    DelegateModel {
        id: visualModel

        model: ListModel {}

        property var lessThan: [
            function(left, right) { return left.name < right.name },
            function(left, right) { return left.base < right.base },
            function(left, right) { return left.root < right.root }
        ]

        property int sortOrder: orderSelector.selectedIndex
        onSortOrderChanged: items.setGroups(0, items.count, "unsorted")

        function insertPosition(lessThan, item) {
            var lower = 0
            var upper = items.count
            while (lower < upper) {
                var middle = Math.floor(lower + (upper - lower) / 2)
                var result = lessThan(item.model, items.get(middle).model);
                if (result) {
                    upper = middle
                } else {
                    lower = middle + 1
                }
            }
            return lower
        }

        function sort(lessThan) {
            while (unsortedItems.count > 0) {
                var item = unsortedItems.get(0)
                var index = insertPosition(lessThan, item)

                item.groups = "items"
                items.move(item.itemsIndex, index)
            }
        }

        items.includeByDefault: false
        groups: DelegateModelGroup {
            id: unsortedItems
            name: "unsorted"

            includeByDefault: true
            onChanged: {
                if (visualModel.sortOrder == visualModel.lessThan.length)
                    setGroups(0, count, "items")
                else
                    visualModel.sort(visualModel.lessThan[visualModel.sortOrder])
            }
        }


        delegate: resouceItemDelegate
    }
    property alias resourceItemsModel: visualModel.model  
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
