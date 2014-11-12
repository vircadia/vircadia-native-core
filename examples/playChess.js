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
  
  whitesTileColor: { red: 250, green: 195, blue: 135 },
  blacksTileColor: { red: 190, green: 115, blue: 50 },

  board: null,
  pieces: {
    all: new Array(),
    king: {
      whiteURL: "https://s3.amazonaws.com/hifi-public/models/props/chess/King_White.fbx",
      blackURL: "https://s3.amazonaws.com/hifi-public/models/props/chess/King_Black.fbx",
      dimensions: { x: 110.496, y: 306.713, z: 110.496 },
      rotation: Quat.fromPitchYawRollDegrees(0, 90, 0)
    },
    queen: {
      whiteURL: "https://s3.amazonaws.com/hifi-public/models/props/chess/Queen_White.fbx",
      blackURL: "https://s3.amazonaws.com/hifi-public/models/props/chess/Queen_Black.fbx",
      dimensions: { x: 110.496, y: 257.459, z: 110.496 },
      rotation: Quat.fromPitchYawRollDegrees(0, 0, 0)
    },
    bishop: {
      whiteURL: "https://s3.amazonaws.com/hifi-public/models/props/chess/Bishop_White.fbx",
      blackURL: "https://s3.amazonaws.com/hifi-public/models/props/chess/Bishop_Black.fbx",
      dimensions: { x: 102.002, y: 213.941, z: 102.002 },
      rotation: Quat.fromPitchYawRollDegrees(0, 0, 0),
      white: [],
      black: []
    },
    knight: {
      whiteURL: "https://s3.amazonaws.com/hifi-public/models/props/chess/Knight_White.fbx",
      blackURL: "https://s3.amazonaws.com/hifi-public/models/props/chess/Knight_Black.fbx",
      dimensions: { x: 95.602, y: 186.939, z: 95.602 },
      rotation: Quat.fromPitchYawRollDegrees(0, 180, 0),
      white: [],
      black: []
    },
    rook: {
      whiteURL: "https://s3.amazonaws.com/hifi-public/models/props/chess/Rook_White.fbx",
      blackURL: "https://s3.amazonaws.com/hifi-public/models/props/chess/Rook_Black.fbx",
      dimensions: { x: 101.024, y: 167.523, z: 101.024 },
      rotation: Quat.fromPitchYawRollDegrees(0, 0, 0),
      white: [],
      black: []
    },
    pawn: {
      whiteURL: "https://s3.amazonaws.com/hifi-public/models/props/chess/Pawn_White.fbx",
      blackURL: "https://s3.amazonaws.com/hifi-public/models/props/chess/Pawn_Black.fbx",
      dimensions: { x: 93.317, y: 138.747, z: 93.317 },
      rotation: Quat.fromPitchYawRollDegrees(0, 0, 0),
      white: [],
      black: []
    }
  }
};


