
function addOne() {

   var newOverlay = Overlays.addOverlay("image", {
             x: 100, y: 100, width: 500, height: 250,
		//	imageURL: "http://selfieFrame",
			imageURL: "http://1.bp.blogspot.com/-1GABEq__054/T03B00j_OII/AAAAAAAAAa8/jo55LcvEPHI/s1600/Winning.jpg",
             alpha: 1.0
         });

}

function addTwo() {

   	var orientation = MyAvatar.orientation;
	orientation = Quat.safeEulerAngles(orientation);
	orientation.x = 0;
	orientation = Quat.fromVec3Degrees(orientation);
	var root = Vec3.sum(MyAvatar.position, Vec3.multiply(5, Quat.getForward(orientation)));


    var newOverlay = Overlays.addOverlay("image3d", {
   	                //url: kickOverlayURL(),
                    	url: "http://selfieFrame",
					//url: "http://1.bp.blogspot.com/-1GABEq__054/T03B00j_OII/AAAAAAAAAa8/jo55LcvEPHI/s1600/Winning.jpg",
     				position: root,
                    size: 1,
                    scale: 5.0,
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    solid: true,
                    isFacingAvatar: true,
                    drawInFront: false
                });
}


addTwo();