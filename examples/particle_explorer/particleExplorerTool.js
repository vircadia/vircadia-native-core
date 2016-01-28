var PARTICLE_EXPLORER_HTML_URL = Script.resolvePath('particleExplorer.html');

ParticleExplorerTool = function() {
    var that = {};

    var url = PARTICLE_EXPLORER_HTML_URL;
    var webView = new OverlayWebWindow({
        title: 'Particle Explorer',  source: url,  toolWindow: true   
    });

    var visible = false;
    webView.setVisible(visible);

    that.setVisible = function(newVisible) {
        visible = newVisible;
        webView.setVisible(visible);
    }


};
