.pragma library

// FIXME(loicm) It would be better to have these functions available in a global
//     set of common native C++ functions.

function clamp(x, min, max) {
    return Math.max(min, Math.min(x, max));
}

function lerp(x, a, b) {
    return ((1.0 - x) * a) + (x * b);
}

// Linearly project a value x from [xmin, xmax] into [ymin, ymax]
function projectValue(x, xmin, xmax, ymin, ymax) {
    return ((x - xmin) * ymax - (x - xmax) * ymin) / (xmax - xmin)
}

function clampAndProject(x, xmin, xmax, ymin, ymax) {
    return projectValue(clamp(x, xmin, xmax), xmin, xmax, ymin, ymax)
}
