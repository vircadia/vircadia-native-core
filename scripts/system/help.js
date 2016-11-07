"use strict";

//
//  help.js
//  scripts/system/
//
//  Created by Howard Stearns on 2 Nov 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE

    var toolBar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");
    var buttonName = "help"; // matching location reserved in Desktop.qml
    var button = toolBar.addButton({
        objectName: buttonName,
        imageURL: Script.resolvePath("assets/images/tools/help.svg"),
        visible: true,
        hoverState: 2,
        defaultState: 1,
        buttonState: 1,
        alpha: 0.9
    });

    // TODO: make button state reflect whether the window is opened or closed (independently from us).
    
    function onClicked(){
        Menu.triggerOption('Help...')
    }

    button.clicked.connect(onClicked);

    Script.scriptEnding.connect(function () {
        toolBar.removeButton(buttonName);
        button.clicked.disconnect(onClicked);
    });

}()); // END LOCAL_SCOPE
