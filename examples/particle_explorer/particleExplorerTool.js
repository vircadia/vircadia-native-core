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

        that.webView.setVisible(true);
        that.webView.eventBridge.webEventReceived.connect(that.webEventReceived);        
    }


    that.destroyWebView = function() {
        print("EBL DESTROY WEB VIEW" + that.webView);
        that.webView.close();
        that.webView = null;
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


    return that;


};