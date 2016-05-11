//
//  junkyardClientReset.js
//  examples/junkyard
//
//  Created by Eric Levin on 1/21/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  This script resets the junkyard scene
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var IMPORT_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/arfs/junkyard.json";
var PASTE_ENTITIES_LOCATION = {x: 0, y: 0, z: 0};
reset();

function reset() {
  // Delete everything and re-import the junkyard arf
  var e = Entities.findEntities(MyAvatar.position, 1000);
  for (i = 0; i < e.length; i++) {
    Entities.deleteEntity(e[i]);
  }
  importAssetResourceFile();
}

function importAssetResourceFile() {
  Clipboard.importEntities(IMPORT_URL);
  Clipboard.pasteEntities(PASTE_ENTITIES_LOCATION);
}