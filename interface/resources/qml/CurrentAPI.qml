//
//  ScriptAPI.qml
//
//  Created by Luis Cuenca on 12/18/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import stylesUit 1.0
import controlsUit 1.0 as HifiControls

Item {
    id: root
    width: parent.width
    height: parent.height

    property var hideQtMethods: true
    property var maxUpdateValues: 20
    property var maxReloadValues: 200
    property var apiMembers: []
    property var membersWithValues: []
    property var isReloading: false
    property var evaluatingIdx: -1
    property Component scrollSlider
    property Component keyboard
    
    Rectangle {
        color: hifi.colors.baseGray
        width: parent.width
        height: parent.height
    }

    Timer {
        id: updateList
        interval: 200
        repeat: false
        running: false
        onTriggered: {
            scrollSlider.y = 0;
            list.contentY = 0;
        }
    }

    Row {
        id: topBar
        anchors.left: parent.left
        anchors.leftMargin: 30
        anchors.top: parent.top
        anchors.topMargin: 30
        width: parent.width
        height: 40

        HifiControls.GlyphButton {
            id: back;
            enabled: true;
            color: hifi.buttons.black
            glyph: hifi.glyphs.backward
            size: 40
            width: 40
            height: 40
            anchors.left: search.right
            anchors.leftMargin: 12
            onClicked: {
                var text = searchBar.text;
                var chain = text.split(".");
                if (chain.length > 2) {
                    var result = chain[0]+".";
                    for (var i = 1; i < chain.length-2; i++) {
                        result += chain[i] + ".";
                    }
                    result += chain[chain.length-2];
                    searchBar.text = result;
                } else {
                    searchBar.text = (chain.length > 1) ? chain[0] : "";
                }
                if (chain.length > 1) {
                    addListElements(searchBar.text);
                } else {
                    addListElements();
                }
                focus = true;
            }
        }

        HifiControls.TextField {
            id: searchBar
            focus: true
            isSearchField: true
            width: parent.width - 112
            height: 40
            colorScheme: hifi.colorSchemes.dark
            anchors.left: back.right
            anchors.leftMargin: 10
            font.family: firaSansSemiBold.name
            placeholderText: "Search"
            onAccepted: {
                console.log("Enter Pressed");
                addListElements(searchBar.text);
            }
            onActiveFocusChanged: {
                if (activeFocus && HMD.mounted) {
                    keyboard.raised = true;
                } else {
                    keyboard.raised = false;
                }
            }

        }
    }

    Row {
        id: topBar2
        anchors.left: parent.left
        anchors.leftMargin: 30
        anchors.top: topBar.bottom
        anchors.topMargin: 30
        width: parent.width -60
        height: 40

        HifiControls.GlyphButton {
            id: addMember;
            enabled: true;
            color: hifi.buttons.black
            glyph: hifi.glyphs.maximize
            width: 40
            height: 40
            anchors.top: parent.top
            anchors.left: parent.left
            onClicked: {
                addNewMember();
                updateList.start();
                focus = true;
            } 
        }

        HifiControls.Button {
            id: evaluate;
            enabled: true;
            color: hifi.buttons.black
            text: "Eval"
            width: 40
            height: 40
            anchors.left: addMember.right
            anchors.leftMargin: 12
            onClicked: {
                evaluateMember();
                focus = true;
            } 
        }

        HifiControls.TextField {
            id: valueBar
            isSearchField: false
            font.pixelSize: 16
            width: parent.width - 208
            height: 40
            colorScheme: hifi.colorSchemes.dark
            font.family: firaSansSemiBold.name
            placeholderText: "Value"
            anchors.left: evaluate.right
            anchors.leftMargin: 12
            onActiveFocusChanged: {
                if (activeFocus && HMD.mounted) {
                    keyboard.raised = true;
                } else {
                    keyboard.raised = false;
                }
            }
        }

        HifiControls.GlyphButton {
            id: reload;
            enabled: false;
            color: hifi.buttons.black
            glyph: hifi.glyphs.reload
            size: 40
            width: 40
            height: 40
            anchors.right: update.left
            anchors.rightMargin: 12
            onClicked: {
                reloadListValues();
                focus = true;
            } 
        }
        
        HifiControls.GlyphButton {
            id: update;
            enabled: false;
            color: hifi.buttons.black
            glyph: hifi.glyphs.playback_play
            size: 40
            width: 40
            height: 40
            anchors.right: parent.right
            onClicked: {
                if (isReloading) {
                    update.glyph = hifi.glyphs.playback_play
                    isReloading = false;
                    stopReload();
                } else {
                    update.glyph = hifi.glyphs.stop_square
                    isReloading = true;
                    startReload();
                }
                focus = true;
            } 
        }
    }

    Rectangle {
        id: membersBackground
        anchors {
            left: parent.left; right: parent.right; top: topBar2.bottom; bottom: bottomBar.top;
            margins: 30
        }
        color: hifi.colors.tableBackgroundDark
        border.color: hifi.colors.lightGray
        border.width: 2
        radius: 5

        ListModel {
            id: memberModel
        }

        Component {
            id: memberDelegate
            Item {
                id: item
                width: parent.width
                anchors.left: parent.left
                height: 26
                clip: true

                Rectangle {
                    width: parent.width
                    height: parent.height
                    color: index % 2 == 0 ? hifi.colors.tableRowDarkEven : hifi.colors.tableRowDarkOdd
                    anchors.verticalCenter: parent.verticalCenter

                    Row {
                        id: memberRow
                        anchors.bottom: parent.bottom
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 10

                        FiraSansSemiBold {
                            property var isMainKey: apiType === "class";
                            text: apiMember
                            size: isMainKey ? 17 : 15
                            font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                            color: isMainKey ? hifi.colors.faintGray : hifi.colors.lightGrayText
                            MouseArea {
                                width: list.width
                                height: parent.height
                                onClicked: {
                                    searchBar.text = apiType=="function()" ? apiMember + "()" : apiMember;
                                    valueBar.text = !apiValue ? "" : apiValue;
                                    list.currentIndex = index;
                                    evaluatingIdx = index;
                                }
                                onDoubleClicked: {
                                    if (apiType === "class") {
                                        addListElements(apiMember+".");
                                    } else {
                                        isolateElement(evaluatingIdx);
                                    }
                                }
                            }
                        }

                        FiraSansRegular {
                            text: apiType
                            anchors.left: apiMember.right
                            anchors.verticalCenter: parent.verticalCenter
                            size: 13
                            color: hifi.colors.lightGrayText
                        }

                        FiraSansRegular {
                            text: !apiValue ? "" : apiValue;
                            anchors.left: apiType.right
                            anchors.verticalCenter: parent.verticalCenter
                            size: 14
                            color: hifi.colors.primaryHighlight
                        }
                    }
                }
            }
        }

        Component {
            id: highlight
            Rectangle {
                anchors {
                    left: list.left
                    right: scrollBar.left
                    leftMargin: 2
                    rightMargin: 2
                }
                color: hifi.colors.primaryHighlight
                radius: 4
                z: 10
                opacity: 0.5
            }
        }

        ListView {
            id: list
            anchors {
                top: parent.top
                left: parent.left
                right: scrollBar.left
                bottom: parent.bottom
                topMargin: 2
                leftMargin: 2
                bottomMargin: 2
            }
            clip: true
            cacheBuffer: 4000
            model: memberModel
            delegate: memberDelegate
            highlightMoveDuration: 0
            highlight: highlight
            onMovementStarted: {
                scrollSlider.manual = true;
            }
            onMovementEnded: {
                if (list.contentHeight > list.height) {
                    var range = list.contentY/(list.contentHeight-list.height);
                    range = range > 1 ? 1 : range;
                    var idx = Math.round((list.count-1)*range);
                    scrollSlider.positionSlider(idx);
                }
                scrollSlider.manual = false;
                returnToBounds()
            }
        }

        Rectangle {
            id: scrollBar

            property bool scrolling: list.contentHeight > list.height

            anchors {
                top: parent.top
                right: parent.right
                bottom: parent.bottom
                margins: 2
            }
            width: 22
            height: parent.height - 4
            color: hifi.colors.tableScrollBackgroundDark

            MouseArea {
                anchors.fill: parent

                onClicked: {
                    var index = scrollSlider.y * (list.count - 1) / (scrollBar.height - scrollSlider.height);
                    index = Math.round(index);
                    var scrollAmount = Math.round(list.count/10);
                    index = index + (mouse.y <= scrollSlider.y ? -scrollAmount : scrollAmount);
                    if (index < 0) {
                        index = 0;
                    }
                    if (index > list.count - 1) {
                        index = list.count - 1;
                    }
                    scrollSlider.positionSlider(index);
                }
            }

            Rectangle {
                id: scrollSlider

                property var manual: false

                function positionSlider(index){
                    y = index*(scrollBar.height - scrollSlider.height)/(list.count - 1);
                }

                anchors.right: parent.right
                anchors.margins: 2
                width: 18
                height: ((list.height / list.contentHeight) * list.height) < 15 ? 15 : (list.height / list.contentHeight) * list.height
                radius: 5
                color: hifi.colors.tableScrollHandleDark

                visible: scrollBar.scrolling;

                onYChanged: {
                    var index = y * (list.count - 1) / (scrollBar.height - scrollSlider.height);
                    index = Math.round(index);
                    if (!manual) {
                        list.positionViewAtIndex(index, ListView.Visible);
                    } 
                }

                MouseArea {
                    anchors.fill: parent
                    drag.target: scrollSlider
                    drag.axis: Drag.YAxis
                    drag.minimumY: 0
                    drag.maximumY: scrollBar.height - scrollSlider.height
                }
            }
        }
    }

    Row {
        id: bottomBar
        anchors.left: parent.left
        anchors.leftMargin: 30
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 30
        width: parent.width
        height: 40

        HifiControls.GlyphButton {
            id: clipboard;
            enabled: true;
            color: hifi.buttons.black
            glyph: hifi.glyphs.scriptNew
            size: 25
            width: 40
            height: 40
            anchors.left: parent.left
            onClicked: {
                var buffer = "";
                for (var i = 0; i < memberModel.count; i++) {
                    var datarow = memberModel.get(i);
                    buffer += "\n" + datarow.apiMember + "    " + datarow.apiType + "    " + datarow.apiValue;
                }
                Window.copyToClipboard(buffer);
                focus = true;
            } 
        }

        HifiControls.CheckBox {
            id: hideQt
            colorScheme: hifi.checkbox.dark
            boxSize: 25
            boxRadius: 3
            checked: true
            anchors.left: clipboard.right
            anchors.leftMargin: 10
            anchors.verticalCenter: clipboard.verticalCenter
            onClicked: {
                hideQtMethods = checked;
                addListElements();
            }
        }

        HifiControls.Label {
            id: hideLabel
            anchors.left: hideQt.right
            anchors.verticalCenter: clipboard.verticalCenter
            anchors.margins: 2
            font.pixelSize: 15
            text: "Hide Qt Methods"
        }
    
        HifiControls.Button {
            id: debug;
            enabled: true;
            color: hifi.buttons.black
            text: "Debug Script"
            width: 120
            height: 40
            anchors.right: parent.right
            anchors.rightMargin: 60
            anchors.bottom: parent.bottom
        
            onClicked: {
                sendToScript({type: "selectScript"});
            } 
        } 
    }

    HifiControls.Keyboard {
        id: keyboard;
        raised: false;
        anchors {
            bottom: parent.bottom;
            left: parent.left;
            right: parent.right;
        }

        Keys.onPressed: {
            console.log(event.nativeScanCode);
            if (event.key == Qt.Key_Left) {
                keyboard.raised = false;
            }
        }
    }

    function addNewMember() {
        apiMembers.push({member: searchBar.text, type: "user", value: valueBar.text, hasValue: true});
        var data = {'memberIndex': apiMembers.length-1, 'apiMember': searchBar.text, 'apiType':"user", 'apiValue': valueBar.text};
        memberModel.insert(0, data);
        computeMembersWithValues();
    }

    function evaluateMember() {
        sendToScript({type: "evaluateMember", data:{member: searchBar.text, index: evaluatingIdx}});
    }

    function getValuesToRefresh() {
        var valuesToRefresh = [];
        for (var i = 0; i < membersWithValues.length; i++) {
            var index = membersWithValues[i];
            var row = memberModel.get(index);
            valuesToRefresh.push({index: index, member: (row.apiType == "function()")  ? row.apiMember+"()" : row.apiMember, value: row.apiValue});
        }
        return valuesToRefresh;
    }


    function reloadListValues(){
        var valuesToRefresh = getValuesToRefresh();
        sendToScript({type: "refreshValues", data:valuesToRefresh});
    }

    function startReload(){
        var valuesToRefresh = getValuesToRefresh();
        sendToScript({type: "startRefreshValues", data:valuesToRefresh});
    }

    function stopReload(){
        sendToScript({type: "stopRefreshValues"});
    }

    function refreshValues(data) {
        var buffer = "";
        for (var i = 0; i < data.length; i++) {
            var row = memberModel.get(data[i].index);
            row.apiValue = data[i].value;
            apiMembers[row.memberIndex].value = data[i].value;
            memberModel.set(data[i].index, row);
            buffer += "\n" + apiMembers[row.memberIndex].member + " : " + data[i].value;
        }
        print(buffer);
    }
    

    function fromScript(message) {
        if (message.type === "methods") {
            apiMembers = message.data;
            if (ScriptDiscoveryService.debugScriptUrl != "") {
                addListElements("GlobalDebugger");
                if (memberModel.count == 0) {
                    addListElements();
                }
            } else {
                addListElements();
            }
            
        } else if (message.type === "debugMethods") {
            addListElements("GlobalDebugger");
        } else if (message.type === "refreshValues") {
            refreshValues(message.data);
        } else if (message.type === "evaluateMember") {
            valueBar.text = message.data.value;
            var selrow = memberModel.get(message.data.index);
            if (selrow.apiMember === searchBar.text || selrow.apiMember === searchBar.text + "()") {
                selrow.apiValue = message.data.value;
                apiMembers[selrow.memberIndex].value = message.data.value;
                apiMembers[selrow.memberIndex].hasValue = true;
                memberModel.set(message.data.index, selrow);
            }
        } else if (message.type === "selectScript") {
            if (message.data.length > 0) {
                ScriptDiscoveryService.debugScriptUrl = message.data;
            }
        }
    }


    function getFilterPairs(filter){
        var filteredArray = [];
        var filterChain;
        filterChain = filter.split(" ");
        for (var i = 0; i < filterChain.length; i++) {
            filterChain[i] = filterChain[i].toUpperCase();
        }
        var matchPairs = [];
        
        for (var i = 0; i < apiMembers.length; i++) {
            if (filterChain != undefined) {
                var found = 0;
                var memberComp = apiMembers[i].member.toUpperCase();
                for (var j = 0; j < filterChain.length; j++) {
                    found += memberComp.indexOf(filterChain[j]) >= 0 ? 1 : 0;
                }
                if (found === 0) {
                    continue;
                }
                matchPairs.push({index: i, found: found, member: apiMembers[i].member});
            }
        }
        
        matchPairs.sort(function(a, b){
            if(a.found > b.found) return -1;
		    if(a.found < b.found) return 1;
            if(a.member > b.member) return 1;
		    if(a.member < b.member) return -1;
		    return 0;
        });

        return matchPairs;
    }

    function isolateElement(index) {
        var oldElement = memberModel.get(index);
        var newElement = {memberIndex: oldElement.memberIndex, apiMember: oldElement.apiMember, apiType: oldElement.apiType, apiValue: oldElement.apiValue};
        membersWithValues = apiMembers[oldElement.memberIndex].hasValue ? [0] : [];
        memberModel.remove(0, memberModel.count);
        memberModel.append(newElement);
    }

    function computeMembersWithValues() {
        membersWithValues = [];
        for (var i = 0; i < memberModel.count; i++) {
            var idx = memberModel.get(i).memberIndex;
            if (apiMembers[idx].hasValue) {
                membersWithValues.push(i);
            }
        }
        update.enabled = membersWithValues.length <= maxUpdateValues;
        reload.enabled = membersWithValues.length <= maxReloadValues;
    }

    function addListElements(filter) {
        valueBar.text = "";
        memberModel.remove(0, memberModel.count);
        
        var filteredArray = (filter != undefined) ? [] : apiMembers;
        var matchPairs;
        if (filter != undefined) {
            matchPairs = getFilterPairs(filter);
            for (var i = 0; i < matchPairs.length; i++) {
                if (matchPairs[i].found < matchPairs[0].found) {
                    break;
                }
                var idx = matchPairs[i].index;
                filteredArray.push(apiMembers[idx]);
            }
        }

        for (var i = 0; i < filteredArray.length; i++) {
            var data = {'memberIndex':  matchPairs ? matchPairs[i].index : i, 
                        'apiMember': filteredArray[i].member, 
                        'apiType': filteredArray[i].type, 
                        'apiValue': filteredArray[i].value};

            if (hideQtMethods) {
                var chain = data.apiMember.split("."); 
                var method = chain[chain.length-1];
                if (method != "destroyed" &&
                    method != "objectName" &&
                    method != "objectNameChanged") {
                    memberModel.append(data);
                } 
            } else {
                memberModel.append(data);
            }
        }

        computeMembersWithValues();

        if (isReloading) {
            update.glyph = hifi.glyphs.playback_play
            isReloading = false;
            stopReload();
        }

        if (memberModel.count > 0) {
            scrollSlider.y = 0;
            list.contentY = 0;
        }
    }

    signal sendToScript(var message);    
}
