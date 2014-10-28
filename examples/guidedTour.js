//
//  TourGuide.js 
// 
//  This script will follow another person, if their display name is "Tour Guide"
// 
var leaderName = "Tour Guide";

var guide; 
var isGuide = false;
var lastGuidePosition = { x:0, y:0, z:0 };
var MIN_CHANGE = 2.0;
var LANDING_DISTANCE = 2.0;
var LANDING_RANDOM = 0.2;

function update(deltaTime) {

  if (Math.random() < deltaTime) {
    guide = AvatarList.avatarWithDisplayName(leaderName);
    if (guide && !isGuide) {
      print("Found a tour guide!");
      isGuide = true;
    } else if (!guide && isGuide) {
      print("Lost My Guide");
      isguide = false;
    }
  }

  if (guide) {
    //  Check whether guide has moved, update if so
    if (Vec3.length(lastGuidePosition) == 0.0) {
      lastGuidePosition = guide.position;
    } else {
      if (Vec3.length(Vec3.subtract(lastGuidePosition, guide.position)) > MIN_CHANGE) {
        var meToGuide = Vec3.multiply(Vec3.normalize(Vec3.subtract(guide.position, MyAvatar.position)), LANDING_DISTANCE);
        var newPosition = Vec3.subtract(guide.position, meToGuide);
        newPosition = Vec3.sum(newPosition, {   x: Math.random() * LANDING_RANDOM - LANDING_RANDOM / 2.0, 
                                                y: 0, 
                                                z: Math.random() * LANDING_RANDOM - LANDING_RANDOM / 2.0 });
        MyAvatar.position = newPosition;

        lastGuidePosition = guide.position;
        MyAvatar.orientation = guide.orientation;
      }
    }
  }
}
Script.update.connect(update);