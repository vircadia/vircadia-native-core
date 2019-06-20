(function() { // BEGIN LOCAL_SCOPE

    console.debug('controlsGallery: creating window')

    var qml = Script.resolvePath('ControlsGallery.qml');
    var qmlWindow = new OverlayWindow({
                                      title: 'Hifi Controls Gallery',
                                      source: qml,
                                      height: 480,
                                      width: 640,
                                      visible: true
                                  });

    console.debug('controlsGallery: creating window... done')

    qmlWindow.closed.connect(function() { Script.stop(); });

    Script.scriptEnding.connect(function() {
        console.debug('controlsGallery: end of scripting')
        delete qmlWindow;
    });

}()); // END LOCAL_SCOPE
