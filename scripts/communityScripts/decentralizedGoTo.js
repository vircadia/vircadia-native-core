(function () {
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    Menu.menuItemEvent.connect(onMenuItemEvent);
    var AppUi = Script.require('appUi');
    var goToAddresses;
    var permission;
    Menu.addMenu("GoTo");
    Menu.addMenuItem("GoTo", "Subscribe to new GoTo provider");
    Menu.addMenu("GoTo > Unsubscribe from GoTo provider");
    var goToAddress = Settings.getValue("goToDecentral", "");
    for (var i = 0; i < goToAddress.length; i++) {
        Menu.addMenuItem("GoTo > Unsubscribe from GoTo provider", goToAddress[i]);
    }
    var ui;
    function startup() {
        goToAddress = Settings.getValue("goToDecentral", "");
        if (goToAddress == "") {
            var initialGoToList = Script.resolvePath("goto.json");
            Menu.addMenuItem("GoTo > Unsubscribe from GoTo provider", initialGoToList);
            goToAddressNow = [
                initialGoToList
            ];
            Settings.setValue("goToDecentral", goToAddressNow);
        }
        ui = new AppUi({
            buttonName: "Find",
            home: Script.resolvePath("decentralizedGoTo.html"),
            icon: Script.resolvePath("goto-a.svg"),
            activeIcon: Script.resolvePath("goto-a-msg.svg")
        });
    }

    function onWebEventReceived(event) {
        messageData = JSON.parse(event);
        if (messageData.action == "requestAddressList") {
            goToAddresses = Settings.getValue("goToDecentral", "");
            for (var i = 0; i < goToAddresses.length; i++) {

                try {
                    Script.require(goToAddresses[i] + "?" + Date.now());
                }
                catch (e) {
                    goToAddresses.remove(goToAddresses[i]);
                }
            }
            var children = goToAddresses
                .map(function (url) { return Script.require(url + '?' + Date.now()); })
                .reduce(function (result, someChildren) { return result.concat(someChildren); }, []);

            for (var z = 0; z < children.length; z++) {
                delete children[z].age;
                delete children[z].id;
                children[z]["Domain Name"] = children[z]["Domain Name"].replace(/</g, '&lt;').replace(/>/g, '&lt;');
                children[z].Owner = children[z].Owner.replace(/</g, '&lt;').replace(/>/g, '&lt;');
            }

            permission = Entities.canRez()

            var readyEvent = {
                "action": "addressList",
                "myAddress": children,
                "permission": permission
            };

            tablet.emitScriptEvent(JSON.stringify(readyEvent));

        } else if (messageData.action == "goToUrl") {
            Window.location = messageData.visit;
        } else if (messageData.action == "addLocation") {

            var messageDataDomainInformation = {
                owner: messageData.owner,
                domainName: messageData.domainName,
                port: messageData.Port
            };

            locationboxID = Entities.addEntity({
                position: Vec3.sum(MyAvatar.position, Quat.getFront(MyAvatar.orientation)),
                userData: JSON.stringify(messageDataDomainInformation),
                serverScripts: messageData.script,
                color: { red: 255, green: 0, blue: 0 },
                type: "Box"
            });
        } else if (messageData.action == "retrievePortInformation") {
            var readyEvent = {
                "action": "retrievePortInformationResponse",
                "goToAddresses": goToAddresses
            };

            tablet.emitScriptEvent(JSON.stringify(readyEvent));
            
        }
    }

    function onMenuItemEvent(menuItem) {
        if (menuItem == "Subscribe to new GoTo provider") {
            goToAddress = Settings.getValue("goToDecentral", "");
            var arrayLength = goToAddress.length;
            var prom = Window.prompt("Enter URL to GoTo JSON.", "");
            if (prom) {
                goToAddress[arrayLength] = prom;
                Settings.setValue("goToDecentral", goToAddress);
                Menu.addMenuItem("GoTo > Unsubscribe from GoTo provider", prom);
            }
        } else {
            goToAddresses = Settings.getValue("goToDecentral", "");
            Menu.removeMenuItem("GoTo > Unsubscribe from GoTo provider", menuItem);
            goToAddresses.remove(menuItem);
            Settings.setValue("goToDecentral", goToAddresses);
        }
    }

    Array.prototype.remove = function () {
        var what, a = arguments, L = a.length, ax;
        while (L && this.length) {
            what = a[--L];
            while ((ax = this.indexOf(what)) !== -1) {
                this.splice(ax, 1);
            }
        }
        return this;
    };

    tablet.webEventReceived.connect(onWebEventReceived);
    startup();

    Script.scriptEnding.connect(function () {
        Messages.unsubscribe("goTo");
        Menu.removeMenu("GoTo");
        tablet.webEventReceived.disconnect(onWebEventReceived);
        Menu.menuItemEvent.disconnect(onMenuItemEvent);
    });
}());
