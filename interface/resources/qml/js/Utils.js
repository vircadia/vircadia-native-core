
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

function formatSize(size) {
    var suffixes = [ "bytes", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB" ];
    var suffixIndex = 0
    while ((size / 1024.0) > 1.1) {
        size /= 1024.0;
        ++suffixIndex;
    }

    size = Math.round(size*1000)/1000;
    size = size.toLocaleString()

    return size + " " + suffixes[suffixIndex];
}
