var PARTICLE_EXPLORER_HTML_URL = Script.resolvePath('particleExplorer.html');

ParticleExplorerTool = function() {
    var that = {};

    that.createWebView = function() {
        var url = PARTICLE_EXPLORER_HTML_URL;
        that.webView = new OverlayWebWindow({
            title: 'Particle Explorer',
            source: url,
            toolWindow: true
        });

        var visible = false;
        that.webView.setVisible(visible);
        that.webView.eventBridge.webEventReceived.connect(that.webEventReceived);        
    }

    that.webEventReceived = function(data) {
        var data = JSON.parse(data);
        if (data.messageType === "settings_update") {
            Entities.editEntity(that.activeParticleEntity, data.updatedSettings);
        }
        print("EBL WEB EVENT RECIEVED FROM PARTICLE GUI");
    }

    that.setActiveParticleEntity = function(id) {
        that.activeParticleEntity = id;
    }

    that.setVisible = function(newVisible) {
        visible = newVisible;
        that.webView.setVisible(visible);
    }

    return that;


};