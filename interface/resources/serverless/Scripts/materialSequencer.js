// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
// 2020 Silverfish.
//material sequencer, put on a material entity (server script) to make it iterate through discrete steps of X UV offset.

/*UserData:
{
  "verticalOffset": 0,
  "segments": 16,
  "updateInterval": 250
}
*/

(function() {
    var DEFAULT_VERTICAL_OFFSET = 0.0;
    var DEFAULT_HORIZONTAL_SPEED = 1;
    var DEFAULT_UPDATE_INTERVAL = 1000;
    var DEFAULT_SEGMENTS = 16;

    var self = this;
    var _entityID;
    this.preload = function(entityID) {
        _entityID = entityID;
        var verticalOffset = DEFAULT_VERTICAL_OFFSET;
        var updateInterval = DEFAULT_UPDATE_INTERVAL;
        var moveDistance = 1 / DEFAULT_SEGMENTS;
        var verticalOffset = DEFAULT_VERTICAL_OFFSET;
        var oldPosition = 0;
        var currentPosition = 0;
        var userData = JSON.parse(Entities.getEntityProperties(_entityID, ["userData"]).userData);
        if (userData !== undefined) {
            if (userData.segments !== undefined) {
                moveDistance = 1 / userData.segments;
            }
            if (userData.verticalOffset !== undefined) {
                verticalOffset = userData.verticalOffset;
            }
            if (userData.updateInterval !== undefined) {
                updateInterval = userData.updateInterval;
            }
        }

        self.intervalID = Script.setInterval(function() {
            if(currentPosition >= 1){
                currentPosition = 0;
                oldPosition = 0;
                }
            
            Entities.editEntity(_entityID, { materialMappingPos: { x: currentPosition, y: verticalOffset}});
           // print("current Pos: " + currentPosition);
            currentPosition = oldPosition + moveDistance;
            oldPosition = currentPosition;
        }, updateInterval);
    };

    this.unload = function() {
        Script.clearInterval(self.intervalID);
    }
});