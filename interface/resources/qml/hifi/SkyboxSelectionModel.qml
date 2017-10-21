//
//  SkyboxSelectionModel.qml
//  qml/hifi
//
//  Created by Cain Kilgore on 21st October 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5

ListModel {
    id: root;
    ListElement{
        thumbnailPath: "http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/thumbnails/thumb_1.jpg"
        fullSkyboxPath: "http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/skyboxes/1.jpg"
        skyboxEnumValue: 1;
    }
    ListElement{
        thumbnailPath: "http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/thumbnails/thumb_1.jpg"
        fullSkyboxPath: "http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/skyboxes/1.jpg"
        skyboxEnumValue: 2;
    }
    ListElement{
        thumbnailPath: "http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/thumbnails/thumb_1.jpg"
        fullSkyboxPath: "http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/skyboxes/1.jpg"
        skyboxEnumValue: 3;
    }
    ListElement{
        thumbnailPath: "http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/thumbnails/thumb_1.jpg"
        fullSkyboxPath: "http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/skyboxes/1.jpg"
        skyboxEnumValue: 4;
    }
    ListElement{
        thumbnailPath: "http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/thumbnails/thumb_1.jpg"
        fullSkyboxPath: "http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/skyboxes/1.jpg"
        skyboxEnumValue: 5;
    }
    ListElement{
        thumbnailPath: "http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/thumbnails/thumb_1.jpg"
        fullSkyboxPath: "http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/skyboxes/1.jpg"
        skyboxEnumValue: 6;
    }

    function getImagePathFromImageID(imageID) {
        return (imageID ? root.get(imageID - 1).fullSkyboxPath : "");
    }
}