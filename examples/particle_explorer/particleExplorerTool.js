var PARTICLE_EXPLORER_HTML_URL = Script.resolvePath('particleExplorer.html');

ParticleExplorerTool = function() {
    var that = {};

    var url = PARTICLE_EXPLORER_HTML_URL;
    var webView = new OverlayWebWindow({
        title: 'Particle Explorer',
        source: url,
        toolWindow: true
    });

    var visible = false;
    webView.setVisible(visible);
    that.webView = webView;

    var webEventReceived = function() {
        print("EBL WEB EVENT RECIEVED FROM PARTICLE GUI");
    }

    webView.eventBridge.webEventReceived.connect(webEventReceived);


    that.setVisible = function(newVisible) {
        visible = newVisible;
        webView.setVisible(visible);
    }

    return that;


};