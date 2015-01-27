// VR menu prototype
// David Rowe

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

var vrChat = (function () {

    var background,
        CHAT_YAW = -40.0,  // Degrees
        CHAT_DISTANCE = 0.6,
        CHAT_HEIGHT = 0.35;

    function setUp() {
        background = Overlays.addOverlay("rectangle3d", {
            color: { red: 200, green: 200, blue: 200 },
            alpha: 0.5,
            solid: true,
            visible: false,
            dimensions: { width: 0.3, height: 0.5 },
            ignoreRayIntersection: true,
            isFacingAvatar: false
        });
    }

    function show(on) {
        Overlays.editOverlay(background, { visible: on });
    }

    function update() {
        var CHAT_OFFSET = { x: 0.0, y: CHAT_HEIGHT, z: -CHAT_DISTANCE },
            chatRotation,
            chatOffset;

        chatRotation = Quat.multiply(Quat.fromPitchYawRollDegrees(0.0, CHAT_YAW, 0.0), MyAvatar.orientation);
        chatOffset = Vec3.multiplyQbyV(chatRotation, CHAT_OFFSET);
        chatRotation = Quat.multiply(chatRotation, Quat.fromPitchYawRollDegrees(90.0, 0.0, 0.0));

        Overlays.editOverlay(background, {
            position: Vec3.sum(MyAvatar.position, chatOffset),
            rotation: chatRotation
        });
    }

    function tearDown() {
        Overlays.deleteOverlay(background);
    }

    return {
        setUp: setUp,
        show: show,
        update: update,
        tearDown: tearDown
    };

}());


var vrMenu = (function () {

    var menuItems = [],
        menuVisible = false,
        MENU_HEIGHT = 0.7,
        MENU_RADIUS = 0.6,
        ITEM_SPACING = 12.0,  // Degrees
        IMAGE_WIDTH = 160,
        IMAGE_HEIGHT = 100,
        IMAGE_SCALE = 0.1,
        NUMBER_OF_BUTTONS = 10;

    function setVisible(visible) {
        var i;

        menuVisible = visible;

        for (i = 0; i < menuItems.length; i += 1) {
            Overlays.editOverlay(menuItems[i].overlay, { visible: menuVisible });
        }
    }

    function keyPressEvent(event) {
        if (event.text.toLowerCase() === "o") {
            setVisible(!menuVisible);
        }
    }

    function mousePressEvent(event) {
        var pickRay,
            intersection,
            subImage = { x: 0, y: IMAGE_HEIGHT, width: IMAGE_WIDTH, height: IMAGE_HEIGHT },
            i;

        pickRay = Camera.computePickRay(event.x, event.y);
        intersection = Overlays.findRayIntersection(pickRay);

        if (intersection.intersects) {
            for (i = 0; i < menuItems.length; i += 1) {
                if (intersection.overlayID === menuItems[i].overlay) {
                    menuItems[i].on = !menuItems[i].on;
                    subImage.y = (menuItems[i].on ? 0 : 1) * IMAGE_HEIGHT;
                    Overlays.editOverlay(menuItems[i].overlay, { subImage: subImage });
                    if (menuItems[i].callback) {
                        menuItems[i].callback(menuItems[i].on);
                    }
                }
            }
        }
    }

    function setUp() {
        var overlay,
            menuItem,
            i;

        for (i = 0; i < NUMBER_OF_BUTTONS; i += 1) {
            overlay = Overlays.addOverlay("billboard", {
                url: "http://ctrlaltstudio.com/hifi/menu-blank.svg",
                subImage: { x: 0, y: IMAGE_HEIGHT, width: IMAGE_WIDTH, height: IMAGE_HEIGHT },
                alpha: 1.0,
                visible: false,
                scale: IMAGE_SCALE,
                isFacingAvatar: false
            });

            menuItem = {
                overlay: overlay,
                on: false,
                callback: null
            };

            menuItems.push(menuItem);
        }

        Overlays.editOverlay(menuItems[NUMBER_OF_BUTTONS - 2].overlay, {
            url: "http://ctrlaltstudio.com/hifi/menu-chat.svg"
        });
        menuItems[NUMBER_OF_BUTTONS - 2].callback = vrChat.show;

        Controller.keyPressEvent.connect(keyPressEvent);
        Controller.mousePressEvent.connect(mousePressEvent);
    }

    function update() {
        var MENU_OFFSET = { x: 0.0, y: MENU_HEIGHT, z: -MENU_RADIUS },  // Offset from avatar position.
            itemAngle,
            itemRotation,
            itemOffset,
            i;

        itemAngle = menuItems.length * ITEM_SPACING / 2.0;

        for (i = 0; i < menuItems.length; i += 1) {

            itemRotation = Quat.multiply(Quat.fromPitchYawRollDegrees(0.0, itemAngle, 0.0), MyAvatar.orientation);
            itemOffset = Vec3.multiplyQbyV(itemRotation, MENU_OFFSET);

            Overlays.editOverlay(menuItems[i].overlay, {
                position: Vec3.sum(MyAvatar.position, itemOffset),
                rotation: itemRotation
            });

            itemAngle -= ITEM_SPACING;
        }
    }

    function tearDown() {
        var i;

        for (i = 0; i < menuItems.length; i += 1) {
            Overlays.deleteOverlay(menuItems[i].overlay);
        }
    }

    return {
        setUp: setUp,
        update: update,
        tearDown: tearDown
    };

}());

vrChat.setUp();
Script.update.connect(vrChat.update);
Script.scriptEnding.connect(vrChat.tearDown);

vrMenu.setUp();
Script.update.connect(vrMenu.update);
Script.scriptEnding.connect(vrMenu.tearDown);