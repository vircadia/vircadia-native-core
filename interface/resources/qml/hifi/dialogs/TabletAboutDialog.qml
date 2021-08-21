//
//  TabletAssetDialog.qml
//
//  Created by David Rowe on 18 Apr 2017
//  Copyright 2017 High Fidelity, Inc.
//  Copyright 2020 Vircadia contributors.
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
            source: "../../../images/vircadia-banner.svg"
        }
        Item { height: 25; width: 1 }
        Column {
            id: buildColumn
            anchors.left: parent.left
            anchors.leftMargin: 0
            RalewayRegular {
                text: "Interface"
                size: 16
                color: "white"
            }
            RalewayRegular {
                text: "Build " + About.buildVersion + " " + About.releaseName
                size: 16
                color: "white"
            }
            RalewayRegular {
                text: "Released " + About.buildDate
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
            text: "<a href=\"https://vircadia.com\">Website</a>"
            size: 20
            onLinkActivated: {
                About.openUrl("https://vircadia.com");
            }

        }
        RalewayRegular {
            textFormat: Text.StyledText
            linkColor: "#00B4EF"
            color: "white"
            text: "<a href=\"https://github.com/vircadia/vircadia\">Source</a>"
            size: 20
            onLinkActivated: {
                About.openUrl("https://github.com/vircadia/vircadia");
            }

        }
        Item { height: 25; width: 1 }
        Row {
            spacing: 5
            Image {
                sourceSize.width: 34
                sourceSize.height: 25
                source: "../../../images/about-qt.png"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        About.openUrl("https://www.qt.io/");
                    }
                }
            }
            RalewayRegular {
                color: "white"
                text: "Built using Qt " + About.qtVersion
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
                        About.openUrl("http://opus-codec.org/");
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
            text: "© 2019 - 2021 Vircadia contributors."
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
                About.openUrl("http://www.apache.org/licenses/LICENSE-2.0.html");
            }
        }
        Item { height: 35; width: 1 }
        RalewayRegular {
            color: "white"
            text: "In memoriam,"
            size: 14
        }
        RalewayRegular {
            color: "white"
            text: "2012 - 2019 the High Fidelity virtual reality project."
            size: 14
        }
        Item { height: 5; width: 1 }
        Image {
            id: hifiLogo
            width: 200; height: 50
            fillMode: Image.PreserveAspectFit
            source: "../../../images/about-highfidelity.png"
        }
    }
}
