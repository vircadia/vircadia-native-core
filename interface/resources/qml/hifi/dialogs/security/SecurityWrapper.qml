//
//  SecurityWrapper.qml
//  qml\hifi\dialogs\security
//
//  SecurityWrapper
//
//  Created by Zach Fox on 2018-10-31
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import "qrc:////qml//windows"
import "./"

ScrollingWindow {
    id: root;

    resizable: true;
    destroyOnHidden: true;
    width: 400;
    height: 577;
    minSize: Qt.vector2d(400, 500);

    Security { id: security; width: root.width }

    objectName: "SecurityDialog";
    title: security.title;
}
