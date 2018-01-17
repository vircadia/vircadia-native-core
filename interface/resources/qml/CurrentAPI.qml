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
import "styles-uit"
import "controls-uit" as HifiControls

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
        color: "white"
        width: parent.width
        height: parent.height
    }
    
    FontLoader { id: ralewayRegular; source: pathToFonts + "fonts/Raleway-Regular.ttf"; }

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
        anchors.leftMargin: 8
        width: parent.width
        height: 50
        HifiControls.GlyphButton {
            id: search
            enabled: true
            glyph: hifi.glyphs.search
            color: hifi.colors.text
            size: 48
            width: 50
            height: 50
            onClicked: {
                addListElements(searchBar.text);
                focus = true;
            } 
        }

        HifiControls.GlyphButton {
            id: back;
            enabled: true;
            glyph: hifi.glyphs.backward
            color: hifi.colors.text
            size: 48
            width: 30
            height: 50
            anchors.margins: 2
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

        TextField {
            id: searchBar            
            focus: true
            font.pixelSize: 16
            width: 2*(parent.width-back.width-search.width-reload.width-update.width-evaluate.width-addMember.width-16)/3
            height: parent.height
            font.family: ralewayRegular.name
            placeholderText: "Search"
            onAccepted: {
                console.log("Enter Pressed");
                search.clicked();
            }
            onActiveFocusChanged: {
                if (activeFocus && HMD.mounted) {
                    keyboard.raised = true;
                } else {
                    keyboard.raised = false;
                }
            }

        }        

        HifiControls.Button {
            id: addMember;
            enabled: true;
            text: "+"
            width: 50
            height: 50
            anchors.margins: 2
            onClicked: {
                addNewMember();
                updateList.start();
                focus = true;
            } 
        }

        HifiControls.Button {
            id: evaluate;
            enabled: true;
            text: "Eval"
            width: 50
            height: 50
            anchors.margins: 2
            onClicked: {
                evaluateMember();
                focus = true;
            } 
        }
        TextField {
            id: valueBar            
            focus: true
            font.pixelSize: 16
            width: (parent.width-back.width-search.width-reload.width-update.width-evaluate.width-addMember.width-16)/3
            height: parent.height
            font.family: ralewayRegular.name
            placeholderText: "Value"
            textColor: "#4466DD"
            anchors.margins: 2
        }

        HifiControls.GlyphButton {
            id: reload;
            enabled: false;
            glyph: hifi.glyphs.reload
            color: hifi.colors.text
            size: 48
            width: 50
            height: 50
            anchors.margins: 2
            onClicked: {
                reloadListValues();
                focus = true;
            } 
        }
        
        HifiControls.GlyphButton {
            id: update;
            enabled: false;
            glyph: hifi.glyphs.playback_play
            size: 48
            width: 50
            height: 50
            anchors.margins: 2
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
 
    ListModel {
        id: memberModel
    }

    Component {
        id: memberDelegate
        
        Row {
            id: memberRow
            property var isMainKey: apiType === "class";
            spacing: 10
            Rectangle {
                width: isMainKey ? 20 : 40;
                height: parent.height 
            }
            
            RalewayRegular { 
                text: apiMember
                size: !isMainKey ? 16 : 22
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
            

            RalewayRegular { 
                text: apiType
                size: 14
                color: hifi.colors.baseGrayHighlight
            }

            RalewayRegular { 
                text: !apiValue ? "" : apiValue;
                size: 16
                color: "#4466DD"
            }
        }
    }

   
    Rectangle {
        id: membersBackground
        anchors {
            left: parent.left; right: parent.right; top: topBar.bottom; bottom: parent.bottom;
            margins: hifi.dimensions.contentMargin.x
            bottomMargin: hifi.dimensions.contentSpacing.y + 40
        }
        color: "white"
        radius: 4

        ListView {
            id: list
            anchors {
                top: parent.top
                left: parent.left
                right: scrollBar.left
                bottom: parent.bottom
                margins: 4
            }
            clip: true
            cacheBuffer: 4000
            model: memberModel
            delegate: memberDelegate
            highlightMoveDuration: 0

            highlight: Rectangle {
                anchors {
                    left: parent ? parent.left : undefined
                    right: parent ? parent.right : undefined
                    leftMargin: hifi.dimensions.borderWidth
                    rightMargin: hifi.dimensions.borderWidth
                }
                color: "#BBDDFF"
            }
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
                topMargin: 4
                bottomMargin: 4
            }
            width: scrolling ? 18 : 0
            radius: 4
            color: hifi.colors.baseGrayShadow

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

                anchors {
                    right: parent.right
                    rightMargin: 3
                }
                width: 12
                height: (list.height / list.contentHeight) * list.height
                radius: width / 4
                color: "white"

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
    
    HifiControls.GlyphButton {
        id: clipboard;
        enabled: true;
        glyph: hifi.glyphs.scriptNew
        size: 38
        width: 50
        height: 50
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 2
        anchors.leftMargin: 8
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
    
    HifiControls.Button {
        id: debug;
        enabled: true;
        text: "Debug Script"
        width: 120
        height: 50
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 2
        anchors.rightMargin: 8
        onClicked: {
            sendToScript({type: "selectScript"});
        } 
    }

    HifiControls.CheckBox {
        id: hideQt
        boxSize: 25
        boxRadius: 3
        checked: true
        anchors.left: clipboard.right
        anchors.leftMargin: 8
        anchors.verticalCenter: clipboard.verticalCenter
        anchors.margins: 2
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