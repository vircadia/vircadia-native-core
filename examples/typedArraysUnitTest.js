//
//  typedArraysunitTest.js
//  examples
//
//  Created by Clément Brisset on 7/7/14
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("Test.js");

test('ArrayBuffer', function(finished) {
     this.assertEquals(new ArrayBuffer(0).byteLength, 0, 'no length');
     
     this.assertEquals(typeof(new ArrayBuffer(0)), 'object', 'creation');
     this.assertEquals(typeof(new ArrayBuffer(1)), 'object', 'creation');
     this.assertEquals(typeof(new ArrayBuffer(123)), 'object', 'creation');
     
     this.assertEquals(new ArrayBuffer(123).byteLength, 123, 'length');
});

test('DataView constructors', function (finished) {
  var d = new DataView(new ArrayBuffer(8));

  d.setUint32(0, 0x12345678);
  this.assertEquals(d.getUint32(0), 0x12345678, 'big endian/big endian');

  d.setUint32(0, 0x12345678, true);
  this.assertEquals(d.getUint32(0, true), 0x12345678, 'little endian/little endian');

  d.setUint32(0, 0x12345678, true);
  this.assertEquals(d.getUint32(0), 0x78563412, 'little endian/big endian');

  d.setUint32(0, 0x12345678);
  this.assertEquals(d.getUint32(0, true), 0x78563412, 'big endian/little endian');

  // raises(function () { return new DataView({}); }, 'non-ArrayBuffer argument');
  // raises(function () { return new DataView("bogus"); }, TypeError, 'non-ArrayBuffer argument');
});
