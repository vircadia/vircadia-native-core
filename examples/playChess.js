//
//  playChess.js
//  examples
//
//  Created by Clement Brisset on 11/08/2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

// Script namespace
var extraDebug = extraDebug || true;
var ChessGame = ChessGame || {
  BOARD_SIZE: 8,
  MAX_PLAYERS: 2,
  ROWS: ['1', '2', '3', '4', '5', '6', '7', '8'],
  COLUMNS: ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'],
  
  whitesTileColor: {red: 250, green: 195, blue: 135},
  blacksTileColor: {red: 190, green: 115, blue: 50},
  whiteKingURL: "http://public.highfidelity.io/models/attachments/King Piece.fst",
  whiteKingDimensions: { x: 0.46, y: 1.0, z: 0.46 },
  blackKingURL: "http://public.highfidelity.io/models/attachments/King Piece.fst",
  blackKingDimensions: { x: 0.46, y: 1.0, z: 0.46 },
  
  board: null,
  pieces: null
};


// Board class
ChessGame.Board = (function(position, size) {
  this.position = position;
  this.size = size;
  this.tileSize = this.size / ChessGame.BOARD_SIZE;
  this.height = this.tileSize * 0.2;
  this.tiles = new Array();
  
  this.spawn();
});
// Applies operation to each tile where operation = function(i, j)
ChessGame.Board.prototype.apply = function(operation) {
  for (var i = 0; i < ChessGame.BOARD_SIZE; i++) {
    for (var j = 0; j < ChessGame.BOARD_SIZE; j++) {
      operation.call(this, i, j);
    } 
  }
}
// Returns tile position
ChessGame.Board.prototype.tilePosition = function(i, j) {
  return { x: this.position.x + i * this.tileSize,
           y: this.position.y,
           z: this.position.z + j * this.tileSize };
}
// Checks the color of the tile
ChessGame.Board.prototype.isWhite = function(x, y) {
  return (x + y) % 2 != 0;
}
// Spawns the board using entities
ChessGame.Board.prototype.spawn = function() {
  if (extraDebug) {
    print("Spawning board...");
  }
  
  this.apply(function(i, j) {
    var tile = Entities.addEntity({
      type: "Box",
      position: this.tilePosition(i, j),
      dimensions: { x: this.tileSize, y: this.height, z: this.tileSize },
      color: (this.isWhite(i, j)) ? ChessGame.whitesTileColor : ChessGame.blacksTileColor
    });
    
    if (j === 0) {
      // Create new row if needed
      this.tiles.push(new Array());
    }
    this.tiles[i].push(tile); // Store tile
  });
  print("Board spawned");
}
// Cleans up the entities of the board
ChessGame.Board.prototype.cleanup = function() {
  if (extraDebug) {
    print("Cleaning up board...");
  }
  
  this.apply(function(i, j) {
    Entities.deleteEntity(this.tiles[i][j]);
  });
  print("Board cleaned up");
}

// Piece class
ChessGame.Piece = (function(position, size, url) {
  this.position = position;
  this.size = size;
  this.entityProperties = {
    type: "Model",
    position: this.position,
    dimensions: this.size,
    modelURL: url
  }
  this.entity = null;
});
// Spawns the piece
ChessGame.Piece.prototype.spawn = function() {
  this.entity = Entities.addEntity(this.entityProperties);
}
// Cleans up the piece
ChessGame.Piece.prototype.cleanup = function() {
  Entities.deleteEntity(this.entity);
}




// Player class
ChessGame.Player = (function() {
  
});

ChessGame.update = function() {
  
}

ChessGame.scriptStarting = function() {
  print("playChess.js started");
  ChessGame.board = new ChessGame.Board({ x: 1, y: 1, z: 1 }, 1);
  ChessGame.pieces = new Array();

  var url = "http://public.highfidelity.io/models/attachments/King Piece.fst";
  var dimensions = { x: 0.46, y: 1.0, z: 0.46 };
  
  var rowsIndex = [0, 1, 6, 7];
  for(var i in rowsIndex) {
    for(var j = 0; j < ChessGame.BOARD_SIZE; j++) {
      var size = Vec3.multiply(dimensions, ChessGame.board.tileSize);
      if (rowsIndex[i] == 1 || rowsIndex[i] == 6) {
        size = Vec3.multiply(size, 0.7);
      }
      var position = Vec3.sum(ChessGame.board.tilePosition(rowsIndex[i], j),
                              { x: 0, y: (size.y + ChessGame.board.height) / 2.0, z: 0});
      var color = (rowsIndex[i] < 4) ? ChessGame.whitesTileColor : ChessGame.blacksTileColor;
      
      var piece = new ChessGame.Piece(position, size, url);
      piece.spawn();
      ChessGame.pieces.push(piece);
    }
  }
}

ChessGame.scriptEnding = function() {
  // Cleaning up board
  ChessGame.board.cleanup();
  delete ChessGame.board;
  
  // Cleaning up pieces
  for(var i in ChessGame.pieces) {
    ChessGame.pieces[i].cleanup();
  }
  delete ChessGame.pieces;
  
  print("playChess.js finished");
}


Script.update.connect(ChessGame.update)
Script.scriptEnding.connect(ChessGame.scriptEnding);
ChessGame.scriptStarting();