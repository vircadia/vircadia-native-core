//
//  TabletConnectionFailureDialog.qml
//
//  Created by Vlad Stelmahovsky on 29 Mar 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.5
import QtQuick.Dialogs 1.2 as OriginalDialogs

Item {
    Component.onCompleted: {
        var object = tabletRoot.messageBox({
                                            icon: OriginalDialogs.StandardIcon.Warning,
                                            buttons: OriginalDialogs.StandardButton.Ok,
                                            defaultButton: OriginalDialogs.StandardButton.NoButton,
                                            title: "No Connection",
                                            text: "Unable to connect to this domain. Click the 'EXPLORE' button on the toolbar to visit another domain."
                                        });
        object.selected.connect(function(button) {
            if (button === OriginalDialogs.StandardButton.Ok) {
                var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
                tablet.gotoHomeScreen()
            }
        });
    }
}
