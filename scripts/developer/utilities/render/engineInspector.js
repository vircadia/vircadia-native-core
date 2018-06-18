    function openEngineTaskView() {
    // Set up the qml ui
        var qml = Script.resolvePath('engineInspector.qml');
        var window = new OverlayWindow({
            title: 'Render Engine',
            source: qml,
            width: 500, 
            height: 100
        });
        window.setPosition(200, 50);
        window.closed.connect(function() { Script.stop(); });
    }
    openEngineTaskView();

    