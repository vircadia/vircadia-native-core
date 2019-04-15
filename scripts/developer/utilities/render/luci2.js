function openView() {
    // Set up the qml ui
    var window = Desktop.createWindow(Script.resolvePath('luci.qml'), {
        title: this.title,
        presentationMode: Desktop.PresentationMode.NATIVE,
        size: {x: 300, y: 400}
    });
    //window.closed.connect(function() { Script.stop(); });
}
openView();