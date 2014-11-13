//
//  playChess.js
//  examples
//
//  Created by Clement Brisset on 11/08/2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

var gamePosition = { x: 1.0, y: 1.0, z: 1.0 };
var gameSize = 1.0;

// Script namespace
var extraDebug = extraDebug || true;
var ChessGame = ChessGame || {
  BOARD: {
    modelURL: "https://s3.amazonaws.com/hifi-public/models/props/chess/Board.fbx",
    dimensions: { x: 773.191, y: 20.010, z: 773.191 },
    rotation: Quat.fromPitchYawRollDegrees(0, 90, 0),
    numTiles: 10.0
  },
  KING: 0, QUEEN: 1, BISHOP: 2, KNIGHT: 3, ROOK: 4, PAWN: 5,
  
  board: null,
  pieces: new Array()
};

ChessGame.getPieceInfo = function(type, isWhite) {
  var info = {
    modelURL: "https://s3.amazonaws.com/hifi-public/models/props/chess/",
    dimensions: { x: 1.0, y: 1.0, z: 1.0 },
    rotation: Quat.fromPitchYawRollDegrees(0, 0, 0)
  }
  
  switch(type) {
  case ChessGame.KING:
    info.modelURL += "King";
    info.dimensions = { x: 110.496, y: 306.713, z: 110.496 };
    info.rotation = Quat.fromPitchYawRollDegrees(0, 90, 0);
    break;
  case ChessGame.QUEEN:
    info.modelURL += "Queen";
    info.dimensions = { x: 110.496, y: 257.459, z: 110.496 };
    break;
  case ChessGame.BISHOP:
    info.modelURL += "Bishop";
    info.dimensions = { x: 102.002, y: 213.941, z: 102.002 };
    info.rotation = Quat.fromPitchYawRollDegrees(0, 90, 0);
    break;
  case ChessGame.KNIGHT:
    info.modelURL += "Knight";
    info.dimensions = { x: 95.602, y: 186.939, z: 95.602 };
    info.rotation = Quat.fromPitchYawRollDegrees(0, 180, 0);
    break;
  case ChessGame.ROOK:
    info.modelURL += "Rook";
    info.dimensions = { x: 101.024, y: 167.523, z: 101.024 };
    break;
  case ChessGame.PAWN:
    info.modelURL += "Pawn";
    info.dimensions = { x: 93.317, y: 138.747, z: 93.317 };
    break;
  }
  info.modelURL += ((isWhite) ? "_White" : "_Black") + ".fbx"
  
  return info;
}


