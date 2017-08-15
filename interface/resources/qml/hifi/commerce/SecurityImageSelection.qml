//
//  SecurityImageSelection.qml
//  qml/hifi/commerce
//
//  SecurityImageSelection
//
//  Created by Zach Fox on 2017-08-15
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.5
import QtQuick.Controls 1.4
import "../../styles-uit"
import "../../controls-uit" as HifiControlsUit
import "../../controls" as HifiControls

// references XXX from root context

Rectangle {
    HifiConstants { id: hifi; }

    id: securityImageSelectionRoot;
    property string referrerURL: "";
    property bool isManuallyChangingSecurityImage: false;
    anchors.fill: parent;
    // Style
    color: hifi.colors.baseGray;
    z:999; // On top of everything else
    visible: false;

    Hifi.QmlCommerce {
        id: commerce;
        onSecurityImageResult: {
            if (!isManuallyChangingSecurityImage) {
                securityImageSelectionRoot.visible = (imageID == 0);
            }
            if (imageID > 0) {
                for (var itr = 0; itr < gridModel.count; itr++) {
                    var thisValue = gridModel.get(itr).securityImageEnumValue;
                    if (thisValue === imageID) {
                        securityImageGrid.currentIndex = itr;
                        break;
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        commerce.getSecurityImage();
    }

    //
    // TITLE BAR START
    //
    Item {
        id: titleBarContainer;
        // Size
        width: securityImageSelectionRoot.width;
        height: 30;
        // Anchors
        anchors.left: parent.left;
        anchors.top: parent.top;

        // Title Bar text
        RalewaySemiBold {
            id: titleBarText;
            text: "Select a Security Image";
            // Text size
            size: hifi.fontSizes.overlayTitle;
            // Anchors
            anchors.fill: parent;
            anchors.leftMargin: 16;
            // Style
            color: hifi.colors.lightGrayText;
            // Alignment
            horizontalAlignment: Text.AlignHLeft;
            verticalAlignment: Text.AlignVCenter;
        }

        // Separator
        HifiControlsUit.Separator {
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.bottom: parent.bottom;
        }
    }
    //
    // TITLE BAR END
    //

    //
    // EXPLANATION START
    //
    Item {
        id: explanationContainer; 
        // Size
        width: securityImageSelectionRoot.width;
        height: 85;
        // Anchors
        anchors.top: titleBarContainer.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;

        RalewayRegular {
            id: explanationText;
            text: "This image will be displayed on secure Inventory and Marketplace Checkout dialogs.<b><br>If you don't see your selected image on these dialogs, do not use them!</b>";
            // Text size
            size: 16;
            // Anchors
            anchors.top: parent.top;
            anchors.topMargin: 4;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            // Style
            color: hifi.colors.lightGrayText;
            wrapMode: Text.WordWrap;
            // Alignment
            horizontalAlignment: Text.AlignHLeft;
            verticalAlignment: Text.AlignVCenter;
        }

        // Separator
        HifiControlsUit.Separator {
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.bottom: parent.bottom;
        }
    }
    //
    // EXPLANATION END
    //
    
    //
    // SECURITY IMAGE GRID START
    //
    Item {
        id: securityImageGridContainer;
        // Anchors
        anchors.left: parent.left;
        anchors.leftMargin: 8;
        anchors.right: parent.right;
        anchors.rightMargin: 8;
        anchors.top: explanationContainer.bottom;
        anchors.topMargin: 8;
        anchors.bottom: actionButtonsContainer.top;
        anchors.bottomMargin: 8;

        SecurityImageModel {
            id: gridModel;
        }

        GridView {
            id: securityImageGrid;
            clip: true;
            // Anchors
            anchors.fill: parent;
            currentIndex: -1;
            cellWidth: width / 2;
            cellHeight: height / 3;
            model: gridModel;
            delegate: Item {
                width: securityImageGrid.cellWidth;
                height: securityImageGrid.cellHeight;
                Item {
                    anchors.fill: parent;
                    Image {
                        width: parent.width - 8;
                        height: parent.height - 8;
                        source: sourcePath;
                        anchors.horizontalCenter: parent.horizontalCenter;
                        anchors.verticalCenter: parent.verticalCenter;
                        fillMode: Image.PreserveAspectFit;
                        mipmap: true;
                    }
                }
                MouseArea {
                    anchors.fill: parent;
                    onClicked: {
                        securityImageGrid.currentIndex = index;
                    }
                }
            }
            highlight: Rectangle {
                    width: securityImageGrid.cellWidth;
                    height: securityImageGrid.cellHeight;
                    color: hifi.colors.blueHighlight;
                }
        }
    }
    //
    // SECURITY IMAGE GRID END
    //
    
    
    //
    // ACTION BUTTONS START
    //
    Item {
        id: actionButtonsContainer;
        // Size
        width: securityImageSelectionRoot.width;
        height: 40;
        // Anchors
        anchors.left: parent.left;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 8;

        // "Cancel" button
        HifiControlsUit.Button {
            id: cancelButton;
            color: hifi.buttons.black;
            colorScheme: hifi.colorSchemes.dark;
            anchors.top: parent.top;
            anchors.topMargin: 3;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 3;
            anchors.left: parent.left;
            anchors.leftMargin: 20;
            width: parent.width/2 - anchors.leftMargin*2;
            text: "Cancel"
            onClicked: {
                if (!securityImageSelectionRoot.isManuallyChangingSecurityImage) {
                    sendToScript({method: 'securityImageSelection_cancelClicked', referrerURL: securityImageSelectionRoot.referrerURL});
                } else {
                    securityImageSelectionRoot.visible = false;
                }
            }
        }

        // "Confirm" button
        HifiControlsUit.Button {
            id: confirmButton;
            color: hifi.buttons.black;
            colorScheme: hifi.colorSchemes.dark;
            anchors.top: parent.top;
            anchors.topMargin: 3;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 3;
            anchors.right: parent.right;
            anchors.rightMargin: 20;
            width: parent.width/2 - anchors.rightMargin*2;
            text: "Confirm";
            onClicked: {
                securityImageSelectionRoot.isManuallyChangingSecurityImage = false;
                commerce.chooseSecurityImage(gridModel.get(securityImageGrid.currentIndex).securityImageEnumValue);
            }
        }
    }
    //
    // ACTION BUTTONS END
    //
    
    //
    // FUNCTION DEFINITIONS START
    //
    signal sendToScript(var message);

    function getImagePathFromImageID(imageID) {
        return (imageID ? gridModel.get(imageID - 1).sourcePath : "");
    }
    //
    // FUNCTION DEFINITIONS END
    //
}
