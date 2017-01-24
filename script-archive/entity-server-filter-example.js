function filter(p) {
    /* block comments are ok, but not double-slash end-of-line-comments */

    /* Simple example: if someone specifies name, add an 'x' to it. Note that print is ok to use. */
    if (p.name) {p.name += 'x'; print('fixme name', p. name);}

    /* This example clamps y. A better filter would probably zero y component of velocity and acceleration. */
    if (p.position) {p.position.y = Math.min(1, p.position.y); print('fixme p.y', p.position.y);}

    /* Can also reject altogether */
    if (p.userData) { return false; }
	
    /* Reject if modifications made to Model properties */
    if (p.modelURL || p.compoundShapeURL || p.shape || p.shapeType || p.url || p.fps || p.currentFrame || p.running || p.loop || p.firstFrame || p.lastFrame || p.hold || p.textures || p.xTextureURL || p.yTextureURL || p.zTextureURL) { return false; }
	
	/* Clamp velocity to 5 units/second. Zeroing each component of acceleration keeps us from slamming.*/
    if (p.velocity) {
		if (p.velocity.x > 5) {
			p.velocity.x = 5;
			p.acceleration.x = 0;
		}
		if (p.velocity.y > 5) {
			p.velocity.y = 5;
			p.acceleration.y = 0;
		}
		if (p.velocity.z > 5) {
			p.velocity.z = 5;
			p.acceleration.z = 0;
		}
    }

    /* If we make it this far, return the (possibly modified) properties. */
    return p;
}
