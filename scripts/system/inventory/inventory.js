(function () { // BEGIN LOCAL_SCOPE
var AppUi = Script.require('appUi');
var ui;
var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");

// APP EVENT ROUTING

function onWebAppEventReceived(event) {
    var eventJSON = JSON.parse(event);
    if (eventJSON.app == "inventory") { // This is our web app!
        print("gemstoneApp.js received a web event: " + event);
    }
}

tablet.webEventReceived.connect(onWebAppEventReceived);

// END APP EVENT ROUTING

function saveInventory() {
    
}

function onOpened() {
    console.log("hello world!");
}

function onClosed() {
    console.log("hello world!");
}

function startup() {
    ui = new AppUi({
        buttonName: "INVENTORY",
        home: Script.resolvePath("inventory.html"),
        graphicsDirectory: Script.resolvePath("./"), // Where your button icons are located
        onOpened: onOpened,
        onClosed: onClosed
    });
}
startup();
}()); // END LOCAL_SCOPE