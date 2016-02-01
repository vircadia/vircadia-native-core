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

    var webEventReceived = function(data) {
        var data = JSON.parse(data);
        if (data.messageType === "settings_update") {
            Entities.editEntity(that.activeParticleEntity, data.updatedSettings);
        }
        print("EBL WEB EVENT RECIEVED FROM PARTICLE GUI");
    }
    webView.eventBridge.webEventReceived.connect(webEventReceived);

    that.setActiveParticleEntity = function(id) {
        that.activeParticleEntity = id;
    }

    that.setVisible = function(newVisible) {
        visible = newVisible;
        webView.setVisible(visible);
    }

    return that;


};