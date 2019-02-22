//
//  Marketplace.qml
//  qml/hifi/commerce/marketplace
//
//  Marketplace
//
//  Created by Roxanne Skelly on 2019-01-18
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.9
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit
import "../../../controls" as HifiControls
import "../common" as HifiCommerceCommon
import "qrc:////qml//hifi//models" as HifiModels  // Absolute path so the same code works everywhere.
import "../common/sendAsset"
import "../.." as HifiCommon

Rectangle {
    HifiConstants {
        id: hifi
    }

    id: root

    property string activeView: "initialize"
    property int currentSortIndex: 1
    property string sortString: "recent"
    property bool isAscending: false
    property string categoryString: ""
    property string searchString: ""
    property bool keyboardEnabled: HMD.active
    property bool keyboardRaised: false
    property string searchScopeString: "Featured"
    property bool isLoggedIn: false
    property bool supports3DHTML: true
    property bool pendingGetMarketplaceItemCall: false

    anchors.fill: (typeof parent === undefined) ? undefined : parent

    function getMarketplaceItems() {
        marketplaceItemView.visible = false;
        itemsList.visible = true;
        licenseInfo.visible = false;
        marketBrowseModel.getFirstPage();
        {
            if(root.searchString !== undefined && root.searchString !== "") {
                root.searchScopeString = "Search Results: \"" + root.searchString + "\"";
            } else if (root.categoryString !== "") {
                root.searchScopeString = root.categoryString;
            } else {
                root.searchScopeString = "Featured";
            }
        }
    }

    Component.onCompleted: {
        Commerce.getLoginStatus();
        
        supports3DHTML = PlatformInfo.has3DHTML();
    }

    Component.onDestruction: {
        keyboard.raised = false;
    }

    Connections {
        target: GlobalServices

        onMyUsernameChanged: {
            Commerce.getLoginStatus();
        }
    }
    
    Connections {
        target: MarketplaceScriptingInterface

        onGetMarketplaceCategoriesResult: {
            if (result.status !== 'success') {
                console.log("Failed to get Marketplace Categories", result.data.message);
            } else {
                categoriesModel.clear();
                categoriesModel.append({
                    id: -1,
                    name: "Everything"
                });
                result.data.items.forEach(function(category) {
                    categoriesModel.append({
                        id: category.id,
                        name: category.name
                    });
                });
            }
            getMarketplaceItems();
        }
        onGetMarketplaceItemsResult: {
            if (!pendingGetMarketplaceItemCall) {
                marketBrowseModel.handlePage(result.status !== "success" && result.message, result);
            }
        }
        
        onGetMarketplaceItemResult: {
            if (result.status !== 'success') {
                console.log("Failed to get Marketplace Item", result.data.message);
            } else {
                marketplaceItem.supports3DHTML = root.supports3DHTML;
                marketplaceItem.item_id = result.data.id;
                marketplaceItem.image_url = result.data.thumbnail_url;
                marketplaceItem.name = result.data.title;
                marketplaceItem.likes = result.data.likes;
                if (result.data.has_liked !== undefined) {
                    marketplaceItem.liked = result.data.has_liked;
                }
                marketplaceItem.creator = result.data.creator;
                marketplaceItem.categories = result.data.categories;
                marketplaceItem.price = result.data.cost;
                marketplaceItem.description = result.data.description;
                marketplaceItem.attributions = result.data.attributions;
                marketplaceItem.license = result.data.license;
                marketplaceItem.availability = result.data.availability;
                marketplaceItem.updated_item_id = result.data.updated_item_id || "";
                marketplaceItem.created_at = result.data.created_at;
                marketplaceItemScrollView.contentHeight = marketplaceItemContent.height;
                itemsList.visible = false;
                marketplaceItemView.visible = true;
                pendingGetMarketplaceItemCall = false;
            }
        }
    }

    Connections {
        target: Commerce

        onLoginStatusResult: {
            root.isLoggedIn = isLoggedIn;
        }
    }

    HifiCommerceCommon.CommerceLightbox {
        id: lightboxPopup
        visible: false
        anchors.fill: parent
    }

    //
    // HEADER BAR START
    //
    
    Rectangle {
        id: headerShadowBase
        anchors.fill: header
        color: "white"
    }
    DropShadow {
        anchors.fill: headerShadowBase
        source: headerShadowBase
        verticalOffset: 4
        horizontalOffset: 4
        radius: 6
        samples: 9
        color: hifi.colors.baseGrayShadow
        z:5
    }
    Rectangle {
        id: header;

        visible: true
        anchors {
            left: parent.left
            top: parent.top
            right: parent.right
        }

        height: childrenRect.height+5
        z: 5
    
        Rectangle {
            id: titleBarContainer

            anchors.left: parent.left
            anchors.top: parent.top
            width: parent.width
            height: 60
            visible: true

            Image {
                id: marketplaceHeaderImage;
                source: "../common/images/marketplaceHeaderImage.png";
                anchors.top: parent.top;
                anchors.topMargin: 2;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 0;
                anchors.left: parent.left;
                anchors.leftMargin: 8;
                width: 140;
                fillMode: Image.PreserveAspectFit;

                MouseArea {
                    anchors.fill: parent;
                    onClicked: {
                        sendToParent({method: "header_marketplaceImageClicked"});
                    }
                }
            }
        }

        Rectangle {
            id: searchBarContainer

            anchors.top: titleBarContainer.bottom
            width: parent.width
            height: 50
            
            visible: true
            clip: false

            //
            // TODO: Possibly change this to be a combo box
            //
            Rectangle {
                id: categoriesButton

                anchors {
                    left: parent.left
                    leftMargin: 10
                    verticalCenter: parent.verticalCenter
                }
                height: 34
                width: categoriesText.width + 25

                color: hifi.colors.white
                radius: 4
                border.width: 1
                border.color: hifi.colors.lightGrayText
                
                RalewayRegular {
                    id: categoriesText

                    anchors.centerIn: parent

                    text: "Categories"
                    size: 14
                    color: hifi.colors.lightGrayText
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    rightPadding: 10
                }

                HiFiGlyphs {
                    id: categoriesDropdownIcon

                    anchors {
                        right: parent.right
                        rightMargin: -8
                        verticalCenter: parent.verticalCenter
                    }

                    text: hifi.glyphs.caratDn
                    size: 34
                    horizontalAlignment: Text.AlignHCenter
                    color: hifi.colors.baseGray
                }

                MouseArea {
                    anchors.fill: parent

                    hoverEnabled: enabled
                    onClicked: {
                        categoriesDropdown.visible = !categoriesDropdown.visible;
                        categoriesButton.color = categoriesDropdown.visible ? hifi.colors.lightGray : hifi.colors.white;
                        categoriesDropdown.forceActiveFocus();
                    }
                    onEntered: categoriesText.color = hifi.colors.baseGrayShadow
                    onExited: categoriesText.color = hifi.colors.baseGray
                }

                Component.onCompleted: {
                    MarketplaceScriptingInterface.getMarketplaceCategories();
                }                
            }
                
            // or
            RalewayRegular {
                id: orText

                anchors.left: categoriesButton.right
                anchors.verticalCenter: parent.verticalCenter
                width: 25

                text: "or"
                size: 18;
                color: hifi.colors.baseGray
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                rightPadding: 10
                leftPadding: 10
            }

            HifiControlsUit.TextField {
                id: searchField

                anchors {
                    verticalCenter: parent.verticalCenter
                    right: parent.right
                    left: orText.right
                    rightMargin: 10
                }
                height: 34

                isSearchField: true
                colorScheme: hifi.colorSchemes.faintGray
                font.family: "Fira Sans"
                font.pixelSize: hifi.fontSizes.textFieldInput
                placeholderText: "Search Marketplace"

                Timer {
                    id: keypressTimer
                    running: false
                    repeat: false
                    interval: 300
                    onTriggered: searchField.accepted()

                }

                // workaround for https://bugreports.qt.io/browse/QTBUG-49297
                Keys.onPressed: {
                    switch (event.key) {
                        case Qt.Key_Return: 
                        case Qt.Key_Enter: 
                            event.accepted = true;
                            searchField.text = "";

                            getMarketplaceItems();
                            searchField.forceActiveFocus();
                        break;
                        default:
                            keypressTimer.restart();
                        break;
                    }
                }

                onAccepted: {
                    root.searchString = searchField.text;
                    getMarketplaceItems();
                    searchField.forceActiveFocus();
                }

                onActiveFocusChanged: {
                    root.keyboardRaised = activeFocus;
                }
 
            }
        }
    }
    //
    // HEADER BAR END
    //
    
    //
    // CATEGORIES LIST START
    //    
    Item {
        id: categoriesDropdown

        anchors.fill: parent;

        visible: false
        z: 10
        MouseArea {
            anchors.fill: parent
            propagateComposedEvents: true
            onClicked: {
                categoriesDropdown.visible = false;
                categoriesButton.color = hifi.colors.white;
            }            
        }

        Rectangle {
            anchors {
                left: parent.left;
                bottom: parent.bottom;
                top: parent.top;
                topMargin: 100;
            }
            width: parent.width/3

            color: hifi.colors.white
            
            ListModel {
                id: categoriesModel
            }

            ListView {
                id: categoriesListView;

                anchors.fill: parent
                anchors.rightMargin: 10
                width: parent.width
                currentIndex: -1;
                clip: true
                
                model: categoriesModel
                delegate: ItemDelegate {
                    height: 34
                    width: parent.width

                    clip: true
                    contentItem: Rectangle {
                        id: categoriesItem

                        anchors.fill: parent

                        color: hifi.colors.white
                        visible: true

                        RalewayRegular {
                            id: categoriesItemText

                            anchors.leftMargin: 15
                            anchors.fill:parent
 
                            text: model.name
                            color: ListView.isCurrentItem ? hifi.colors.lightBlueHighlight : hifi.colors.baseGray
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                            size: 14
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        z: 10

                        hoverEnabled: true
                        propagateComposedEvents: false

                        onEntered: {
                            categoriesItem.color = ListView.isCurrentItem ? hifi.colors.white : hifi.colors.lightBlueHighlight;
                        }

                        onExited: {
                            categoriesItem.color = ListView.isCurrentItem ? hifi.colors.lightBlueHighlight : hifi.colors.white;
                        }

                        onClicked: {
                            categoriesListView.currentIndex = index;
                            categoriesText.text = categoriesItemText.text;
                            categoriesDropdown.visible = false;
                            categoriesButton.color = hifi.colors.white;
                            root.categoryString = categoriesItemText.text;
                            getMarketplaceItems();
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar {
                    parent: categoriesListView.parent

                    anchors {
                        top: categoriesListView.top;
                        bottom: categoriesListView.bottom;
                        left: categoriesListView.right;
                    }

                    contentItem.opacity: 1
                }
            }
        }
    }
    //
    // CATEGORIES LIST END
    //
    
    //
    // ITEMS LIST START
    //
    Item {
        id: itemsList

        anchors {
            fill: parent
            topMargin: 115
            bottomMargin: 50
        }
 
        visible: true

        HifiModels.PSFListModel {
            id: marketBrowseModel

            itemsPerPage: 7
            listModelName: 'marketBrowse'
            listView: marketBrowseList

            getPage: function () {
                MarketplaceScriptingInterface.getMarketplaceItems(
                    root.searchString === "" ? undefined : root.searchString,
                    "",
                    root.categoryString.toLowerCase(),
                    "",
                    "",
                    root.sortString,
                    root.isAscending,
                    WalletScriptingInterface.limitedCommerce,
                    marketBrowseModel.currentPageToRetrieve,
                    marketBrowseModel.itemsPerPage
                );
            }

            processPage: function(data) {
                return data.items;
            }
        }
        ListView {
            id: marketBrowseList

            anchors.fill: parent
            anchors.rightMargin: 10

            model: marketBrowseModel

            orientation: ListView.Vertical
            focus: true
            clip: true
            
            delegate: MarketplaceListItem {
                item_id: model.id

                anchors.topMargin: 10

                image_url:model.thumbnail_url
                name: model.title
                likes: model.likes
                liked: model.has_liked ? model.has_liked : false
                creator: model.creator
                category: model.primary_category
                price: model.cost
                availability: model.availability
                isLoggedIn: root.isLoggedIn;

                onShowItem: {
                    MarketplaceScriptingInterface.getMarketplaceItem(item_id);
                }

                onBuy: {
                    sendToScript({method: 'marketplace_checkout', itemId: item_id});
                }

                onCategoryClicked: {
                    root.categoryString = category;
                    categoriesText.text = category;                    
                    getMarketplaceItems();
                }

                onRequestReload: getMarketplaceItems();
            }

            ScrollBar.vertical: ScrollBar {
                parent: marketBrowseList.parent

                anchors {
                    top: marketBrowseList.top
                    bottom: marketBrowseList.bottom
                    left: marketBrowseList.right
                }

                contentItem.opacity: 1
            }
            
            headerPositioning: ListView.InlineHeader

            header: Item {
                id: itemsHeading

                height: childrenRect.height
                width: parent.width

                Rectangle {
                    id: itemsSpacer;
                    height: 20
                }

                Rectangle {
                    id: itemsLoginStatus;
                    anchors {
                        top: itemsSpacer.bottom
                        left: parent.left
                        right: parent.right
                        leftMargin: 15
                    }
                    height: root.isLoggedIn ? 0 : 80

                    visible: !root.isLoggedIn
                    color: hifi.colors.greenHighlight
                    border.color: hifi.colors.greenShadow
                    border.width: 1
                    radius: 4
                    z: 10000

                    HifiControlsUit.Button {
                        id: loginButton;
                        anchors {
                            left: parent.left
                            top: parent.top
                            bottom: parent.bottom
                            leftMargin: 15
                            topMargin:10
                            bottomMargin: 10
                        }
                        width: 80;

                        text: "LOG IN"

                        onClicked: {
                            sendToScript({method: 'needsLogIn_loginClicked'});
                        }
                    }

                    RalewayRegular {
                        id: itemsLoginText

                        anchors {
                            leftMargin: 15
                            top: parent.top;
                            bottom: parent.bottom;
                            right: parent.right;
                            left: loginButton.right
                        }

                        text: "to get items from the Marketplace."
                        color: hifi.colors.baseGray
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        size: 18
                    }
                }

                Item {
                    id: breadcrumbs

                    anchors.top: itemsLoginStatus.bottom;
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 34
                    visible: categoriesListView.currentIndex >= 0

                    RalewayRegular {
                        id: categoriesItemText

                        anchors.leftMargin: 15
                        anchors.fill:parent

                        text: "Home /"
                        color: hifi.colors.blueHighlight
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        size: 18
                    }

                    MouseArea {
                        anchors.fill: parent
                        
                        onClicked: {
                            categoriesListView.currentIndex = -1;
                            categoriesText.text = "Categories";
                            root.categoryString = "";
                            searchField.text = "";
                            getMarketplaceItems();
                        }
                    }
                }
                
                Item {
                    id: searchScope
                    
                    anchors {
                        top: breadcrumbs.bottom
                        left: parent.left
                        right: parent.right
                    }
                    height: 50
                    
                    RalewaySemiBold {
                        id: searchScopeText;

                        anchors {
                            leftMargin: 15
                            fill:parent
                            topMargin: 10
                        }

                        text: searchScopeString
                        color: hifi.colors.baseGray
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        size: 22
                    }
                }
                Item {
                    id: sort
                    visible: searchString === undefined || searchString === ""

                    anchors {
                        top: searchScope.bottom;
                        left: parent.left;
                        right: parent.right;
                        topMargin: 10;
                        leftMargin: 15;
                    }
                    height: visible ? 36 : 0

                    RalewayRegular {
                        id: sortText

                        anchors.leftMargin: 15
                        anchors.rightMargin: 20
                        height: 34

                        text: "Sort:"
                        color: hifi.colors.lightGrayText
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        size: 14
                    }
                    
                    Rectangle {
                        anchors {
                            left: sortText.right
                            top: parent.top
                            leftMargin: 20
                        }

                        width: root.isLoggedIn ? 342 : 262
                        height: parent.height

                        radius: 4
                        border.width: 1
                        border.color: hifi.colors.faintGray

                        ListModel {
                            id: sortModel

                            ListElement {
                                name: "Name"
                                sortString: "alpha"
                                ascending: true
                            }

                            ListElement {
                                name: "Date"
                                sortString: "recent"
                                ascending: false
                            }

                            ListElement {
                                name: "Popular"
                                sortString: "likes"
                                ascending: false
                            }

                            ListElement {
                                name: "My Likes"
                                sortString: "my_likes"
                                ascending: false
                            }
                        }
                    
                        ListView {
                            id: sortListView

                            anchors {
                                top: parent.top
                                bottom: parent.bottom
                                left: parent.left
                                topMargin:1
                                bottomMargin:1
                                leftMargin:1
                                rightMargin:1
                            }
                            width: childrenRect.width
                            height: 34

                            orientation: ListView.Horizontal
                            focus: true
                            clip: true
                            highlightFollowsCurrentItem: false
                            currentIndex: 1;
                            
                            delegate: SortButton {
                                width: 85
                                height: parent.height

                                ascending: model.ascending
                                text: model.name
                                
                                visible: root.isLoggedIn || model.sortString != "my_likes"

                                checked: ListView.isCurrentItem

                                onClicked: {
                                    if(root.currentSortIndex == index) {
                                        ascending = !ascending;
                                    } else {
                                        ascending = model.ascending;
                                    }
                                    root.isAscending = ascending;
                                    root.currentSortIndex = index;
                                    sortListView.positionViewAtIndex(index, ListView.Beginning);
                                    sortListView.currentIndex = index;
                                    root.sortString = model.sortString;
                                    getMarketplaceItems();
                                }                            
                            }
                            highlight: Rectangle {
                                width: 85
                                height: parent.height

                                color: hifi.colors.faintGray
                                x: sortListView.currentItem.x
                            }

                            model: sortModel
                        } 
                    }
                }
            }
        }
    }
    
    //
    // ITEMS LIST END
    // 

    //
    // ITEM START
    //     
    Item {
        id: marketplaceItemView

        anchors.fill: parent
        anchors.topMargin: 115
        anchors.bottomMargin: 50
        width: parent.width

        visible: false

        ScrollView {
            id: marketplaceItemScrollView

            anchors.fill: parent

            clip: true
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn
            contentWidth: parent.width
            contentHeight: childrenRect.height

            function resize() {
                contentHeight = (marketplaceItemContent.y - itemSpacer.y + marketplaceItemContent.height);
            }

            Item {
                id: itemSpacer
                anchors.top: parent.top
                height: 15
            }

            Rectangle {
                id: itemLoginStatus;
                anchors {
                    left: parent.left
                    right: parent.right
                    top: itemSpacer.bottom
                    topMargin: 10
                    leftMargin: 15
                    rightMargin: 15
                }
                height: root.isLoggedIn ? 0 : 80

                visible: !root.isLoggedIn
                color: hifi.colors.greenHighlight
                border.color: hifi.colors.greenShadow
                border.width: 1
                radius: 4
                z: 10000

                HifiControlsUit.Button {
                    id: loginButton;
                    anchors {
                        left: parent.left
                        top: parent.top
                        bottom: parent.bottom
                        leftMargin: 15
                        topMargin:10
                        bottomMargin: 10
                    }
                    width: 80;

                    text: "LOG IN"

                    onClicked: {
                        sendToScript({method: 'needsLogIn_loginClicked'});
                    }
                }

                RalewayRegular {
                    id: itemsLoginText

                    anchors {
                        leftMargin: 15
                        top: parent.top;
                        bottom: parent.bottom;
                        right: parent.right;
                        left: loginButton.right
                    }

                    text: "to get items from the Marketplace."
                    color: hifi.colors.baseGray
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    size: 18
                }
            }


            Rectangle {
                id: marketplaceItemContent
                anchors.top: itemLoginStatus.bottom
                width: parent.width
                height: childrenRect.height;
                
                RalewaySemiBold {
                    id: backText

                    anchors {
                        top: parent.top
                        left: parent.left
                        topMargin: 10
                        leftMargin: 15
                        bottomMargin: 10
                    }
                    width: paintedWidth
                    height: paintedHeight

                    text: "Back"
                    size: hifi.fontSizes.overlayTitle
                    color: hifi.colors.blueHighlight
                    verticalAlignment: Text.AlignVCenter
                }

                MouseArea {
                    anchors.fill: backText
                    
                    onClicked: {
                        getMarketplaceItems();
                    }
                }

                MarketplaceItem {
                    id: marketplaceItem


                    anchors.topMargin: 15
                    anchors.top: backText.bottom
                    width: parent.width
                    height: childrenRect.height
                    
                    isLoggedIn: root.isLoggedIn;

                    onBuy: {
                        sendToScript({method: 'marketplace_checkout', itemId: item_id, itemEdition: edition});
                    }
                    
                    onShowLicense: {
                        var xhr = new XMLHttpRequest;
                        xhr.open("GET", url);
                        xhr.onreadystatechange = function() {
                            if (xhr.readyState == XMLHttpRequest.DONE) {
                                licenseText.text = xhr.responseText;
                                licenseInfo.visible = true;
                            }
                        };
                        xhr.send();
                    }
                    onCategoryClicked: {
                        root.categoryString = category;
                        categoriesText.text = category;                      
                        getMarketplaceItems();
                    }

                    onResized: {
                        marketplaceItemScrollView.resize();
                    }
                }
            }
        }
    }
    //
    // ITEM END
    //
    
    //
    // FOOTER START
    //
    
    Rectangle {
        id: footer

        anchors {
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        height: 50
        
        color: hifi.colors.blueHighlight

        Item {
            anchors {
                fill: parent
                rightMargin: 15
                leftMargin: 15
            }



            Item {
                id: footerText

                anchors.fill: parent
                visible: root.supports3DHTML && itemsList.visible

                HiFiGlyphs {
                    id: footerGlyph

                    anchors {
                        left: parent.left
                        top: parent.top
                        bottom: parent.bottom
                        rightMargin: 10
                    }

                    text: hifi.glyphs.info
                    size: 34
                    color: hifi.colors.white
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                RalewaySemiBold {
                    id: footerInfo

                    anchors {
                        left: footerGlyph.right
                        top: parent.top
                        bottom: parent.bottom
                    }

                    text: "Get items from Clara.io!"
                    color: hifi.colors.white
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    size: 18
                }
            }

            HifiControlsUit.Button {
                anchors {
                    left: parent.left
                    top: parent.top
                    bottom: parent.bottom
                    topMargin: 10
                    bottomMargin: 10
                    leftMargin: 10
                    rightMargin: 10
                }

                visible: marketplaceItemView.visible
                text: "< BACK"
                width: 100

                onClicked: {
                    marketplaceItemView.visible = false;
                    itemsList.visible = true;
                    licenseInfo.visible = false;
                }
            }
            
            HifiControlsUit.Button {
                anchors {
                    right: parent.right
                    top: parent.top
                    bottom: parent.bottom
                    topMargin: 10
                    bottomMargin: 10
                    leftMargin: 10
                    rightMargin: 10
                }

                visible: root.supports3DHTML

                text: "SEE ALL MARKETS"
                width: 180

                onClicked: {
                    sendToScript({method: 'marketplace_marketplaces', itemId: marketplaceItemView.visible ? marketplaceItem.item_id : undefined});
                }
            }
        }  
    }
    //
    // FOOTER END
    //
    
    
    //
    // LICENSE START
    //    
    Rectangle {
        id: licenseInfo

        anchors {
            fill: root
            topMargin: 120
            bottomMargin: 0
        }

        visible: false;

        ScrollView {
            anchors {
                bottomMargin: 1
                topMargin: 60
                leftMargin: 15
                fill: parent
            }

            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            RalewayRegular {
                id: licenseText

                width:440
                wrapMode: Text.Wrap

                text: ""
                size: 18;
                color: hifi.colors.baseGray
            }
        }

        Item {
            id: licenseClose

            anchors {
                top: parent.top
                right: parent.right
                topMargin: 10
                rightMargin: 10
            }
            width: 30
            height: 30

            HiFiGlyphs {
                anchors {
                    fill: parent
                    rightMargin: 0
                    verticalCenter: parent.verticalCenter
                }
                height: 30

                text: hifi.glyphs.close
                size: 34
                horizontalAlignment: Text.AlignHCenter
                color: hifi.colors.baseGray
            }
            
            MouseArea {
                anchors.fill: licenseClose

                onClicked: licenseInfo.visible = false;
            }
        }
    }
    //
    // LICENSE END
    //        

    HifiControlsUit.Keyboard {
        id: keyboard

        anchors {
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }

        raised: parent.keyboardEnabled && parent.keyboardRaised
        numeric: false
    }
    
    //
    // Function Name: fromScript()
    //
    // Relevant Variables:
    // None
    //
    // Arguments:
    // message: The message sent from the JavaScript, in this case the Marketplaces JavaScript.
    //     Messages are in format "{method, params}", like json-rpc.
    //
    // Description:
    // Called when a message is received from a script.
    //

    function fromScript(message) {
        switch (message.method) {
            case 'updateMarketplaceQMLItem':
                if (!message.params.itemId) {
                    console.log("A message with method 'updateMarketplaceQMLItem' was sent without an itemId!");
                    return;
                }
                pendingGetMarketplaceItemCall = true;
                marketplaceItem.edition = message.params.edition ? message.params.edition : -1;
                MarketplaceScriptingInterface.getMarketplaceItem(message.params.itemId);
                break;
        }
    }
}