// Board class
ChessGame.Board = (function(position, size) {
  this.dimensions = Vec3.multiply(1.0 / ChessGame.BOARD.dimensions.x, ChessGame.BOARD.dimensions); // 1 by 1
  this.dimensions = Vec3.multiply(size, this.dimensions); // size by size
  this.position = Vec3.sum(position, Vec3.multiply(0.5, this.dimensions));
  this.tileSize = size / ChessGame.BOARD.numTiles;

  this.entity = null;
});
// Returns the top center point of tile i,j
ChessGame.Board.prototype.tilePosition = function(i, j) {
  return {
    x: this.position.x + (i - ChessGame.BOARD.numTiles / 2.0 + 0.5) * this.tileSize,
    y: this.position.y + this.dimensions.y / 2.0,
    z: this.position.z + (j - ChessGame.BOARD.numTiles / 2.0 + 0.5) * this.tileSize
  };
}
// Checks the color of the tile
ChessGame.Board.prototype.isWhite = function(i, j) {
  return (i + j) % 2 != 0;
}
// Spawns the board using entities
ChessGame.Board.prototype.spawn = function() {
  if (extraDebug) {
    print("Spawning board...");
  }
  
  this.entity = Entities.addEntity({
    type: "Model",
    modelURL: ChessGame.BOARD.modelURL,
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
ChessGame.Piece = (function(position, dimensions, url, rotation) {
  this.dimensions = dimensions;
  this.position = position;
  this.position.y += this.dimensions.y / 2.0;
  this.entityProperties = {
    type: "Model",
    position: this.position,
    rotation: rotation,
    dimensions: this.dimensions,
    modelURL: url,
    //script: "https://s3.amazonaws.com/hifi-public/scripts/entityScripts/chessPiece.js"
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
ChessGame.Piece.prototype.updateMetadata = function(metadataString) {
  Entities.editEntity(this.entity, { animationURL: metadataString });
}
// Cleans up the piece
ChessGame.Piece.prototype.cleanup = function() {
  Entities.deleteEntity(this.entity);
}
ChessGame.makePiece = function(piece, i, j, isWhite) {
  var info = ChessGame.getPieceInfo(piece, isWhite);
  var url = info.modelURL; 
  var size = Vec3.multiply(0.5 * (1.0 / ChessGame.BOARD.dimensions.x), info.dimensions);
  var rotation = (isWhite) ? info.rotation
                           : Quat.multiply(Quat.fromPitchYawRollDegrees(0, 180, 0),
                                           info.rotation);
  var position = ChessGame.board.tilePosition(i, j);
                
  var piece = new ChessGame.Piece(position, size, url, rotation);
  piece.spawn();
  ChessGame.pieces.push(piece);
  return piece;
}

ChessGame.buildMetadataString = function() {
  var metadataObject = {
    gamePosition: gamePosition,
    gameSize: gameSize,
    pieces: new Array()
  };
  for (i in ChessGame.pieces) {
    metadataObject.pieces.push(ChessGame.pieces[i].entity);
  }
  return JSON.stringify(metadataObject);
}

ChessGame.buildBoard = function() {
  // Setup board
  ChessGame.board = new ChessGame.Board(gamePosition, gameSize);
  ChessGame.board.spawn();
  
  // Setup white pieces
  var isWhite = true;
  var row = 1;
  // King
  var piece =ChessGame.makePiece(ChessGame.KING, row, 5, isWhite);
  // Queen
  piece = ChessGame.makePiece(ChessGame.QUEEN, row, 4, isWhite);
  // Bishop
  piece = ChessGame.makePiece(ChessGame.BISHOP, row, 3, isWhite);
  piece = ChessGame.makePiece(ChessGame.BISHOP, row, 6, isWhite);
  // Knight
  piece = ChessGame.makePiece(ChessGame.KNIGHT, row, 2, isWhite);
  piece = ChessGame.makePiece(ChessGame.KNIGHT, row, 7, isWhite);
  // Rook
  piece = ChessGame.makePiece(ChessGame.ROOK, row, 1, isWhite);
  piece = ChessGame.makePiece(ChessGame.ROOK, row, 8, isWhite);
  for(var j = 1; j <= 8; j++) {
    piece = ChessGame.makePiece(ChessGame.PAWN, row + 1, j, isWhite);
  }
  
  // Setup black pieces
  isWhite = false;
  row = 8;
  // King
  piece = ChessGame.makePiece(ChessGame.KING, row, 5, isWhite);
  // Queen
  piece = ChessGame.makePiece(ChessGame.QUEEN, row, 4, isWhite);
  // Bishop
  piece = ChessGame.makePiece(ChessGame.BISHOP, row, 3, isWhite);
  piece = ChessGame.makePiece(ChessGame.BISHOP, row, 6, isWhite);
  // Knight
  piece = ChessGame.makePiece(ChessGame.KNIGHT, row, 2, isWhite);
  piece = ChessGame.makePiece(ChessGame.KNIGHT, row, 7, isWhite);
  // Rook
  piece = ChessGame.makePiece(ChessGame.ROOK, row, 1, isWhite);
  piece = ChessGame.makePiece(ChessGame.ROOK, row, 8, isWhite);
  for(var j = 1; j <= 8; j++) {
    piece = ChessGame.makePiece(ChessGame.PAWN, row - 1, j, isWhite);
  }
  
  var metadataString = ChessGame.buildMetadataString();
  for (i in ChessGame.pieces) {
    ChessGame.pieces[i].updateMetadata(metadataString);
  }
}


ChessGame.scriptStarting = function() {
  print("playChess.js started");
  gamePosition = Vec3.sum(MyAvatar.position,
                          Vec3.multiplyQbyV(MyAvatar.orientation,
                                            { x: 0, y: 0, z: -1 }));
  ChessGame.buildBoard();
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

Script.scriptEnding.connect(ChessGame.scriptEnding);
ChessGame.scriptStarting();