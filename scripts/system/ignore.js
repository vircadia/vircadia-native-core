//
//  ignore.js
//  scripts/system/
//
//  Created by Stephen Birarda on 07/11/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// grab the toolbar
var toolbar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");

// setup the ignore button and add it to the toolbar
var button = toolbar.addButton({
    objectName: 'ignore',
    imageURL: Script.resolvePath("assets/images/tools/mic.svg"),
    visible: true,
    buttonState: 1,
    alpha: 0.9
});

var isShowingOverlays = false;

// handle clicks on the toolbar button
function buttonClicked(){
    if (isShowingOverlays) {
        hideOverlays();
    } else {
        showOverlays();
    }
}

button.clicked.connect(buttonClicked);

// remove the toolbar button when script is stopped
Script.scriptEnding.connect(function() {
    toolbar.removeButton('ignore');
});
