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
    property int category_index: -1;
    property alias categoryChoices: categoriesModel;
 
    anchors.fill: (typeof parent === undefined) ? undefined : parent;

    Component.onDestruction: {
        KeyboardScriptingInterface.raised = false;
    }
    
    Connections {
        target: Marketplace;

        onGetMarketplaceCategoriesResult: {
            if (result.status !== 'success') {
                console.log("Failed to get Marketplace Categories", result.data.message);
            } else {

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
    Item {
        id: header;
        visible: true;
        width: parent.width;
        anchors.left: parent.left;
        anchors.top: parent.top;
        anchors.right: parent.right;
        
        Item {
            id: titleBarContainer;
            visible: true;
            // Size
            width: parent.width;
            height: 50;
            // Anchors
            anchors.left: parent.left;
            anchors.top: parent.top;

            // Wallet icon
            Image {
                id: walletIcon;
                source: "../../../../images/hifi-logo-blackish.svg";
                height: 20
                width: walletIcon.height;
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
                anchors.left: walletIcon.right;
                anchors.leftMargin: 6;
                anchors.bottom: parent.bottom;
                width: paintedWidth;
                // Style
                color: hifi.colors.black;
                // Alignment
                verticalAlignment: Text.AlignVCenter;
            }
        }

        Item {
            id: searchBarContainer;
            visible: true;
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
                color: "white";
                radius: 4;
                border.width: 1;
                border.color: hifi.colors.lightGray;
                

                // Categories Text
                RalewayRegular {
                    id: categoriesText;
                    text: "Categories";
                    // Text size
                    size: 18;
                    // Style
                    color: hifi.colors.baseGray;
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
                    }
                    onEntered: categoriesText.color = hifi.colors.baseGrayShadow;
                    onExited: categoriesText.color = hifi.colors.baseGray;
                }

                Component.onCompleted: {
                    console.log("Getting Marketplace Categories");
                    console.log(JSON.stringify(Marketplace));
                    Marketplace.getMarketplaceItems();
                }                
            }
            
            Rectangle {
                id: categoriesContainer;
                visible: true;
                height: 50 * categoriesModel.count;
                width: parent.width;
                anchors.top: categoriesButton.bottom;
                anchors.left: categoriesButton.left;
                color: hifi.colors.white;

                ListModel {
                    id: categoriesModel;
                }

                ListView {
                    id: dropdownListView;
                    interactive: false;
                    anchors.fill: parent;
                    model: categoriesModel;
                    delegate: Item {
                        width: parent.width;
                        height: 50;
                        Rectangle {
                            id: dropDownButton;
                            color: hifi.colors.white;
                            width: parent.width;
                            height: 50;
                            visible: true;

                            RalewaySemiBold {
                                id: dropDownButtonText;
                                text: model.displayName;
                                anchors.fill: parent;
                                anchors.topMargin: 2;
                                anchors.leftMargin: 12;
                                color: hifi.colors.baseGray;
                                horizontalAlignment: Text.AlignLeft;
                                verticalAlignment: Text.AlignVCenter;
                                size: 18;
                            }

                            MouseArea {
                                anchors.fill: parent;
                                hoverEnabled: true;
                                propagateComposedEvents: false;
                                onEntered: {
                                    dropDownButton.color = hifi.colors.blueHighlight;
                                }
                                onExited: {
                                    dropDownButton.color = hifi.colors.white;
                                }
                                onClicked: {
                                    root.category_index = index;
                                    dropdownContainer.visible = false;
                                }
                            }
                        }
                        Rectangle {
                            height: 2;
                            width: parent.width;
                            color: hifi.colors.lightGray;
                            visible: model.separator
                        }
                    }
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
                    text: root.primaryFilter_displayName;
                }

                // workaround for https://bugreports.qt.io/browse/QTBUG-49297
                Keys.onPressed: {
                    switch (event.key) {
                        case Qt.Key_Return: 
                        case Qt.Key_Enter: 
                            event.accepted = true;

                            // emit accepted signal manually
                            if (acceptableInput) {
                                root.accepted();
                                root.forceActiveFocus();
                            }
                        break;
                        case Qt.Key_Backspace:
                            if (textField.text === "") {
                                primaryFilter_index = -1;
                            }
                        break;
                    }
                }

                onAccepted: {
                    root.forceActiveFocus();
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
    DropShadow {
        anchors.fill: header;
        horizontalOffset: 0;
        verticalOffset: 4;
        radius: 4.0;
        samples: 9
        color: Qt.rgba(0, 0, 0, 0.25);
        source: header;
        visible: header.visible;
    }
}
