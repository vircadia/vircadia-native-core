
function clamp(value, min, max) {
    return Math.min(Math.max(value, min), max);
}

function clampVector(value, min, max) {
    return Qt.vector2d(
                clamp(value.x, min.x, max.x),
                clamp(value.y, min.y, max.y))
}

function randomPosition(min, max) {
    return Qt.vector2d(
                Math.random() * (max.x - min.x),
                Math.random() * (max.y - min.y));
}
