function filter(p) {
    /******************************************************/
    /* General Filter Comments
    /*
    - Custom filters must be named "filter" and must be global
    - Block comments are ok, but not double-slash end-of-line-comments
    - Certain JavaScript functions are not available, like Math.sign(), as they are undefined in QT's non-conforming JS
    - HiFi's scripting interface is unavailable here. That means you can't call, for example, Users.*()
    */
    /******************************************************/
    
    /******************************************************/
    /* Simple Filter Examples
    /******************************************************/
    /* Simple example: if someone specifies name, add an 'x' to it. Note that print is ok to use. */
    if (p.name) {p.name += 'x'; print('fixme name', p. name);}

    
    /* This example clamps y. A better filter would probably zero y component of velocity and acceleration. */
    if (p.position) {p.position.y = Math.min(1, p.position.y); print('fixme p.y', p.position.y);}

    
    /* Can also reject new properties altogether by returning false */
    if (p.userData) { return false; }
	
    
    /* Reject if modifications made to Model properties */
    if (p.modelURL || p.compoundShapeURL || p.shape || p.shapeType || p.url || p.fps || p.currentFrame || p.running || p.loop || p.firstFrame || p.lastFrame || p.hold || p.textures || p.xTextureURL || p.yTextureURL || p.zTextureURL) { return false; }
	
    
    /******************************************************/
    /* Physical Property Filter Examples
    /*
    NOTES about filtering physical properties:
    - For now, ensure you always supply a new value for the filtered physical property
        (instead of simply removing the property)
    - Ensure you always specify a slightly different value for physical properties every
        time your filter returns. Look to "var nearZero" below for an example).
        This is necessary because Interface checks if a physical property has changed
        when deciding whether to apply or reject the server's physical properties.
        If a physical property's value doesn't change, Interface will reject the server's property value,
        and Bullet will continue simulating the entity with stale physical properties.
        Ensure that this value is not changed by such a small amount such that new values
        fall within floating point precision boundaries. If you accidentally do this, prepare for many
        hours of frustrating debugging :).
    */
    /******************************************************/
	/* Clamp velocity to maxVelocity units/second. Zeroing each component of acceleration keeps us from slamming.*/
    if (p.velocity) {
        var maxVelocity = 5;
        /* Random near-zero value used as "zero" to prevent two sequential updates from being
        exactly the same (which would cause them to be ignored) */
        var nearZero = 0.0001 * Math.random() + 0.001;
        function sign(val) {
            if (val > 0) {
                return 1;
            } else if (val < 0) {
                return -1;
            } else {
                return 0;
            }
        }
		if (Math.abs(p.velocity.x) > maxVelocity) {
			p.velocity.x = sign(p.velocity.x) * (maxVelocity + nearZero);
			p.acceleration.x = nearZero;
		}
		if (Math.abs(p.velocity.y) > maxVelocity) {
			p.velocity.y = sign(p.velocity.y) * (maxVelocity + nearZero);
			p.acceleration.y = nearZero;
		}
		if (Math.abs(p.velocity.z) > maxVelocity) {
			p.velocity.z = sign(p.velocity.z) * (maxVelocity + nearZero);
			p.acceleration.z = nearZero;
		}
    }
    
    
    /* Define an axis-aligned zone in which entities are not allowed to enter. */
    /* This example zone corresponds to an area to the right of the spawnpoint
    in your Sandbox. It's an area near the big rock to the right. If an entity
    enters the zone, it'll move behind the rock.*/
    if (p.position) {
        /* Random near-zero value used as "zero" to prevent two sequential updates from being
        exactly the same (which would cause them to be ignored) */
        var nearZero = 0.0001 * Math.random() + 0.001;
        /* Define the points that create the "NO ENTITIES ALLOWED" box */
        var boxMin = {x: 25.5, y: -0.48, z: -9.9};
        var boxMax = {x: 31.1, y: 4, z: -3.79};
        /* Define the point that you want entites that enter the box to appear */
        var resetPoint = {x: 29.5, y: 0.37 + nearZero, z: -2};
        var x = p.position.x;
        var y = p.position.y;
        var z = p.position.z;
        if ((x > boxMin.x && x < boxMax.x) &&
            (y > boxMin.y && y < boxMax.y) &&
            (z > boxMin.z && z < boxMax.z)) {
            p.position = resetPoint;
            if (p.velocity) {
                p.velocity = {x: 0, y: nearZero, z: 0};
            }
            if (p.acceleration) {
                p.acceleration = {x: 0, y: nearZero, z: 0};
            }
        }
    }

    /* If we make it this far, return the (possibly modified) properties. */
    return p;
}
