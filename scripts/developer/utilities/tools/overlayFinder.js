function mousePressEvent(event) {
    var overlay = Overlays.getOverlayAtPoint({x:event.x,y:event.y});
    print('overlay is: ' + overlay)
   //      var pickRay = Camera.computePickRay(event.x, event.y);

   // var intersection =  Overlays.findRayIntersection(pickRay);
   // print('intersection is: ' + intersection)
};

Controller.mouseMoveEvent.connect(function(event){
    print('mouse press')
    mousePressEvent(event)
});

Script.scriptEnding.connect(function(){
    Controller.mousePressEvent.disconnect(mousePressEvent);
})