//
//  TabletAssetDialog.qml
//
//  Created by David Rowe on 18 Apr 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import stylesUit 1.0

Rectangle {
    width: 480
    height: 706

    color: "#404040"

    Column {
        x: 45
        y: 30
        spacing: 5

        Image {
            width: 400; height: 73
            fillMode: Image.PreserveAspectFit
            source: "../../../images/vircadia-logo.svg"
        }
        Item { height: 30; width: 1 }
        Column {
            id: buildColumm
            anchors.left: parent.left
            anchors.leftMargin: 70
            RalewayRegular {
                text: "Build " + HiFiAbout.buildVersion
                size: 16
                color: "white"
            }
            RalewayRegular {
                text: "Released " + HiFiAbout.buildDate
                size: 16
                color: "white"
            }
        }
        Item { height: 10; width: 1 }
        RalewayRegular {
            text: "An open source virtual reality platform."
            size: 20
            color: "white"
        }
        RalewayRegular {
            textFormat: Text.StyledText
            linkColor: "#00B4EF"
            color: "white"
            text: "<a href=\"https:/github.com/kasenvr/project-athena\">Vircadia Github</a>."
            size: 20
            onLinkActivated: {
                HiFiAbout.openUrl("https:/github.com/kasenvr/project-athena");
            }

        }
        Item { height: 40; width: 1 }
        Row {
            spacing: 5
            Image {
                sourceSize.width: 34
                sourceSize.height: 25
                source: "../../../images/about-qt.png"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        HiFiAbout.openUrl("https://www.qt.io/");
                    }
                }
            }
            RalewayRegular {
                color: "white"
                text: "Built using Qt " + HiFiAbout.qtVersion
                size: 12
                anchors.verticalCenter: parent.verticalCenter
            }
            Item { height: 1; width: 15 }
            Image {
                sourceSize.width: 70
                sourceSize.height: 26
                source: "../../../images/about-physics.png"
            }
            RalewayRegular {
                color: "white"
                text: "Physics powered by Bullet"
                size: 12
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        Row {
            spacing: 5
            Image {
                sourceSize.width: 34
                sourceSize.height: 25
                source: "../../../images/about-opus.png"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        HiFiAbout.openUrl("http://opus-codec.org/");
                    }
                }
            }
            RalewayRegular {
                color: "white"
                text: "Built using the Opus codec."
                size: 12
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        Item { height: 20; width: 1 }
        RalewayRegular {
            color: "white"
            text: "© 2019-2020 Vircadia contributors."
            size: 14
        }
        RalewayRegular {
            color: "white"
            text: "© 2012 - 2019 High Fidelity, Inc. All rights reserved."
            size: 14
        }
        RalewayRegular {
            textFormat: Text.StyledText
            color: "white"
            linkColor: "#00B4EF"
            text: "Distributed under the <a href=\"http://www.apache.org/licenses/LICENSE-2.0.html\">Apache License, Version 2.0.</a>."
            size: 14
            onLinkActivated: {
                HiFiAbout.openUrl("http://www.apache.org/licenses/LICENSE-2.0.html");
            }
        }
    }
}
