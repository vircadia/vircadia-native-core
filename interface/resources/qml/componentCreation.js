var component;
var instance;
var parent;

function createObject(parentObject, url) {
	parent = parentObject;
    component = Qt.createComponent(url);
    if (component.status == Component.Ready)
        finishCreation();
    else
        component.statusChanged.connect(finishCreation);
}

function finishCreation() {
    if (component.status == Component.Ready) {
    	instance = component.createObject(parent, {"x": 100, "y": 100});
        if (instance == null) {
            // Error Handling
            console.log("Error creating object");
        } else {
        	instance.enabled = true
        }
    } else if (component.status == Component.Error) {
        // Error Handling
        console.log("Error loading component:", component.errorString());
    } else {
    	console.log("Unknown component status: " + component.status);
    }
}