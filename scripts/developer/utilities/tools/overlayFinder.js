function mousePressEvent(event) {
    var overlay = Overlays.getOverlayAtPoint({
        x: event.x,
        y: event.y
    });
    // var pickRay = Camera.computePickRay(event.x, event.y);
    // var intersection =  Overlays.findRayIntersection(pickRay);
    // print('intersection is: ' + intersection)
};

Controller.mousePressEvent.connect(function(event) {
    mousePressEvent(event)
});

Script.scriptEnding.connect(function() {
    Controller.mousePressEvent.disconnect(mousePressEvent);
})