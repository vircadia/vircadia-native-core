Script.include('easyStar.js');
var easystar = loadEasyStar();

var grid = [
    [0, 0, 1, 0, 0],
    [0, 0, 1, 0, 0],
    [0, 0, 1, 0, 0],
    [0, 0, 1, 0, 0],
    [0, 0, 0, 0, 0]
];

easystar.setGrid(grid);

easystar.setAcceptableTiles([0]);

easystar.findPath(0, 0, 4, 0, function(path) {
    if (path === null) {
        print("Path was not found.");
        Script.update.disconnect(tickEasyStar)
    } else {
        print("Path was found. The first Point is " + path[0].x + " " + path[0].y);
        Script.update.disconnect(tickEasyStar)
    }
});

var tickEasyStar = function() {
    easystar.calculate();
}

Script.update.connect(tickEasyStar);