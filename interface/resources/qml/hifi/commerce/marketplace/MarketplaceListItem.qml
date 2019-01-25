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
    property string category: "";
    property int price: 0;
    property bool available: false;

    signal buy();
    signal showItem();
    signal categoryClicked(string category);
    signal requestReload();
    
    HifiConstants { id: hifi; }

    width: parent.width;
    height: childrenRect.height+50;
    
    DropShadow {
        anchors.fill: shadowBase;
        source: shadowBase;
        verticalOffset: 4;
        horizontalOffset: 4;
        radius: 6;
        samples: 9;
        color: hifi.colors.baseGrayShadow;
    }
    
    Rectangle {
        id: shadowBase;
        anchors.fill: itemRect;
        color: "white";
    }
    
    Rectangle {
        id: itemRect;
        height: childrenRect.height;
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.top: parent.top;
        anchors.topMargin: 20;
        anchors.bottomMargin: 10;
        anchors.leftMargin: 15;
        anchors.rightMargin: 15;        
        
        MouseArea {
            anchors.fill: parent;
            onClicked: root.showItem();
            onEntered: { hoverRect.visible = true; console.log("entered"); }
            onExited: { hoverRect.visible = false; console.log("exited"); }
            hoverEnabled: true           
        }
        
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
                size: hifi.fontSizes.overlayTitle;
                // Anchors
                anchors.top: parent.top;
                anchors.left: parent.left;
                anchors.right: parent.right;
                anchors.rightMargin: 50;
                anchors.leftMargin: 15;
                anchors.bottom: parent.bottom;
                elide: Text.ElideRight;
                width: paintedWidth;
                // Style
                color: hifi.colors.blueHighlight;
                // Alignment
                verticalAlignment: Text.AlignVCenter;
            }
            Item {
                id: likes;
                anchors.top: parent.top;
                anchors.right: parent.right;
                anchors.rightMargin: 15;
                anchors.bottom: parent.bottom;
                width: childrenRect.width;
                Connections {
                    target: MarketplaceScriptingInterface;

                    onMarketplaceItemLikeResult: {
                        if (result.status !== 'success') {
                            console.log("Failed to get Marketplace Categories", result.data.message);
                            root.requestReload();
                        }
                    }
                }                
                
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
                    anchors.fill: parent;
                    onClicked: {
                        console.log("like " + root.item_id);
                        root.liked = !root.liked;
                        root.likes = root.liked ? root.likes + 1 : root.likes - 1;
                        MarketplaceScriptingInterface.marketplaceItemLike(root.item_id, root.liked);
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
            height: 60;
            
            RalewaySemiBold {
                id: creatorLabel;
                text: "CREATOR:";
                // Text size
                size: 14;
                // Anchors
                anchors.top: parent.top;
                anchors.left: parent.left;
                anchors.leftMargin: 15;
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
                size: 14;
                // Anchors
                anchors.top: creatorLabel.top;
                anchors.left: creatorLabel.right;
                anchors.leftMargin: 15;
                width: paintedWidth;
                // Style
                color: hifi.colors.lightGray;
                // Alignment
                verticalAlignment: Text.AlignVCenter;
            }
            RalewaySemiBold {
                id: categoryLabel;
                text: "IN:";
                // Text size
                size: 14;
                // Anchors
                anchors.top: creatorLabel.bottom;
                anchors.left: parent.left;
                anchors.leftMargin: 15;
                width: paintedWidth;
                // Style
                color: hifi.colors.lightGrayText;
                // Alignment
                verticalAlignment: Text.AlignVCenter;
            }
            
            RalewaySemiBold {
                id: categoryText;
                text: root.category;
                // Text size
                size: 14;
                // Anchors
                anchors.top: categoryLabel.top;
                anchors.left: categoryLabel.right;
                anchors.leftMargin: 15;
                width: paintedWidth;
                // Style
                color: hifi.colors.blueHighlight;
                // Alignment
                verticalAlignment: Text.AlignVCenter;
                
                MouseArea {
                    anchors.fill: parent;
                    onClicked: root.categoryClicked(root.category);
                }
            }

            HifiControlsUit.Button {
                text: root.price ? root.price : "FREE";
                buttonGlyph: root.price ? hifi.glyphs.hfc : "";
                anchors.right: parent.right;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                anchors.rightMargin: 15;
                anchors.topMargin:10;
                anchors.bottomMargin: 10;
                color: hifi.buttons.blue;
                
                onClicked: root.buy();
            }
        }
        Rectangle {
            id: hoverRect;
            anchors.fill: parent;
            border.color: hifi.colors.blueHighlight;
            border.width: 2;
            color: "#00000000";
            visible: false;
        }        
    }
}
