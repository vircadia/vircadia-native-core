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
    HifiConstants { id: hifi; }

    id: root;

    property string activeView: "initialize";
    property bool keyboardRaised: false;
    property int currentSortIndex: 0;
    property string sortString: "";
    property string categoryString: "";
    property string searchString: "";
 
    anchors.fill: (typeof parent === undefined) ? undefined : parent;

    function getMarketplaceItems() {
        marketplaceItemView.visible = false;
        itemsList.visible = true;          
        marketBrowseModel.getFirstPage();
    }
    
    Component.onDestruction: {
        KeyboardScriptingInterface.raised = false;
    }
    
    Connections {
        target: MarketplaceScriptingInterface;

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
            marketBrowseModel.handlePage(result.status !== "success" && result.message, result);
        }
        
        onGetMarketplaceItemResult: {
            if (result.status !== 'success') {
                console.log("Failed to get Marketplace Item", result.data.message);
            } else {
            
                console.log(JSON.stringify(result.data));
                marketplaceItem.item_id = result.data.id;
                marketplaceItem.image_url = result.data.thumbnail_url;
                marketplaceItem.name = result.data.title;
                marketplaceItem.likes = result.data.likes;
                marketplaceItem.liked = result.data.has_liked;
                marketplaceItem.creator = result.data.creator;
                marketplaceItem.categories = result.data.categories;
                marketplaceItem.price = result.data.cost;
                marketplaceItem.description = result.data.description;
                marketplaceItem.attributions = result.data.attributions;
                marketplaceItem.license = result.data.license;
                marketplaceItem.available = result.data.availability == "available";
                marketplaceItem.created_at = result.data.created_at;
                console.log("HEIGHT: " + marketplaceItemContent.height);
                marketplaceItemScrollView.contentHeight = marketplaceItemContent.height;
                itemsList.visible = false;
                marketplaceItemView.visible = true;
            }
        }
    }

    HifiCommerceCommon.CommerceLightbox {
        id: lightboxPopup;
        visible: false;
        anchors.fill: parent;
    }

    //
    // HEADER BAR START
    //
    
    
    Rectangle {
        id: headerShadowBase;
        anchors.fill: header;
        color: "white";
    }
    DropShadow {
        anchors.fill: headerShadowBase;
        source: headerShadowBase;
        verticalOffset: 4;
        horizontalOffset: 4;
        radius: 6;
        samples: 9;
        color: hifi.colors.baseGrayShadow;
        z:5;
    }    
    Rectangle {
        id: header;
        visible: true;
        anchors.left: parent.left;
        anchors.top: parent.top;
        anchors.right: parent.right;
        anchors.topMargin: -1;
        anchors.leftMargin: -1;
        anchors.rightMargin: -1;
        height: childrenRect.height+5;
        z: 5;
    
        Rectangle {
            id: titleBarContainer;
            visible: true;
            // Size
            width: parent.width;
            height: 50;
            // Anchors
            anchors.left: parent.left;
            anchors.top: parent.top;
              
            
            // Marketplace icon
            Image {
                id: marketplaceIcon;
                source: "../../../../images/hifi-logo-blackish.svg";
                height: 20
                width: marketplaceIcon.height;
                anchors.left: parent.left;
                anchors.leftMargin: 8;
                anchors.verticalCenter: parent.verticalCenter;
                visible: true;
            }

            // Title Bar text
            RalewaySemiBold {
                id: titleBarText;
                text: "Marketplace";
                // Text size
                size: hifi.fontSizes.overlayTitle;
                // Anchors
                anchors.top: parent.top;
                anchors.left: marketplaceIcon.right;
                anchors.leftMargin: 6;
                anchors.bottom: parent.bottom;
                width: paintedWidth;
                // Style
                color: hifi.colors.black;
                // Alignment
                verticalAlignment: Text.AlignVCenter;
            }
        }

        Rectangle {
            id: searchBarContainer;
            visible: true;
            clip: false;
            // Size
            width: parent.width;
            anchors.top: titleBarContainer.bottom;
            height: 50;


           Rectangle {
                id: categoriesButton;
                anchors.left: parent.left;
                anchors.leftMargin: 10;
                anchors.verticalCenter: parent.verticalCenter;
                height: 34;
                width: categoriesText.width + 25;
                color: hifi.colors.white;
                radius: 4;
                border.width: 1;
                border.color: hifi.colors.lightGrayText;
                

                // Categories Text
                RalewayRegular {
                    id: categoriesText;
                    text: "Categories";
                    // Text size
                    size: 14;
                    // Style
                    color: hifi.colors.lightGrayText;
                    elide: Text.ElideRight;
                    horizontalAlignment: Text.AlignHCenter;
                    verticalAlignment: Text.AlignVCenter;
                    width: Math.min(textMetrics.width + 25, 110);
                    // Anchors
                    anchors.centerIn: parent;
                    rightPadding: 10;
                }
                HiFiGlyphs {
                    id: categoriesDropdownIcon;
                    text: hifi.glyphs.caratDn;
                    // Size
                    size: 34;
                    // Anchors
                    anchors.right: parent.right;
                    anchors.rightMargin: -8;
                    anchors.verticalCenter: parent.verticalCenter;
                    horizontalAlignment: Text.AlignHCenter;
                    // Style
                    color: hifi.colors.baseGray;
                }

                MouseArea {
                    anchors.fill: parent;
                    hoverEnabled: enabled;
                    onClicked: {
                        categoriesDropdown.visible = !categoriesDropdown.visible;
                        categoriesButton.color = categoriesDropdown.visible ? hifi.colors.lightGray : hifi.colors.white;
                        categoriesDropdown.forceActiveFocus();
                    }
                    onEntered: categoriesText.color = hifi.colors.baseGrayShadow;
                    onExited: categoriesText.color = hifi.colors.baseGray;
                }

                Component.onCompleted: {
                    console.log("Getting Marketplace Categories");
                    MarketplaceScriptingInterface.getMarketplaceCategories();
                }                
            }
                
            // or
            RalewayRegular {
                id: orText;
                text: "or";
                // Text size
                size: 18;
                // Style
                color: hifi.colors.baseGray;
                elide: Text.ElideRight;
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignVCenter;
                width: Math.min(textMetrics.width + 25, 110);
                // Anchors
                anchors.left: categoriesButton.right;
                rightPadding: 10;
                leftPadding: 10;
                anchors.verticalCenter: parent.verticalCenter;            
            }
            HifiControlsUit.TextField {
                id: searchField;
                anchors.verticalCenter: parent.verticalCenter;
                anchors.right: parent.right;
                anchors.left: orText.right;
                anchors.rightMargin: 10;
                height: 34;
                isSearchField: true;
                colorScheme: hifi.colorSchemes.faintGray;
                

                font.family: "Fira Sans"
                font.pixelSize: hifi.fontSizes.textFieldInput;

                placeholderText: "Search Marketplace";
                            
                TextMetrics {
                    id: primaryFilterTextMetrics;
                    font.family: "FiraSans Regular";
                    font.pixelSize: hifi.fontSizes.textFieldInput;
                    font.capitalization: Font.AllUppercase;
                }

                // workaround for https://bugreports.qt.io/browse/QTBUG-49297
                Keys.onPressed: {
                    switch (event.key) {
                        case Qt.Key_Return: 
                        case Qt.Key_Enter: 
                            event.accepted = true;

                            // emit accepted signal manually
                            if (acceptableInput) {
                                searchField.accepted();
                                searchField.forceActiveFocus();
                            }
                        break;
                        case Qt.Key_Backspace:
                            if (searchField.text === "") {
                                primaryFilter_index = -1;
                            }
                        break;
                    }
                }
                onTextChanged: root.searchString = text;
                onAccepted: {
                    root.searchString = searchField.text;
                    getMarketplaceItems();
                    searchField.forceActiveFocus();
                }

                onActiveFocusChanged: {
                    if (!activeFocus) {
                        dropdownContainer.visible = false;
                    }
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
        id: categoriesDropdown;    
        anchors.fill: parent;
        visible: false;
        z: 10;
        
        MouseArea {
            anchors.fill: parent;
            propagateComposedEvents: true;
            onClicked: {
                categoriesDropdown.visible = false;
                categoriesButton.color = hifi.colors.white;
            }            
        }
        
        Rectangle {
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;
            anchors.top: parent.top;
            anchors.topMargin: 100;
            width: parent.width/3;
            color: hifi.colors.white;
            
            ListModel {
                id: categoriesModel;
            }

            ListView {
                id: categoriesListView;
                anchors.fill: parent;
                anchors.rightMargin: 10;
                width: parent.width;
                clip: true;
                
                model: categoriesModel;
                delegate: ItemDelegate {
                    height: 34;
                    width: parent.width;
                    clip: true;
                    contentItem: Rectangle {
                        id: categoriesItem;
                        anchors.fill: parent;
                        color: hifi.colors.white;
                        visible: true;

                        RalewayRegular {
                            id: categoriesItemText;
                            text: model.name;
                            anchors.leftMargin: 15;
                            anchors.fill:parent;
                            color: ListView.isCurrentItem ? hifi.colors.lightBlueHighlight : hifi.colors.baseGray;
                            horizontalAlignment: Text.AlignLeft;
                            verticalAlignment: Text.AlignVCenter;
                            size: 14;
                        }
                    }
                    MouseArea {
                        anchors.fill: parent;
                        hoverEnabled: true;
                        propagateComposedEvents: false;
                        z: 10;
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
                    parent: categoriesListView.parent;
                    anchors.top: categoriesListView.top;
                    anchors.bottom: categoriesListView.bottom;
                    anchors.left: categoriesListView.right;
                    contentItem.opacity: 1;
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
        id: itemsList;
        anchors.fill: parent;
        anchors.topMargin: 100;
        anchors.bottomMargin: 50;
        visible: true;
        
        HifiModels.PSFListModel {
            id: marketBrowseModel;
            itemsPerPage: 7;
            listModelName: 'marketBrowse';
            listView: marketBrowseList;
            getPage: function () {

                MarketplaceScriptingInterface.getMarketplaceItems(
                    root.searchString == "" ? undefined : root.searchString,
                    "",
                    root.categoryString.toLowerCase(),
                    "",
                    "",
                    root.sortString,
                    false,
                    marketBrowseModel.currentPageToRetrieve,
                    marketBrowseModel.itemsPerPage
                );
            }
            processPage: function(data) {
                console.log(JSON.stringify(data));
                data.items.forEach(function (item) {
                    console.log(JSON.stringify(item));
                });
                return data.items;
            }
        }
        ListView {
            id: marketBrowseList;
            model: marketBrowseModel;
            // Anchors
            anchors.fill: parent;
            anchors.rightMargin: 10;
            orientation: ListView.Vertical;
            focus: true;
            clip: true;            
            
            delegate: MarketplaceListItem {
                item_id: model.id;
                image_url:model.thumbnail_url;
                name: model.title;
                likes: model.likes;
                liked: model.has_liked;
                creator: model.creator;
                category: model.primary_category;
                price: model.cost;
                available: model.availability == "available";
                anchors.topMargin: 10;
                
                
                Component.onCompleted: {
                    console.log("Rendering marketplace list item " + model.id);
                    console.log(marketBrowseModel.count);
                }
                
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
                parent: marketBrowseList.parent;
                anchors.top: marketBrowseList.top;
                anchors.bottom: marketBrowseList.bottom;
                anchors.left: marketBrowseList.right;
                contentItem.opacity: 1;
            }            
            headerPositioning: ListView.InlineHeader;
            header: Item {
                id: itemsHeading;
                
                height: childrenRect.height;
                width: parent.width;
                
                Item {
                    id: breadcrumbs;
                    anchors.left: parent.left;
                    anchors.right: parent.right;
                    height: 34;
                    visible: categoriesListView.currentIndex >= 0;
                    RalewayRegular {
                        id: categoriesItemText;
                        text: "Home /";
                        anchors.leftMargin: 15;
                        anchors.fill:parent;
                        color: hifi.colors.blueHighlight;
                        horizontalAlignment: Text.AlignLeft;
                        verticalAlignment: Text.AlignVCenter;
                        size: 18;
                    }
                    MouseArea {
                        anchors.fill: parent;
                        onClicked: {
                            categoriesListView.currentIndex = -1;
                            categoriesText.text = "Categories";
                            root.categoryString = "";
                            getMarketplaceItems();
                        }
                    }
                }
                
                Item {
                    id: searchScope;
                    anchors.top: breadcrumbs.bottom;
                    anchors.left: parent.left;
                    anchors.right: parent.right;
                    height: 50;
                    
                    RalewaySemiBold {
                        id: searchScopeText;
                        text: "Featured";
                        anchors.leftMargin: 15;
                        anchors.fill:parent;
                        anchors.topMargin: 10;
                        color: hifi.colors.baseGray;
                        horizontalAlignment: Text.AlignLeft;
                        verticalAlignment: Text.AlignVCenter;
                        size: 22;
                    }
                }
                Item {
                    id: sort;
                    anchors.top: searchScope.bottom;
                    anchors.left: parent.left;
                    anchors.right: parent.right;
                    anchors.topMargin: 10;
                    anchors.leftMargin: 15;
                    height: childrenRect.height;
                    RalewayRegular {
                        id: sortText;
                        text: "Sort:";
                        anchors.leftMargin: 15;
                        anchors.rightMargin: 20;
                        height: 34;
                        color: hifi.colors.lightGrayText;
                        horizontalAlignment: Text.AlignLeft;
                        verticalAlignment: Text.AlignVCenter;
                        size: 14;
                    }
                    
                    Rectangle {
                        radius: 4;
                        border.width: 1;
                        border.color: hifi.colors.faintGray;
                        anchors.left: sortText.right;
                        anchors.top: parent.top;
                        width: 322;
                        height: 36;
                        anchors.leftMargin: 20;
                        
                        
                        ListModel {
                            id: sortModel;
                            ListElement {
                                name: "Name";
                                glyph: ";";
                                sortString: "alpha";
                            }
                            ListElement {
                                name: "Date";
                                glyph: ";";
                                sortString: "recent";
                            }
                            ListElement {
                                name: "Popular";
                                glyph: ";";
                                sortString: "likes";
                            }
                            ListElement {
                                name: "My Likes";
                                glyph: ";";
                                sortString: "my_likes";
                            }
                        }
                    
                        ListView {
                            id: sortListView;
                            anchors.top: parent.top;
                            anchors.bottom: parent.bottom;
                            anchors.left: parent.left;
                            anchors.topMargin:1;
                            anchors.bottomMargin:1;
                            anchors.leftMargin:1;
                            anchors.rightMargin:1;
                            width: childrenRect.width;
                            height: 34;
                            orientation: ListView.Horizontal;
                            focus: true;
                            clip: true;
                            highlightFollowsCurrentItem: false;
                            
                            delegate: SortButton {
                                width: 80;
                                height: parent.height;
                                glyph: model.glyph;
                                text: model.name;
                                
                                checked: {
                                    ListView.isCurrentItem;
                                }
                                onClicked: {
                                    root.currentSortIndex = index;
                                    sortListView.positionViewAtIndex(index, ListView.Beginning);
                                    sortListView.currentIndex = index;
                                    root.sortString = model.sortString;
                                    getMarketplaceItems();
                                }                            
                            }
                            highlight: Rectangle {
                                width: 80;
                                height: parent.height;
                                color: hifi.colors.faintGray;
                                x: sortListView.currentItem.x;
                            }
                            model: sortModel;
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
        id: marketplaceItemView;
        anchors.fill: parent;
        width: parent.width;
        anchors.topMargin: 120;
        visible: false;
        
        ScrollView {
            id: marketplaceItemScrollView;
            anchors.fill: parent;

            clip: true;
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn;
            contentWidth: parent.width;

            Rectangle {
                id: marketplaceItemContent;
                width: parent.width;
                height: childrenRect.height + 100;
                
                // Title Bar text
                RalewaySemiBold {
                    id: backText;
                    text: "Back";
                    // Text size
                    size: hifi.fontSizes.overlayTitle;
                    // Anchors
                    anchors.top: parent.top;
                    anchors.left: parent.left
                    anchors.leftMargin: 15;
                    anchors.bottomMargin: 10;
                    width: paintedWidth;
                    height: paintedHeight;
                    // Style
                    color: hifi.colors.blueHighlight;
                    // Alignment
                    verticalAlignment: Text.AlignVCenter;
                }
                MouseArea {
                    anchors.fill: backText;
                    
                    onClicked: {
                        getMarketplaceItems();
                    }
                }
                
                MarketplaceItem {
                    id: marketplaceItem;
                    anchors.top: backText.bottom;
                    width: parent.width;
                    height: childrenRect.height;
                    anchors.topMargin: 15;
                    
                    onBuy: {
                        sendToScript({method: 'marketplace_checkout', itemId: item_id});
                    }
                    
                    onShowLicense: {
                        licenseInfoWebView.url = url;
                        licenseInfo.visible = true;
                    }
                    onCategoryClicked: {
                        root.categoryString = category;
                        categoriesText.text = category;                      
                        getMarketplaceItems();
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
        id: footer;
        anchors.bottom: parent.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;
        height: 50;
        
        color: hifi.colors.blueHighlight;

        Item {
            anchors.fill: parent;
            anchors.rightMargin: 15;
            anchors.leftMargin: 15;
        
            HiFiGlyphs {
                id: footerGlyph;
                text: hifi.glyphs.info;
                // Size
                size: 34;
                // Anchors
                anchors.left: parent.left;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                
                anchors.rightMargin: 10;
                // Style
                color: hifi.colors.white;
                // Alignment
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignVCenter;
            }

            RalewaySemiBold {
                id: footerInfo;
                text: "Get items from Clara.io!";
                anchors.left: footerGlyph.right;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                color: hifi.colors.white;
                horizontalAlignment: Text.AlignLeft;
                verticalAlignment: Text.AlignVCenter;
                size: 18;
            }
            
            HifiControlsUit.Button {
                text: "SEE ALL MARKETS";
                anchors.right: parent.right;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                anchors.topMargin: 10;
                anchors.bottomMargin: 10;
                anchors.leftMargin: 10;
                anchors.rightMargin: 10;
                width: 180;
                
                onClicked: {
                    sendToScript({method: 'marketplace_marketplaces'});
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
        id: licenseInfo;
        anchors.fill: root;
        anchors.topMargin: 100;
        anchors.bottomMargin: 0;
        visible: false;
        
        
        HifiControlsUit.WebView {
            id: licenseInfoWebView;
            anchors.bottomMargin: 1;
            anchors.topMargin: 50;
            anchors.leftMargin: 1;
            anchors.rightMargin: 1;
            anchors.fill: parent;
        }
        Item {
            id: licenseClose;
            anchors.top: parent.top;
            anchors.right: parent.right;
            anchors.topMargin: 10;
            anchors.rightMargin: 10;
            
            width: 30;
            height: 30;
            HiFiGlyphs {
                anchors.fill: parent;
                height: 30;
                text: hifi.glyphs.close;
                // Size
                size: 34;
                // Anchors
                anchors.rightMargin: 0;
                anchors.verticalCenter: parent.verticalCenter;
                horizontalAlignment: Text.AlignHCenter;
                // Style
                color: hifi.colors.baseGray;
            }
            
            MouseArea {
                anchors.fill: licenseClose;
                onClicked: licenseInfo.visible = false;
            }
        }
    }
    //
    // LICENSE END
    //        

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
        console.log("FROM SCRIPT " + JSON.stringify(message));
        switch (message.method) {
            case 'updateMarketplaceQMLItem':
                if (!message.params.itemId) {
                    console.log("A message with method 'updateMarketplaceQMLItem' was sent without an itemId!");
                    return;
                }

                MarketplaceScriptingInterface.getMarketplaceItem(message.params.itemId);
                break;
        }
    }
}
