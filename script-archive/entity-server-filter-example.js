function filter(p) {
    /* block comments are ok, but not double-slash end-of-line-comments */

    /* Simple example: if someone specifies name, add an 'x' to it. Note that print is ok to use. */
    if (p.name) {p.name += 'x'; print('fixme name', p. name);}

    /* This example clamps y. A better filter would probably zero y component of velocity and acceleration. */
    if (p.position) {p.position.y = Math.min(1, p.position.y); print('fixme p.y', p.position.y);}

    /* Can also reject altogether */
    if (p.userData) { return false; }

    /* If we make it this far, return the (possibly modified) properties. */
    return p;
}
