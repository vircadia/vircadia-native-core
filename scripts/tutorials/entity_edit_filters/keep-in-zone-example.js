function filter(properties, type, originalProperties, zoneProperties) {    

    var nearZero = 0.0001 * Math.random() + 0.001;

	/* Clamp position changes to bounding box of zone.*/
    function clamp(val, min, max) {
        /* Random near-zero value used as "zero" to prevent two sequential updates from being
        exactly the same (which would cause them to be ignored) */
        if (val > max) {
            val = max - nearZero;
        } else if (val < min) {
            val = min + nearZero;
        }
        return val;
    }

    if (properties.position) {
        properties.position.x = clamp(properties.position.x, zoneProperties.boundingBox.brn.x, zoneProperties.boundingBox.tfl.x);
        properties.position.y = clamp(properties.position.y, zoneProperties.boundingBox.brn.y, zoneProperties.boundingBox.tfl.y);
        properties.position.z = clamp(properties.position.z, zoneProperties.boundingBox.brn.z, zoneProperties.boundingBox.tfl.z);
    }

    return properties;
}