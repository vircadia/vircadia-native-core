    function openEngineTaskView() {
    // Set up the qml ui
        var qml = Script.resolvePath('engineList.qml');
        var window = new OverlayWindow({
            title: 'Render Engine',
            source: qml,
            width: 300, 
            height: 400
        });
        window.setPosition(200, 50);
        //window.closed.connect(function() { Script.stop(); });
    }
    openEngineTaskView();