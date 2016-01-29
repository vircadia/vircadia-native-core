var PARTICLE_EXPLORER_HTML_URL = Script.resolvePath('particleExplorer.html?v1' + Math.random());

ParticleExplorerTool = function() {
    var that = {};

    var url = PARTICLE_EXPLORER_HTML_URL;
    var webView = new OverlayWebWindow({
        title: 'Particle Explorer',  source: url,  toolWindow: true   
    });

    var visible = false;
    webView.setVisible(visible);

    webView.eventBridge.webEventReceived.connect(function(data) {

        print("EBL GOT AN EVENT!");
    });

    that.setVisible = function(newVisible) {
        visible = newVisible;
        webView.setVisible(visible);
    }

    that.getWebView = function() {
        return webView;
    }


};
