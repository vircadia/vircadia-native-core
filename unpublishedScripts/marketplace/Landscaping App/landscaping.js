(function () {
    // Every great app starts with a great name (keep it short so that it can fit in the tablet button)
    var APP_NAME = "LANDSCAPE";
    // Link to your app's HTML file
    var APP_URL = "https://rebeccastankus.github.io/";
    // Path to the icon art for the app. As of now I can only make it work using a relative path as 
    //linking to the URL on my asset server is not loading the svg file
    var APP_ICON = "https://s3-us-west-1.amazonaws.com/rebeccahifi/landscapeIcon.svg";

    // Get a reference to the tablet 
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");

    // "Install" the app to the tablet
    // The following lines create a button on the tablet's menu screen
    var button = tablet.addButton({
        icon: APP_ICON,
        text: APP_NAME
    });

    // When user click the app button, display the app on the tablet screen
    function onClicked() {
        tablet.gotoWebScreen(APP_URL);
        print("landscape opened in tablet");
    }
    button.clicked.connect(onClicked);

    // Handle the events we're receiving from the web UI
    function onWebEventReceived(event) {
        print("landscape.js received a web event: " + event);
    }
    tablet.webEventReceived.connect(onWebEventReceived);
    //all entities are coming in at varying y coordinates, so for now each has its own position function
    //get position in front of avatar for dahlia
    function getPositionDahlia() {
        var direction = Quat.getFront(MyAvatar.orientation);
        var distance = 1;
        var position = Vec3.sum(MyAvatar.position, Vec3.multiply(direction, distance));
        position.y -= 0.4;
        return position;
    }
    //get position in front of avatar for cycas
    function getPositionCycas() {
        var direction = Quat.getFront(MyAvatar.orientation);
        var distance = 1;
        var position = Vec3.sum(MyAvatar.position, Vec3.multiply(direction, distance));
        position.y -= 0.1;
        return position;
    }
    //get position in front of avatar for grass
    function getPositionGrass() {
        var direction = Quat.getFront(MyAvatar.orientation);
        var distance = 1;
        var position = Vec3.sum(MyAvatar.position, Vec3.multiply(direction, distance));
        position.y -= 0.48;
        return position;
    }

    //get position in front of avatar for tupelo tree
    function getPositionTupelo() {
        var direction = Quat.getFront(MyAvatar.orientation);
        var distance = 1;
        var position = Vec3.sum(MyAvatar.position, Vec3.multiply(direction, distance));
        position.y += 4;
        return position;
    }

    // Handle the events we're recieving from the web UI
    function onWebEventReceived(event) {
        print("landscaping.js received a web event:" + event);

        // Converts the event to a JavasScript Object
        if (typeof event === "string") {
            event = JSON.parse(event);
        }

        if (event.type === "click") {

            if (event.data === "Tupelo") {
                
                // references to our assets
                var MODEL_URL = "https://s3-us-west-1.amazonaws.com/rebeccahifi/tupelo.fbx";

                // An entity is described and created by specifying a map of properties
                var tupelo = Entities.addEntity({
                    type: "Model",
                    modelURL: MODEL_URL,
                    name: "tupelo",
                    "position": getPositionTupelo(),
                "userData": "{\"grabbableKey\":{\"grabbable\":true}}",
                    dimensions: {
                        x: 6,
                        y: 9,
                        z: 6
                    },
                    dynamic: false,
                    lifetime: -1,
                    collisionless: true,
                    shapeType: "box"
                });

            } else if (event.data === "Cycas") {

                // references to our assets
                var MODEL_URL = "https://s3-us-west-1.amazonaws.com/rebeccahifi/cycas.fbx";

                // An entity is described and created by specifying a map of properties
                var tupelo = Entities.addEntity({
                    type: "Model",
                    modelURL: MODEL_URL,
                    name: "cycas",
                    "position": getPositionCycas(),
                    "userData": "{\"grabbableKey\":{\"grabbable\":true}}",
                    dimensions: {
                        x: 1.5,
                        y: 1,
                        z: 1.5
                    },
                    dynamic: false,
                    lifetime: -1,
                    collisionless: true,
                    shapeType: "box"
                });

            } else if (event.data === "Dahlia") {

                // references to our assets
                var MODEL_URL = "https://s3-us-west-1.amazonaws.com/rebeccahifi/dahlia.fbx";

                // An entity is described and created by specifying a map of properties
                var tupelo = Entities.addEntity({
                    type: "Model",
                    modelURL: MODEL_URL,
                    name: "dahlia",
                    "position": getPositionDahlia(),
                    "userData": "{\"grabbableKey\":{\"grabbable\":true}}",
                    dimensions: {
                        x: .2,
                        y: .4,
                        z: .2
                    },
                    dynamic: false,
                    lifetime: -1,
                    collisionless: true,
                    shapeType: "box"
                });

            } else if (event.data === "Grass") {

                // references to our assets
                var MODEL_URL = "https://s3-us-west-1.amazonaws.com/rebeccahifi/grass.fbx";

                // An entity is described and created by specifying a map of properties
                var tupelo = Entities.addEntity({
                    type: "Model",
                    modelURL: MODEL_URL,
                    name: "grass",
                    "position": getPositionGrass(),
                    "userData": "{\"grabbableKey\":{\"grabbable\":true}}",
                    dimensions: {
                        x: .2,
                        y: .1,
                        z: .2
                    },
                    dynamic: false,
                    lifetime: -1,
                    collisionless: true,
                    shapeType: "box"
                });

            }
        }
    }

    // Provide a way to "uninstall" the app
    // Here, we write a function called "cleanup" which gets executed when
    // this script stops running. It'll remove the app button from the tablet.
    function cleanup() {
        tablet.removeButton(button);
    }
    Script.scriptEnding.connect(cleanup);
}());