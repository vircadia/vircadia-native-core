//
//  MarketplaceListItem.qml
//  qml/hifi/commerce/marketplace
//
//  MarketplaceItem
//
//  Created by Roxanne Skelly on 2019-01-22
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.9
import QtQuick.Controls 2.2
import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit
import "../../../controls" as HifiControls
import "../common" as HifiCommerceCommon
import "qrc:////qml//hifi//models" as HifiModels  // Absolute path so the same code works everywhere.
import "../common/sendAsset"
import "../.." as HifiCommon

Rectangle {
    id: root;

    property string item_id: ""
    property string image_url: ""
    property string name: ""
    property int likes: 0
    property bool liked: false
    property string creator: ""
    property var categories: []
    property int price: 0
    property string availability: "unknown"
    property string updated_item_id: ""
    property var attributions: []
    property string description: ""
    property string license: ""
    property string posted: ""
    property string created_at: ""
    property bool isLoggedIn: false;
    property int edition: -1;
    property bool supports3DHTML: false;
    
    onCategoriesChanged: {
        categoriesListModel.clear();
        categories.forEach(function(category) {
            categoriesListModel.append({"category":category});
        });
    }
    
    onDescriptionChanged: {
    
        if(root.supports3DHTML) {
            descriptionTextModel.clear();
            descriptionTextModel.append({text: description});
        } else {
            descriptionText.text = description;
        }
    }

    onAttributionsChanged: {
        attributionsModel.clear();
        if(root.attributions) {
            root.attributions.forEach(function(attribution) {
                attributionsModel.append(attribution);
            });
        }
        footer.evalHeight();
    }

    signal buy()
    signal categoryClicked(string category)
    signal showLicense(string url)
    
    HifiConstants {
        id: hifi 
    }
    
    Connections {
        target: MarketplaceScriptingInterface

        onMarketplaceItemLikeResult: {
            if (result.status !== 'success') {
                console.log("Like/Unlike item", result.data.message);
            } else {
                root.liked = !root.liked;
                root.likes = root.liked ? root.likes + 1 : root.likes - 1;
            }
        }
    }
    function getFormattedDate(timestamp) {
        function addLeadingZero(n) {
            return n < 10 ? '0' + n : '' + n;
        }

        var a = new Date(timestamp);

        var year = a.getFullYear();
        var month = addLeadingZero(a.getMonth() + 1);
        var day = addLeadingZero(a.getDate());
        var hour = a.getHours();
        var drawnHour = hour;
        if (hour === 0) {
            drawnHour = 12;
        } else if (hour > 12) {
            drawnHour -= 12;
        }
        drawnHour = addLeadingZero(drawnHour);
        
        var amOrPm = "AM";
        if (hour >= 12) {
            amOrPm = "PM";
        }

        var min = addLeadingZero(a.getMinutes());
        var sec = addLeadingZero(a.getSeconds());
        return a.toDateString() + " " + drawnHour + ':' + min + amOrPm;
    }
    function evalHeight() {
        height = footer.y - header.y + footer.height;
    }
    
    signal resized()
    
    onHeightChanged: {
        resized();
    }
    
    anchors {
        left: parent.left
        right: parent.right
        leftMargin: 15
        rightMargin: 15
    }
    height: footer.y - header.y + footer.height

    Rectangle {
        id: header
        
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }
        height: 50
        
        RalewaySemiBold {
            id: nameText

            anchors {
                top: parent.top
                left: parent.left
                bottom: parent.bottom
            }
            width: paintedWidth
            
            text: root.name
            size: 24
            color: hifi.colors.baseGray
            verticalAlignment: Text.AlignVCenter
        }

        Item {
            id: likes

            anchors {
                top: parent.top
                right: parent.right
                bottom: parent.bottom
                rightMargin: 5
            }

            RalewaySemiBold {
                id: heart

                anchors {
                    top: parent.top
                    right: parent.right
                    rightMargin: 0
                    verticalCenter: parent.verticalCenter
                }
                
                text: "\u2665"
                size: 20
                horizontalAlignment: Text.AlignHCenter
                color: root.liked ? hifi.colors.redAccent  : hifi.colors.lightGrayText
            }
            
            RalewaySemiBold {
                id: likesText

                anchors {
                    top: parent.top
                    right: heart.left
                    rightMargin: 5
                    leftMargin: 15
                    bottom: parent.bottom
                }
                width: paintedWidth

                text: root.likes
                size: hifi.fontSizes.overlayTitle
                color: hifi.colors.baseGray
                verticalAlignment: Text.AlignVCenter
            }

            MouseArea {
                anchors {
                    left: likesText.left
                    right: heart.right
                    top: likesText.top
                    bottom: likesText.bottom
                }
                
                onClicked: {
                    if (isLoggedIn) {
                        MarketplaceScriptingInterface.marketplaceItemLike(root.item_id, !root.liked);
                    }
                }
            }                
        }
    }

    Image {
        id: itemImage

        anchors {
            top: header.bottom
            left: parent.left
            right: parent.right
        }
        height: width*0.5625

        source: root.image_url
        fillMode: Image.PreserveAspectCrop;
    }

    Item {
        id: footer

        anchors {
            left: parent.left;
            right: parent.right;
            top: itemImage.bottom;
        }
        height: categoriesList.y - buyButton.y + categoriesList.height
        
        function evalHeight() {
            height = categoriesList.y - buyButton.y + categoriesList.height;
            console.log("HEIGHT: " + height);
        }
        
        HifiControlsUit.Button {
            id: buyButton

            anchors {
                right: parent.right
                top: parent.top
                left: parent.left
                topMargin: 15
            }
            height: 50 

            property bool isUpdate: root.edition >= 0  // Special case of updating from a specific older item
            property bool isStocking: (creator === Account.username) && (availability === "not for sale") && !updated_item_id // Note: server will say "sold out" or "invalidated" before it says NFS
            property bool isFreeSpecial: isStocking || isUpdate
            enabled: isFreeSpecial || (availability === 'available')
            buttonGlyph: (enabled && !isUpdate && (price > 0)) ? hifi.glyphs.hfc : ""
            text: isUpdate ? "UPDATE FOR FREE" : (isStocking ? "FREE STOCK" : (enabled ? (price || "FREE") : availability))
            color: hifi.buttons.blue
            
            onClicked: root.buy();
        }
        
        Item {
            id: creatorItem

            anchors {
                top: buyButton.bottom
                leftMargin: 15
                topMargin: 15
            }
            width: parent.width
            height: childrenRect.height
            
            RalewaySemiBold {
                id: creatorLabel
                
                anchors.top: parent.top
                anchors.left: parent.left
                width: paintedWidth
                
                text: "CREATOR:"
                size: 14
                color: hifi.colors.lightGrayText
                verticalAlignment: Text.AlignVCenter
            }
            
            RalewaySemiBold {
                id: creatorText
                
                anchors {
                    top: creatorLabel.bottom
                    left: parent.left
                    topMargin: 10
                }
                width: paintedWidth              
                text: root.creator

                size: 18
                color: hifi.colors.blueHighlight
                verticalAlignment: Text.AlignVCenter
            }
        }
        
        Item {
            id: posted
            
            anchors {
                top: creatorItem.bottom
                leftMargin: 15
                topMargin: 15
            }
            width: parent.width
            height: childrenRect.height

            RalewaySemiBold {
                id: postedLabel

                anchors.top: parent.top
                anchors.left: parent.left
                width: paintedWidth
                
                text: "POSTED:"
                size: 14
                color: hifi.colors.lightGrayText
                verticalAlignment: Text.AlignVCenter
            }
            
            RalewaySemiBold {
                id: created_at

                anchors {
                    top: postedLabel.bottom
                    left: parent.left
                    right: parent.right
                    topMargin: 5
                }

                text: { getFormattedDate(root.created_at); }
                size: 14
                color: hifi.colors.lightGray
                verticalAlignment: Text.AlignVCenter
            }           
        }
        
        Rectangle {

            anchors {
                top: posted.bottom
                leftMargin: 15
                topMargin: 15
            }
            width: parent.width
            height: 1

            color: hifi.colors.lightGrayText
        }

        Item {
            id: attributions

            anchors {
                top: posted.bottom
                topMargin: 30
                left: parent.left
                right: parent.right
            }
            width: parent.width
            height: attributionsModel.count > 0 ? childrenRect.height : 0
            visible: attributionsModel.count > 0

            RalewaySemiBold {
                id: attributionsLabel

                anchors.top: parent.top
                anchors.left: parent.left
                width: paintedWidth
                height: paintedHeight

                text: "ATTRIBUTIONS:"
                size: 14
                color: hifi.colors.lightGrayText
                verticalAlignment: Text.AlignVCenter
            }
            ListModel {
                id: attributionsModel
            }

            ListView {
                anchors {
                    left: parent.left
                    right: parent.right
                    top: attributionsLabel.bottom
                    bottomMargin: 15
                }

                height: 24*model.count+10

                model: attributionsModel
                delegate: Item {
                    height: 24
                    width: parent.width
				    RalewaySemiBold {
						id: attributionName

						anchors.leftMargin: 15
						height: 24
                        width: parent.width

						text: model.name
                        elide: Text.ElideRight
						size: 14

						color: (model.link && root.supports3DHTML)? hifi.colors.blueHighlight : hifi.colors.baseGray
						verticalAlignment: Text.AlignVCenter
						MouseArea {
							anchors.fill: parent

							onClicked: {
                                if (model.link && root.supports3DHTML) {
                                    sendToScript({method: 'marketplace_open_link', link: model.link});
                                }
                            }
						}
                    }
                }
            }
            Rectangle {

                anchors {
                    bottom: attributions.bottom
                    leftMargin: 15
                }
                width: parent.width
                height: 1

                color: hifi.colors.lightGrayText
            }
        }
        Item {
            id: licenseItem;
            
            anchors {
                top: attributions.bottom
                left: parent.left
                topMargin: 15
            }
            width: parent.width
            height: childrenRect.height

            RalewaySemiBold {
                id: licenseLabel

                anchors.top: parent.top;
                anchors.left: parent.left;
                width: paintedWidth;

                text: "SHARED UNDER:";
                size: 14;
                color: hifi.colors.lightGrayText;
                verticalAlignment: Text.AlignVCenter;
            }

            RalewaySemiBold {
                id: licenseText
                
                anchors.top: licenseLabel.bottom
                anchors.left: parent.left
                anchors.topMargin: 5
                width: paintedWidth

                text: root.license
                size: 14
                color: hifi.colors.lightGray
                verticalAlignment: Text.AlignVCenter
            }

            RalewaySemiBold {
                id: licenseHelp

                anchors.top: licenseText.bottom
                anchors.left: parent.left
                anchors.topMargin: 5
                width: paintedWidth

                text: "More about this license"
                size: 14
                color: hifi.colors.blueHighlight
                verticalAlignment: Text.AlignVCenter
                
                MouseArea {
                    anchors.fill: parent
                    
                    onClicked: { 
                        licenseInfo.visible = true;
                        var url;
                        if (root.license === "No Rights Reserved (CC0)") {
                          url = "https://creativecommons.org/publicdomain/zero/1.0/"
                        } else if (root.license === "Attribution (CC BY)") {
                          url = "https://creativecommons.org/licenses/by/4.0/legalcode.txt"
                        } else if (root.license === "Attribution-ShareAlike (CC BY-SA)") {
                          url = "https://creativecommons.org/licenses/by-sa/4.0/legalcode.txt"
                        } else if (root.license === "Attribution-NoDerivs (CC BY-ND)") {
                          url = "https://creativecommons.org/licenses/by-nd/4.0/legalcode.txt"
                        } else if (root.license === "Attribution-NonCommercial (CC BY-NC)") {
                          url = "https://creativecommons.org/licenses/by-nc/4.0/legalcode.txt"
                        } else if (root.license === "Attribution-NonCommercial-ShareAlike (CC BY-NC-SA)") {
                          url = "https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.txt"
                        } else if (root.license === "Attribution-NonCommercial-NoDerivs (CC BY-NC-ND)") {
                          url = "https://creativecommons.org/licenses/by-nc-nd/4.0/legalcode.txt"
                        } else if (root.license === "Proof of Provenance License (PoP License)") {
                          url = "licenses/Popv1.txt"
                        }
                        if(url) {
                            showLicense(url)
                        }
                    }
                }
            }
        }

        Item {
            id: descriptionItem
            property string text: ""

            anchors {
                top: licenseItem.bottom
                topMargin: 15
                left: parent.left
                right: parent.right
            }
            height: childrenRect.height
            onHeightChanged: {
                footer.evalHeight();
            }
            RalewaySemiBold {
                id: descriptionLabel
                
                anchors.top: parent.top
                anchors.left: parent.left
                width: paintedWidth
                height: 20

                text: "DESCRIPTION:"
                size: 14
                color: hifi.colors.lightGrayText
                verticalAlignment: Text.AlignVCenter
            }

            RalewaySemiBold {
                id: descriptionText
                
                anchors.top: descriptionLabel.bottom
                anchors.left: parent.left
                anchors.topMargin: 5
                width: parent.width
            
                text: root.description
                size: 14
                color: hifi.colors.lightGray
                linkColor: root.supports3DHTML ? hifi.colors.blueHighlight : hifi.colors.lightGray 
                verticalAlignment: Text.AlignVCenter
                textFormat: Text.RichText
                wrapMode: Text.Wrap
                onLinkActivated: {
                    if (root.supports3DHTML) {
                        sendToScript({method: 'marketplace_open_link', link: link});
                    }
                }
                
                onHeightChanged: { footer.evalHeight(); }
            }
        }
        
        Item {
            id: categoriesList

            anchors {
                top: descriptionItem.bottom
                topMargin: 15
                left: parent.left
                right: parent.right
            }
            width: parent.width
            height: categoriesListModel.count*24 + categoryLabel.height + (isLoggedIn ? 50 : 150)
            
            onHeightChanged: { footer.evalHeight(); }

            RalewaySemiBold {
                id: categoryLabel
                
                anchors.top: parent.top
                anchors.left: parent.left
                width: paintedWidth

                text: "IN:"
                size: 14
                color: hifi.colors.lightGrayText
                verticalAlignment: Text.AlignVCenter
            }
            ListModel {
                id: categoriesListModel
            }
            
            ListView {
                anchors {
                    left: parent.left
                    right: parent.right
                    top: categoryLabel.bottom
                    bottomMargin: 15
                }

                height: 24*model.count+10

                model: categoriesListModel
                delegate: RalewaySemiBold {
                    id: categoryText

                    anchors.leftMargin: 15
                    width: paintedWidth

                    text: model.category
                    size: 14
                    height: 24
                    color: hifi.colors.blueHighlight
                    verticalAlignment: Text.AlignVCenter
                    
                    MouseArea {
                        anchors.fill: categoryText

                        onClicked: root.categoryClicked(model.category);
                    }
                }
            }
        }
    }
}
