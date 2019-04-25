"use strict";

(function() {

    var appUi = Script.require("appUi");
    var ui;

    var AVATAR_THEFT_BANNER_IMAGE = Script.resourcesPath() + "images/AvatarTheftBanner.png";
    var AVATAR_THEFT_SETTINGS_QML = Script.resourcesPath() + "qml/AvatarTheftSettings.qml";
    var button;
    var theftBanner = null;
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var onAvatarBanner = false;
    var THEFT_BANNER_DIMENSIONS = {x: 1.0, y: 1.0, z: 0.3};

    function createEntities() {
        //if (HMD.active) {
        var render = tablet.tabletShown ? "world" : "front";
        var pos = tablet.tabletShown ? { x: 0.0, y: 0.0, z: -1.8 } : { x: 0.0, y: 0.0, z: -0.7 };
        var dimensionMultiplier = tablet.tabletShown ? 2.6 : 1;
        var props = {
            type: "Image",
            imageURL: AVATAR_THEFT_BANNER_IMAGE,
            name: "hifi-avatar-theft-banner",
            parentID: MyAvatar.SELF_ID,
            parentJointIndex: MyAvatar.getJointIndex("_CAMERA_MATRIX"),
            localPosition: pos,
            dimensions: {x: THEFT_BANNER_DIMENSIONS.x * dimensionMultiplier, y: THEFT_BANNER_DIMENSIONS.y * dimensionMultiplier},
            renderLayer: render,
            userData: {
                grabbable: false
            },
            visible: true
        };
/*        var props = {*/
            //type: "Web",
            //name: "hifi-avatar-theft-banner",
            //sourceUrl: AVATAR_THEFT_BANNER_QML,
            //parentID: MyAvatar.SELF_ID,
            //parentJointIndex: MyAvatar.getJointIndex("_CAMERA_MATRIX"),
            //localPosition: { x: 0.0, y: 0.0, z: -1.0 },
            //dimensions: THEFT_BANNER_DIMENSIONS,
            //drawInFront: true,
            //userData: {
                //grabbable: false
            //},
            //visible: true
        /*};*/
        if (theftBanner) {
            Entities.deleteEntity(theftBanner);
        }
        theftBanner = Entities.addEntity(props, "local");
        Window.copyToClipboard(theftBanner);
        console.log("created entity");
        //} else {
        //}
    }

    function fromQml(message) {
        if (message.method === "reposition") {
            var theftBannerLocalPosition = Entities.getEntityProperties(theftBanner).localPosition;
            var newTheftBannerLocalPosition;
            if (message.x !== undefined) {
                newTheftBannerLocalPosition = { x: -((THEFT_BANNER_DIMENSIONS.x) / 2) + message.x, y: theftBannerLocalPosition.y, z: theftBannerLocalPosition.z };
            } else if (message.y !== undefined) {
                newtheftBannerLocalPosition = { x: theftBannerLocalPosition.x, y: message.y, z: theftBannerLocalPosition.z };
            } else if (message.z !== undefined) {
                newtheftBannerLocalPosition = { x: theftBannerLocalPosition.x, y: theftBannerLocalPosition.y, z: message.z };
            }
            var theftBannerProps = {
                localPosition: newtheftBannerLocalPosition
            };

            Entities.editEntity(theftBanner, theftBannerProps);
        } else if (message.method === "setVisible") {
            if (message.visible !== undefined) {
                var props = {
                    visible: message.visible
                };
                Entities.editEntity(theftBannerEntity, props);
            }
        } else if (message.method === "print") {
            // prints the local position into the hifi log.
            var theftBannerLocalPosition = Entities.getEntityProperties(theftBannerEntity).localPosition;
            console.log("theft banner local position is at " + JSON.stringify(theftBannerLocalPosition));
        }
    };

    var cleanup = function () {
        if (theftBanner) {
            Entities.deleteEntity(theftBanner);
        }
    };

    function setup() {
        ui = new appUi({
            buttonName: "THEFT",
            home:  AVATAR_THEFT_SETTINGS_QML,
            onMessage: fromQml,
            onOpened: createEntities,
            onClosed: cleanup,
            normalButton: "icons/tablet-icons/edit-i.svg",
            activeButton: "icons/tablet-icons/edit-a.svg",
        });
    };

    setup();

    Entities.mousePressOnEntity.connect(function (entityID, event) {
        if (entityID === theftBanner && theftBanner){
            tablet.loadQMLSource(Script.resourcesPath() + "qml/hifi/AvatarApp.qml");
        }
    })

    tablet.isTabletShownChanged.connect(function () {
        if (theftBanner) {
            if (tablet.tabletShown) {
                Entities.editEntity(theftBanner, {
                    dimensions: THEFT_BANNER_DIMENSIONS,
                    localPosition: { x: 0.0, y: 0.0, z: -0.7 },
                    renderLayer: "world"
                });
            } else {
                Entities.editEntity(theftBanner, {
                    localPosition: { x: 0.0, y: 0.0, z: -1.8 },
                    dimensions: {x: THEFT_BANNER_DIMENSIONS.x * 2.6, y: THEFT_BANNER_DIMENSIONS.y * 2.6},
                    renderLayer: "front"
                });
            }
            console.log(Entities.getEntityProperties(theftBanner).renderLayer);
        }
    })

    Script.scriptEnding.connect(cleanup);

    location.hostChanged.connect(function (){
        if (theftBanner) {
            Entities.editEntity(theftBanner, {
                visible: false 
            });
        }
    })

}());
