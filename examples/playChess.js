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
  boardURL: "https://s3.amazonaws.com/hifi-public/models/props/chess/Board.fbx",
  boardDimensions: { x: 773.191, y: 20.010, z: 773.191 },
  sizeFactor: 1.0 / 773.191,
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
      rotation: Quat.fromPitchYawRollDegrees(0, 90, 0),
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
  this.tileSize = size / 10.0;
  this.dimensions = Vec3.multiply(ChessGame.sizeFactor * size,
                                 ChessGame.boardDimensions);
  this.position = Vec3.sum(position, Vec3.multiply(0.5, this.dimensions));

  this.entity = null;
});
// Returns tile position
ChessGame.Board.prototype.tilePosition = function(i, j) {
  return { x: this.position.x + (i - 5.0 + 0.5) * this.tileSize,
           y: this.position.y + this.dimensions.y / 2.0,
           z: this.position.z + (j - 5.0 + 0.5) * this.tileSize };
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
  
  this.entity = Entities.addEntity({
    type: "Model",
    modelURL: ChessGame.boardURL,
    position: this.position,
    dimensions: this.dimensions
  });
  
  print("Board spawned");
}
// Cleans up the entities of the board
ChessGame.Board.prototype.cleanup = function() {
  if (extraDebug) {
    print("Cleaning up board...");
  }
  Entities.deleteEntity(this.entity);
  print("Board cleaned up");
}

// Piece class
ChessGame.Piece = (function(position, size, url, rotation) {
  this.size = size;
  this.position = position;
  this.position.y += this.size.y / 2.0;
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
  var size = Vec3.multiply(0.5 * ChessGame.sizeFactor, properties.dimensions);
  var rotation = (isWhite) ? properties.rotation
                           : Quat.multiply(Quat.fromPitchYawRollDegrees(0, 180, 0),
                                           properties.rotation);
  var position = ChessGame.board.tilePosition(i, j);
                
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
  var piece = ChessGame.makePiece(ChessGame.pieces.king, 1, 4, true);
  ChessGame.pieces.king.white = piece;
  // Queen
  piece = ChessGame.makePiece(ChessGame.pieces.queen, 1, 5, true);
  ChessGame.pieces.queen.white = piece;
  // Bishop
  piece = ChessGame.makePiece(ChessGame.pieces.bishop, 1, 3, true);
  ChessGame.pieces.bishop.white.push(piece);
  piece = ChessGame.makePiece(ChessGame.pieces.bishop, 1, 6, true);
  ChessGame.pieces.bishop.white.push(piece);
  // Knight
  piece = ChessGame.makePiece(ChessGame.pieces.knight, 1, 2, true);
  ChessGame.pieces.knight.white.push(piece);
  piece = ChessGame.makePiece(ChessGame.pieces.knight, 1, 7, true);
  ChessGame.pieces.knight.white.push(piece);
  // Rook
  piece = ChessGame.makePiece(ChessGame.pieces.rook, 1, 1, true);
  ChessGame.pieces.rook.white.push(piece);
  piece = ChessGame.makePiece(ChessGame.pieces.rook, 1, 8, true);
  ChessGame.pieces.rook.white.push(piece);
  for(var j = 1; j <= ChessGame.BOARD_SIZE; j++) {
    piece = ChessGame.makePiece(ChessGame.pieces.pawn, 2, j, true);
    ChessGame.pieces.pawn.white.push(piece);
  }
  
  // Setup black pieces
  // King
  var piece = ChessGame.makePiece(ChessGame.pieces.king, 8, 4, false);
  ChessGame.pieces.king.white = piece;
  // Queen
  piece = ChessGame.makePiece(ChessGame.pieces.queen, 8, 5, false);
  ChessGame.pieces.queen.white = piece;
  // Bishop
  piece = ChessGame.makePiece(ChessGame.pieces.bishop, 8, 3, false);
  ChessGame.pieces.bishop.white.push(piece);
  piece = ChessGame.makePiece(ChessGame.pieces.bishop, 8, 6, false);
  ChessGame.pieces.bishop.white.push(piece);
  // Knight
  piece = ChessGame.makePiece(ChessGame.pieces.knight, 8, 2, false);
  ChessGame.pieces.knight.white.push(piece);
  piece = ChessGame.makePiece(ChessGame.pieces.knight, 8, 7, false);
  ChessGame.pieces.knight.white.push(piece);
  // Rook
  piece = ChessGame.makePiece(ChessGame.pieces.rook, 8, 1, false);
  ChessGame.pieces.rook.white.push(piece);
  piece = ChessGame.makePiece(ChessGame.pieces.rook, 8, 8, false);
  ChessGame.pieces.rook.white.push(piece);
  for(var j = 1; j <= ChessGame.BOARD_SIZE; j++) {
    piece = ChessGame.makePiece(ChessGame.pieces.pawn, 7, j, false);
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