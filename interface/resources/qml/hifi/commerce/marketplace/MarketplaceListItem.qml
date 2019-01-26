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
    id: root

    property string item_id: ""
    property string image_url: ""
    property string name: ""
    property int likes: 0
    property bool liked: false
    property string creator: ""
    property string category: ""
    property int price: 0
    property bool available: false
    property bool isLoggedIn: false;

    signal buy()
    signal showItem()
    signal categoryClicked(string category)
    signal requestReload()
    
    HifiConstants { id: hifi }

    width: parent.width
    height: childrenRect.height+50
    
    DropShadow {
        anchors.fill: shadowBase

        source: shadowBase
        verticalOffset: 4
        horizontalOffset: 4
        radius: 6
        samples: 9
        color: hifi.colors.baseGrayShadow
    }
    
    Rectangle {
        id: shadowBase

        anchors.fill: itemRect

        color: "white"
    }
    
    Rectangle {
        id: itemRect
        
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            topMargin: 20
            bottomMargin: 10
            leftMargin: 15
            rightMargin: 15
        }
        height: childrenRect.height

        MouseArea {
            anchors.fill: parent

            hoverEnabled: true

            onClicked: root.showItem();
            onEntered: hoverRect.visible = true;
            onExited: hoverRect.visible = false;
       
        }
        
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
                    right: parent.right
                    rightMargin: 50
                    leftMargin: 15
                    bottom: parent.bottom
                }
                width: paintedWidth

                text: root.name
                size: hifi.fontSizes.overlayTitle
                elide: Text.ElideRight
                color: hifi.colors.blueHighlight
                verticalAlignment: Text.AlignVCenter
            }

            Item {
                id: likes

                anchors {
                    top: parent.top
                    right: parent.right
                    rightMargin: 15
                    bottom: parent.bottom
                }
                width: heart.width + likesText.width

                Connections {
                    target: MarketplaceScriptingInterface

                    onMarketplaceItemLikeResult: {
                        if (result.status !== 'success') {
                            console.log("Failed to get Marketplace Categories", result.data.message);
                            root.requestReload();
                        }
                    }
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
                    horizontalAlignment: Text.AlignHCenter;
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
                        top: parent.top
                        bottom: parent.bottom
                    }
                    onClicked: {
                        if(isLoggedIn) {
                            root.liked = !root.liked;
                            root.likes = root.liked ? root.likes + 1 : root.likes - 1;
                            MarketplaceScriptingInterface.marketplaceItemLike(root.item_id, root.liked);
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
            fillMode: Image.PreserveAspectCrop
        }

        Item {
            id: footer

            anchors {
                left: parent.left
                right: parent.right
                top: itemImage.bottom
            }
            height: 60
            
            RalewaySemiBold {
                id: creatorLabel
                
                anchors {
                    top: parent.top
                    left: parent.left
                    leftMargin: 15
                }
                width: paintedWidth
                
                text: "CREATOR:"
                size: 14
                color: hifi.colors.lightGrayText
                verticalAlignment: Text.AlignVCenter
            }
            
            RalewaySemiBold {
                id: creatorText

                anchors {
                    top: creatorLabel.top;
                    left: creatorLabel.right;
                    leftMargin: 15;
                }
                width: paintedWidth;

                text: root.creator;
                size: 14;
                color: hifi.colors.lightGray;
                verticalAlignment: Text.AlignVCenter;
            }

            RalewaySemiBold {
                id: categoryLabel
                
                anchors {
                    top: creatorLabel.bottom
                    left: parent.left
                    leftMargin: 15
                }
                width: paintedWidth;      

                text: "IN:";
                size: 14;
                color: hifi.colors.lightGrayText;
                verticalAlignment: Text.AlignVCenter;
            }

            RalewaySemiBold {
                id: categoryText

                anchors {
                    top: categoryLabel.top
                    left: categoryLabel.right
                    leftMargin: 15
                }
                width: paintedWidth

                text: root.category
                size: 14
                color: hifi.colors.blueHighlight;
                verticalAlignment: Text.AlignVCenter;
                
                MouseArea {
                    anchors.fill: parent

                    onClicked: root.categoryClicked(root.category);
                }
            }

            HifiControlsUit.Button {
                anchors {
                    right: parent.right
                    top: parent.top
                    bottom: parent.bottom
                    rightMargin: 15
                    topMargin:10
                    bottomMargin: 10
                }

                text: root.price ? root.price : "FREE"
                buttonGlyph: root.price ? hifi.glyphs.hfc : ""
                color: hifi.buttons.blue;

                onClicked: root.buy();
            }
        }

        Rectangle {
            id: hoverRect

            anchors.fill: parent

            border.color: hifi.colors.blueHighlight
            border.width: 2
            color: "#00000000"
            visible: false
        }        
    }
}