// Board class
ChessGame.Board = (function(position, size) {
  this.size = size;
  this.tileSize = this.size / ChessGame.BOARD_SIZE;
  this.height = this.tileSize * 0.2;

  this.position = position;
  this.position.x += this.tileSize / 2.0;
  this.position.y += this.height / 2.0;
  this.position.z += this.tileSize / 2.0;

  this.tiles = new Array();
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
ChessGame.Piece = (function(position, size, url, rotation) {
  this.position = position;
  this.size = size;
  this.entityProperties = {
    type: "Model",
    position: this.position,
    rotation: rotation,
    dimensions: this.size,
    modelURL: url,
    //script: "https://s3-us-west-1.amazonaws.com/highfidelity-dev/scripts/chessPiece.js"
    script: "file:/Users/clement/hifi/examples/entityScripts/chessPiece.js"
  }
  this.entity = null;
});
// Spawns the piece
ChessGame.Piece.prototype.spawn = function() {
  this.entity = Entities.addEntity(this.entityProperties);
}
// Updates the metadata stored by the piece
ChessGame.Piece.prototype.updateMetaData = function(metaDataString) {
  Entities.editEntity(this.entity, { animationURL: metaDataString });
}
// Cleans up the piece
ChessGame.Piece.prototype.cleanup = function() {
  Entities.deleteEntity(this.entity);
}
ChessGame.makePiece = function(properties, i, j, isWhite) {
  var url = (isWhite) ? properties.whiteURL : properties.blackURL;
  var size = Vec3.multiply(1.0 / ChessGame.pieces.king.dimensions.y,
                           properties.dimensions);
  size = Vec3.multiply(1.5 * ChessGame.board.tileSize, size);
  var rotation = (isWhite) ? properties.rotation
                           : Quat.multiply(Quat.fromPitchYawRollDegrees(0, 180, 0),
                                           properties.rotation);
  var position = Vec3.sum(ChessGame.board.tilePosition(i, j),
                          { x: 0, y: (size.y + ChessGame.board.height) / 2.0, z: 0});
                
  var piece = new ChessGame.Piece(position, size, url, rotation);
  piece.spawn();
  ChessGame.pieces.all.push(piece);
  return piece;
}


// Player class
ChessGame.Player = (function() {
  
});

ChessGame.update = function() {
  
}
ChessGame.buildMetaDataString = function() {
  var metadataObject = {
    board: {
      position: ChessGame.board.position,
      size: ChessGame.board.size
    }
  };
  
  return JSON.stringify(metadataObject);
}

ChessGame.buildBoard = function() {
  // Setup board
  ChessGame.board = new ChessGame.Board({ x: 1, y: 1, z: 1 }, 1);
  ChessGame.board.spawn();
  
  // Setup white pieces
  // King
  var piece = ChessGame.makePiece(ChessGame.pieces.king, 0, 3, true);
  ChessGame.pieces.king.white = piece;
  // Queen
  piece = ChessGame.makePiece(ChessGame.pieces.queen, 0, 4, true);
  ChessGame.pieces.queen.white = piece;
  // Bishop
  piece = ChessGame.makePiece(ChessGame.pieces.bishop, 0, 2, true);
  ChessGame.pieces.bishop.white.push(piece);
  piece = ChessGame.makePiece(ChessGame.pieces.bishop, 0, 5, true);
  ChessGame.pieces.bishop.white.push(piece);
  // Knight
  piece = ChessGame.makePiece(ChessGame.pieces.knight, 0, 1, true);
  ChessGame.pieces.knight.white.push(piece);
  piece = ChessGame.makePiece(ChessGame.pieces.knight, 0, 6, true);
  ChessGame.pieces.knight.white.push(piece);
  // Rook
  piece = ChessGame.makePiece(ChessGame.pieces.rook, 0, 0, true);
  ChessGame.pieces.rook.white.push(piece);
  piece = ChessGame.makePiece(ChessGame.pieces.rook, 0, 7, true);
  ChessGame.pieces.rook.white.push(piece);
  for(var j = 0; j < ChessGame.BOARD_SIZE; j++) {
    piece = ChessGame.makePiece(ChessGame.pieces.pawn, 1, j, true);
    ChessGame.pieces.pawn.white.push(piece);
  }
  
  // Setup black pieces
  // King
  var piece = ChessGame.makePiece(ChessGame.pieces.king, 7, 3, false);
  ChessGame.pieces.king.white = piece;
  // Queen
  piece = ChessGame.makePiece(ChessGame.pieces.queen, 7, 4, false);
  ChessGame.pieces.queen.white = piece;
  // Bishop
  piece = ChessGame.makePiece(ChessGame.pieces.bishop, 7, 2, false);
  ChessGame.pieces.bishop.white.push(piece);
  piece = ChessGame.makePiece(ChessGame.pieces.bishop, 7, 5, false);
  ChessGame.pieces.bishop.white.push(piece);
  // Knight
  piece = ChessGame.makePiece(ChessGame.pieces.knight, 7, 1, false);
  ChessGame.pieces.knight.white.push(piece);
  piece = ChessGame.makePiece(ChessGame.pieces.knight, 7, 6, false);
  ChessGame.pieces.knight.white.push(piece);
  // Rook
  piece = ChessGame.makePiece(ChessGame.pieces.rook, 7, 0, false);
  ChessGame.pieces.rook.white.push(piece);
  piece = ChessGame.makePiece(ChessGame.pieces.rook, 7, 7, false);
  ChessGame.pieces.rook.white.push(piece);
  for(var j = 0; j < ChessGame.BOARD_SIZE; j++) {
    piece = ChessGame.makePiece(ChessGame.pieces.pawn, 6, j, false);
    ChessGame.pieces.pawn.black.push(piece);
  }
}


ChessGame.scriptStarting = function() {
  print("playChess.js started");
  ChessGame.buildBoard();
}

ChessGame.scriptEnding = function() {
  // Cleaning up board
  ChessGame.board.cleanup();
  delete ChessGame.board;
  
  // Cleaning up pieces
  for(var i in ChessGame.pieces.all) {
    ChessGame.pieces.all[i].cleanup();
  }
  delete ChessGame.pieces;
  
  print("playChess.js finished");
}


Script.update.connect(ChessGame.update)
Script.scriptEnding.connect(ChessGame.scriptEnding);
ChessGame.scriptStarting();