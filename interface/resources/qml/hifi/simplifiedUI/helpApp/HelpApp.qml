//
//  HelpApp.qml
//
//  Created by Zach Fox on 2019-08-07
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import QtQuick.Controls 2.3
import "../simplifiedConstants" as SimplifiedConstants
import "../simplifiedControls" as SimplifiedControls
import stylesUit 1.0 as HifiStylesUit
import "./controls" as HelpControls
import "./faq" as HelpFAQ
import "./about" as HelpAbout
import "./support" as HelpSupport

Rectangle {
    property string activeTabView: "controlsTabView"
    id: root
    color: simplifiedUI.colors.darkBackground
    anchors.fill: parent

    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }
            
    focus: true

    Component.onCompleted: {
        root.forceActiveFocus();
    }


    onActiveTabViewChanged: {
        for (var i = 0; i < tabListModel.count; i++) {
            if (tabListModel.get(i).tabViewName === activeTabView) {
                tabListView.currentIndex = i;
                return;
            }
        }
    }


    Rectangle {
        id: tabContainer
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 64
        color: simplifiedUI.colors.highlightOnDark

        ListModel {
            id: tabListModel

            ListElement {
                tabTitle: "Controls"
                tabViewName: "controlsTabView"
            }
            ListElement {
                tabTitle: "Support"
                tabViewName: "supportTabView"
            }
            ListElement {
                tabTitle: "FAQ"
                tabViewName: "faqTabView"
            }
            ListElement {
                tabTitle: "About"
                tabViewName: "aboutTabView"
            }
        }


        Component {
            id: highlightBar
            Rectangle {
                width: tabListView.currentItem.width
                height: tabListView.currentItem.height
                color: simplifiedUI.colors.darkBackground
                x: tabListView.currentItem.x
                Behavior on x {
                    SmoothedAnimation {
                        duration: 250
                    }
                }
                Behavior on width {
                    SmoothedAnimation {
                        duration: 250
                    }
                }
            }
        }


        ListView {
            id: tabListView
            anchors.fill: parent
            contentHeight: parent.height
            contentWidth: childrenRect.width
            orientation: ListView.Horizontal
            model: tabListModel
            highlight: highlightBar
            highlightFollowsCurrentItem: false
            interactive: contentItem.width > width
            delegate: Item {
                width: tabTitleText.paintedWidth + 32
                height: parent.height

                HifiStylesUit.GraphikRegular {
                    id: tabTitleText
                    color: simplifiedUI.colors.text.white
                    anchors.fill: parent
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    text: model.tabTitle
                    size: 24
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.activeTabView = model.tabViewName;
                    }
                }
            }
        }
    }

    Item {
        id: tabViewContainers
        anchors.top: tabContainer.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom


        HelpControls.HelpControls {
            id: controlsTabViewContainer
            visible: activeTabView === "controlsTabView"
            anchors.fill: parent
        }

        HelpSupport.HelpSupport {
            id: supportTabViewContainer
            visible: activeTabView === "supportTabView"
            anchors.fill: parent
        }

        HelpFAQ.HelpFAQ {
            id: faqTabViewContainer
            visible: activeTabView === "faqTabView"
            anchors.fill: parent

            onSendToScript: {
                root.sendToScript(message);
            }
        }

        HelpAbout.HelpAbout {
            id: aboutTabViewContainer
            visible: activeTabView === "aboutTabView"
            anchors.fill: parent
        }

        SimplifiedControls.VerticalScrollBar {
            parent: {
                if (activeTabView === "controlsTabView") {
                    controlsTabViewContainer
                } else if (activeTabView === "supportTabView") {
                    supportTabViewContainer
                } else if (activeTabView === "faqTabView") {
                    faqTabViewContainer
                } else if (activeTabView === "aboutTabView") {
                    aboutTabViewContainer
                }
            }
        }
    }


    function fromScript(message) {
        switch (message.method) {
            default:
                console.log('HelpApp.qml: Unrecognized message from JS');
                break;
        }
    }
    signal sendToScript(var message);
}
