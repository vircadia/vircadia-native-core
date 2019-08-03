

    (function() {
        var TABLET_BUTTON_NAME = "Profiler";
        var QMLAPP_URL = Script.resolvePath("./engineProfiler.qml");
        var ICON_URL = Script.resolvePath("../../../system/assets/images/luci-i.svg");
        var ACTIVE_ICON_URL = Script.resolvePath("../../../system/assets/images/luci-a.svg");

        var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
        var button = tablet.addButton({
            text: TABLET_BUTTON_NAME,
            icon: ICON_URL,
            activeIcon: ACTIVE_ICON_URL
        });

        Script.scriptEnding.connect(function () {
            killWindow()
            button.clicked.disconnect(onClicked);
            tablet.removeButton(button);
        });

        button.clicked.connect(onClicked);
         
        var onScreen = false;
        var window;

        function onClicked() {
            if (onScreen) {
                killWindow()
            } else {
                createWindow()
            }
        }

        function createWindow() {
            var qml = Script.resolvePath(QMLAPP_URL);
            window = Desktop.createWindow(Script.resolvePath(QMLAPP_URL), {
                title: 'Render Engine Profiler',
                additionalFlags: Desktop.ALWAYS_ON_TOP,
                presentationMode: Desktop.PresentationMode.NATIVE,
                size: {x: 500, y: 100}
            });
            window.setPosition(200, 50);
            window.closed.connect(killWindow);
            onScreen = true
            button.editProperties({isActive: true});
        }

        function killWindow() {
            if (window !==  undefined) { 
                window.closed.disconnect(killWindow);
                window.close()
                window = undefined
            }
            onScreen = false
            button.editProperties({isActive: false})
        }
    }()); 
    