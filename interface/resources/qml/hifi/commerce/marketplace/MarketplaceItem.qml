//
//  MarketplaceListItem.qml
//  qml/hifi/commerce/marketplace
//
//  MarketplaceListItem
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
import QtGraphicalEffects 1.0
import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit
import "../../../controls" as HifiControls
import "../common" as HifiCommerceCommon
import "qrc:////qml//hifi//models" as HifiModels  // Absolute path so the same code works everywhere.
import "../common/sendAsset"
import "../.." as HifiCommon

Rectangle {
    id: root;
    property string item_id: "";
    property string image_url: "";
    property string name: "";
    property int likes: 0;
    property bool liked: false;
    property string creator: "";
    property var categories: [];
    property int price: 0;
    property var attributions: [];
    property string description: "";
    property string license: "";
    property string posted: "";
    property bool available: false;
    property string created_at: "";
    
    onCategoriesChanged: {
        categoriesListModel.clear();
        categories.forEach(function(category) {
        console.log("category is " + category);
            categoriesListModel.append({"category":category});
        });
    }

    signal buy();
    signal categoryClicked(string category);
    signal showLicense(string url);
    
    HifiConstants { id: hifi; }
    
    Connections {
        target: MarketplaceScriptingInterface;

        onMarketplaceItemLikeResult: {
            if (result.status !== 'success') {
                console.log("Failed to get Marketplace Categories", result.data.message);
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
    
    anchors.left: parent.left;
    anchors.right: parent.right;
    anchors.leftMargin: 15;
    anchors.rightMargin: 15;
    height: childrenRect.height;
    
    
    Rectangle {
        id: header;
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.top: parent.top;
        
        height: 50;
        
        RalewaySemiBold {
            id: nameText;
            text: root.name;
            // Text size
            size: 24;
            // Anchors
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;
            width: paintedWidth;
            // Style
            color: hifi.colors.baseGray;
            // Alignment
            verticalAlignment: Text.AlignVCenter;
        }
        Item {
            id: likes;
            anchors.top: parent.top;
            anchors.right: parent.right;
            anchors.bottom: parent.bottom;
            anchors.rightMargin: 5;
            
            RalewaySemiBold {
                id: heart;
                text: "\u2665";
                // Size
                size: 20;
                // Anchors
                anchors.top: parent.top;
                anchors.right: parent.right;
                anchors.rightMargin: 0;
                anchors.verticalCenter: parent.verticalCenter;
                horizontalAlignment: Text.AlignHCenter;
                // Style
                color: root.liked ? hifi.colors.redAccent  : hifi.colors.lightGrayText;
            }
            
            RalewaySemiBold {
                id: likesText;
                text: root.likes;
                // Text size
                size: hifi.fontSizes.overlayTitle;
                // Anchors
                anchors.top: parent.top;
                anchors.right: heart.left;
                anchors.rightMargin: 5;
                anchors.leftMargin: 15;
                anchors.bottom: parent.bottom;
                width: paintedWidth;
                // Style
                color: hifi.colors.baseGray;
                // Alignment
                verticalAlignment: Text.AlignVCenter;
            }
            MouseArea {
                anchors.left: likesText.left;
                anchors.right: heart.right;
                anchors.top: likesText.top;
                anchors.bottom: likesText.bottom;
                
                onClicked: {
                    MarketplaceScriptingInterface.marketplaceItemLike(root.item_id, !root.liked);
                }
            }                
        }
    }
    Image {
        id: itemImage;
        source: root.image_url;
        anchors.top: header.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;
        height: width*0.5625
        fillMode: Image.PreserveAspectCrop;
    }
    Item {
        id: footer;
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.top: itemImage.bottom;
        height: childrenRect.height;
        
        HifiControlsUit.Button {
            id: buyButton;
            text: root.available ? (root.price ? root.price : "FREE") : "UNAVAILABLE (not for sale)";
            enabled: root.available;
            buttonGlyph: root.available ? (root.price ? hifi.glyphs.hfc : "") : "";
            anchors.right: parent.right;
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.topMargin: 15;
            height: 50;
            color: hifi.buttons.blue;
            
            onClicked: root.buy();
        }        
        
        Item {
            id: creatorItem;
            anchors.top: buyButton.bottom;
            anchors.leftMargin: 15;
            anchors.topMargin: 15;
            width: parent.width;
            height: childrenRect.height;
            
            RalewaySemiBold {
                id: creatorLabel;
                text: "CREATOR:";
                // Text size
                size: 14;
                // Anchors
                anchors.top: parent.top;
                anchors.left: parent.left;
                width: paintedWidth;
                // Style
                color: hifi.colors.lightGrayText;
                // Alignment
                verticalAlignment: Text.AlignVCenter;
            }
            
            RalewaySemiBold {
                id: creatorText;
                text: root.creator;
                // Text size
                size: 18;
                // Anchors
                anchors.top: creatorLabel.bottom;
                anchors.left: parent.left;
                anchors.topMargin: 10;
                width: paintedWidth;
                // Style
                color: hifi.colors.blueHighlight;
                // Alignment
                verticalAlignment: Text.AlignVCenter;
            }
        }
        
        Item {
            id: posted;
            anchors.top: creatorItem.bottom;
            anchors.leftMargin: 15;
            anchors.topMargin: 15;
            width: parent.width;
            height: childrenRect.height;
            RalewaySemiBold {
                id: postedLabel;
                text: "POSTED:";
                // Text size
                size: 14;
                // Anchors
                anchors.top: parent.top;
                anchors.left: parent.left;

                width: paintedWidth;
                // Style
                color: hifi.colors.lightGrayText;
                // Alignment
                verticalAlignment: Text.AlignVCenter;
            }
            
            RalewaySemiBold {
                id: created_at;
                text: { getFormattedDate(root.created_at); }
                // Text size
                size: 14;
                // Anchors
                anchors.top: postedLabel.bottom;
                anchors.left: parent.left;
                anchors.right: parent.right;
                anchors.topMargin: 10;
                // Style
                color: hifi.colors.lightGray;
                // Alignment
                verticalAlignment: Text.AlignVCenter;
            }           
        }
        
        Rectangle {
            anchors.top: posted.bottom;
            anchors.leftMargin: 15;
            anchors.topMargin: 15;
            width: parent.width;
            height: 1;
            color: hifi.colors.lightGray;
        }

        Item {
            id: licenseItem;
            anchors.top: posted.bottom;
            anchors.left: parent.left;
            anchors.topMargin: 30;
            width: parent.width;  
            height: childrenRect.height;
            RalewaySemiBold {
                id: licenseLabel;
                text: "SHARED UNDER:";
                // Text size
                size: 14;
                // Anchors
                anchors.top: parent.top;
                anchors.left: parent.left;
                width: paintedWidth;
                // Style
                color: hifi.colors.lightGrayText;
                // Alignment
                verticalAlignment: Text.AlignVCenter;
            }
            
            RalewaySemiBold {
                id: licenseText;
                text: root.license;
                // Text size
                size: 14;
                // Anchors
                anchors.top: licenseLabel.bottom;
                anchors.left: parent.left;
                width: paintedWidth;
                // Style
                color: hifi.colors.lightGray;
                // Alignment
                verticalAlignment: Text.AlignVCenter;
            }
            RalewaySemiBold {
                id: licenseHelp;
                text: "More about this license";
                // Text size
                size: 14;
                // Anchors
                anchors.top: licenseText.bottom;
                anchors.left: parent.left;
                width: paintedWidth;
                // Style
                color: hifi.colors.blueHighlight;
                // Alignment
                verticalAlignment: Text.AlignVCenter;
                
                MouseArea {
                    anchors.fill: parent;
                    
                    onClicked: { 
                        licenseInfo.visible = true;
                        var url;
                        if (root.license == "No Rights Reserved (CC0)") {
                          url = "https://creativecommons.org/publicdomain/zero/1.0/"
                        } else if (root.license == "Attribution (CC BY)") {
                          url = "https://creativecommons.org/licenses/by/4.0/"
                        } else if (root.license == "Attribution-ShareAlike (CC BY-SA)") {
                          url = "https://creativecommons.org/licenses/by-sa/4.0/"
                        } else if (root.license == "Attribution-NoDerivs (CC BY-ND)") {
                          url = "https://creativecommons.org/licenses/by-nd/4.0/"
                        } else if (root.license == "Attribution-NonCommercial (CC BY-NC)") {
                          url = "https://creativecommons.org/licenses/by-nc/4.0/"
                        } else if (root.license == "Attribution-NonCommercial-ShareAlike (CC BY-NC-SA)") {
                          url = "https://creativecommons.org/licenses/by-nc-sa/4.0/"
                        } else if (root.license == "Attribution-NonCommercial-NoDerivs (CC BY-NC-ND)") {
                          url = "https://creativecommons.org/licenses/by-nc-nd/4.0/"
                        } else if (root.license == "Proof of Provenance License (PoP License)") {
                          url = "https://digitalassetregistry.com/PoP-License/v1/"
                        }
                        if(url) {
                            licenseInfoWebView.url = url;
                            
                        }
                    }
                }
            }
        }

        Item {
            id: descriptionItem;
            anchors.top: licenseItem.bottom;
            anchors.topMargin: 15;
            anchors.left: parent.left;
            anchors.right: parent.right;
            height: childrenRect.height;            
            RalewaySemiBold {
                id: descriptionLabel;
                text: "DESCRIPTION:";
                // Text size
                size: 14;
                // Anchors
                anchors.top: parent.top;
                anchors.left: parent.left;
                width: paintedWidth;
                // Style
                color: hifi.colors.lightGrayText;
                // Alignment
                verticalAlignment: Text.AlignVCenter;
            }
            RalewaySemiBold {
                id: descriptionText;
                text: root.description;
                // Text size
                size: 14;
                // Anchors
                anchors.top: descriptionLabel.bottom;
                anchors.left: parent.left;
                width: parent.width;
                // Style
                color: hifi.colors.lightGray;
                // Alignment
                verticalAlignment: Text.AlignVCenter;
                wrapMode: Text.Wrap;
            }
        }
        
        Item {
            id: categoriesList;
            anchors.top: descriptionItem.bottom;
            anchors.topMargin: 15;
            anchors.left: parent.left;
            anchors.right: parent.right;            
            width: parent.width;
            height: childrenRect.height;
            RalewaySemiBold {
                id: categoryLabel;
                text: "IN:";
                // Text size
                size: 14;
                // Anchors
                anchors.top: parent.top;
                anchors.left: parent.left;
                width: paintedWidth;
                // Style
                color: hifi.colors.lightGrayText;
                // Alignment
                verticalAlignment: Text.AlignVCenter;
            }
            ListModel {
                id: categoriesListModel;
            }
            
            ListView {
                anchors.left: parent.left;
                anchors.right: parent.right;
                anchors.top: categoryLabel.bottom;
                model: categoriesListModel;
                height: 20*model.count;
                delegate: RalewaySemiBold {
                    id: categoryText;
                    text: model.category;
                    // Text size
                    size: 14;
                    // Anchors
                    anchors.leftMargin: 15;
                    width: paintedWidth;
                    height: 20;
                    // Style
                    color: hifi.colors.blueHighlight;
                    // Alignment
                    verticalAlignment: Text.AlignVCenter;
                    
                    MouseArea {
                        anchors.fill: categoryText;
                        onClicked: root.categoryClicked(model.category);
                    }
                }
            }
        }
    }
}
