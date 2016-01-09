Script.include([
    "libraries/toolBars.js",
]);

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";
var toolIconUrl = HIFI_PUBLIC_BUCKET + "images/tools/";

var MARKETPLACE_URL = "https://metaverse.highfidelity.com/marketplace";
var marketplaceWindow = new OverlayWebWindow({
    title: 'Marketplace',
    source: "about:blank",
    width: 900,
    height: 700,
    visible: false
});

var toolHeight = 50;
var toolWidth = 50;


function showMarketplace(marketplaceID) {
    var url = MARKETPLACE_URL;
    if (marketplaceID) {
        url = url + "/items/" + marketplaceID;
    }
    print("setting marketplace URL to " + url);
    marketplaceWindow.setURL(url);
    marketplaceWindow.setVisible(true);
    marketplaceWindow.raise();
}

function hideMarketplace() {
    marketplaceWindow.setVisible(false);
    marketplaceWindow.setURL("about:blank");
}

function toggleMarketplace() {
    if (marketplaceWindow.visible) {
        hideMarketplace();
    } else {
        showMarketplace();
    }
}

var toolBar = (function() {
    var that = {},
        toolBar,
        browseMarketplaceButton;

    function initialize() {
        toolBar = new ToolBar(0, 0, ToolBar.VERTICAL, "highfidelity.edit.toolbar", function(windowDimensions, toolbar) {
            return {
                x: windowDimensions.x - 100 - toolbar.width,
                y: (windowDimensions.y - toolbar.height) / 2
            };
        });
        browseMarketplaceButton = toolBar.addTool({
            imageURL: toolIconUrl + "marketplace.svg",
            width: toolWidth,
            height: toolHeight,
            alpha: 0.9,
            visible: true,
        });

        toolBar.showTool(browseMarketplaceButton, true);
    }

    var browseMarketplaceButtonDown = false;
    that.mousePressEvent = function(event) {
        print("CLICKED")
        var clickedOverlay,
            url,
            file;

        if (!event.isLeftButton) {
            // if another mouse button than left is pressed ignore it
            return false;
        }

        clickedOverlay = Overlays.getOverlayAtPoint({
            x: event.x,
            y: event.y
        });



        if (browseMarketplaceButton === toolBar.clicked(clickedOverlay)) {
            toggleMarketplace();
            return true;
        }

        return false;
    };

    that.mouseReleaseEvent = function(event) {
        var handled = false;


        if (browseMarketplaceButtonDown) {
            var clickedOverlay = Overlays.getOverlayAtPoint({
                x: event.x,
                y: event.y
            });
        }

        newModelButtonDown = false;
        browseMarketplaceButtonDown = false;

        return handled;
    }

    that.cleanup = function() {
        toolBar.cleanup();
    };

    initialize();
    return that;
}());

Controller.mousePressEvent.connect(toolBar.mousePressEvent)
Script.scriptEnding.connect(toolBar.cleanup);
