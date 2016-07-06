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

Script.include("../../../../script-archive/libraries/unitTest.js");

// e.g. extractbits([0xff, 0x80, 0x00, 0x00], 23, 30); inclusive
function extractbits(bytes, lo, hi) {
  var out = 0;
  bytes = bytes.slice(); // make a copy
  var lsb = bytes.pop(), sc = 0, sh = 0;

  for (; lo > 0;  lo--, hi--) {
    lsb >>= 1;
    if (++sc === 8) { sc = 0; lsb = bytes.pop(); }
  }

  for (; hi >= 0;  hi--) {
    out = out | (lsb & 0x01) << sh++;
    lsb >>= 1;
    if (++sc === 8) { sc = 0; lsb = bytes.pop(); }
  }

  return out;
}

test('ArrayBuffer', function(finished) {
     this.assertEquals(new ArrayBuffer(0).byteLength, 0, 'no length');
     
     this.assertEquals(typeof(new ArrayBuffer(0)), 'object', 'creation');
     this.assertEquals(typeof(new ArrayBuffer(1)), 'object', 'creation');
     this.assertEquals(typeof(new ArrayBuffer(123)), 'object', 'creation');
     
     this.assertEquals(new ArrayBuffer(123).byteLength, 123, 'length');

      this.raises(function () { return new ArrayBuffer(-1); }, 'negative length');
      this.raises(function () { return new ArrayBuffer(0x80000000); }, 'absurd length');
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

  this.raises(function () { return new DataView({}); }, 'non-ArrayBuffer argument');
  this.raises(function () { return new DataView("bogus"); }, 'non-ArrayBuffer argument');
});


test('ArrayBufferView', function () {
  var ab = new ArrayBuffer(48);
  var i32 = new Int32Array(ab, 16);
  i32.set([1, 2, 3, 4, 5, 6, 7, 8]);

  this.assertEquals(i32.buffer, ab, 'ArrayBuffers equal');
  this.assertEquals(i32.byteOffset, 16, 'byteOffset');
  this.assertEquals(i32.byteLength, 32, 'byteLength');

  var da = new DataView(i32.buffer, 8);
  this.assertEquals(da.buffer, ab, 'DataView: ArrayBuffers equal');
  this.assertEquals(da.byteOffset, 8, 'DataView: byteOffset');
  this.assertEquals(da.byteLength, 40, 'DataView: byteLength');
});

test('TypedArrays', function () {
  var a;

  this.assertEquals(Int8Array.BYTES_PER_ELEMENT, 1, 'Int8Array.BYTES_PER_ELEMENT');
  a = new Int8Array([1, 2, 3, 4, 5, 6, 7, 8]);
  this.assertEquals(a.BYTES_PER_ELEMENT, 1, 'int8Array.BYTES_PER_ELEMENT');
  this.assertEquals(a.byteOffset, 0, 'int8Array.byteOffset');
  this.assertEquals(a.byteLength, 8, 'int8Array.byteLength');

  this.assertEquals(Uint8Array.BYTES_PER_ELEMENT, 1, 'Uint8Array.BYTES_PER_ELEMENT');
  a = new Uint8Array([1, 2, 3, 4, 5, 6, 7, 8]);
  this.assertEquals(a.BYTES_PER_ELEMENT, 1, 'uint8Array.BYTES_PER_ELEMENT');
  this.assertEquals(a.byteOffset, 0, 'uint8Array.byteOffset');
  this.assertEquals(a.byteLength, 8, 'uint8Array.byteLength');

  this.assertEquals(Int16Array.BYTES_PER_ELEMENT, 2, 'Int16Array.BYTES_PER_ELEMENT');
  a = new Int16Array([1, 2, 3, 4, 5, 6, 7, 8]);
  this.assertEquals(a.BYTES_PER_ELEMENT, 2, 'int16Array.BYTES_PER_ELEMENT');
  this.assertEquals(a.byteOffset, 0, 'int16Array.byteOffset');
  this.assertEquals(a.byteLength, 16, 'int16Array.byteLength');

  this.assertEquals(Uint16Array.BYTES_PER_ELEMENT, 2, 'Uint16Array.BYTES_PER_ELEMENT');
  a = new Uint16Array([1, 2, 3, 4, 5, 6, 7, 8]);
  this.assertEquals(a.BYTES_PER_ELEMENT, 2, 'uint16Array.BYTES_PER_ELEMENT');
  this.assertEquals(a.byteOffset, 0, 'uint16Array.byteOffset');
  this.assertEquals(a.byteLength, 16, 'uint16Array.byteLength');

  this.assertEquals(Int32Array.BYTES_PER_ELEMENT, 4, 'Int32Array.BYTES_PER_ELEMENT');
  a = new Int32Array([1, 2, 3, 4, 5, 6, 7, 8]);
  this.assertEquals(a.BYTES_PER_ELEMENT, 4, 'int32Array.BYTES_PER_ELEMENT');
  this.assertEquals(a.byteOffset, 0, 'int32Array.byteOffset');
  this.assertEquals(a.byteLength, 32, 'int32Array.byteLength');

  this.assertEquals(Uint32Array.BYTES_PER_ELEMENT, 4, 'Uint32Array.BYTES_PER_ELEMENT');
  a = new Uint32Array([1, 2, 3, 4, 5, 6, 7, 8]);
  this.assertEquals(a.BYTES_PER_ELEMENT, 4, 'uint32Array.BYTES_PER_ELEMENT');
  this.assertEquals(a.byteOffset, 0, 'uint32Array.byteOffset');
  this.assertEquals(a.byteLength, 32, 'uint32Array.byteLength');

  this.assertEquals(Float32Array.BYTES_PER_ELEMENT, 4, 'Float32Array.BYTES_PER_ELEMENT');
  a = new Float32Array([1, 2, 3, 4, 5, 6, 7, 8]);
  this.assertEquals(a.BYTES_PER_ELEMENT, 4, 'float32Array.BYTES_PER_ELEMENT');
  this.assertEquals(a.byteOffset, 0, 'float32Array.byteOffset');
  this.assertEquals(a.byteLength, 32, 'float32Array.byteLength');

  this.assertEquals(Float64Array.BYTES_PER_ELEMENT, 8, 'Float64Array.BYTES_PER_ELEMENT');
  a = new Float64Array([1, 2, 3, 4, 5, 6, 7, 8]);
  this.assertEquals(a.BYTES_PER_ELEMENT, 8, 'float64Array.BYTES_PER_ELEMENT');
  this.assertEquals(a.byteOffset, 0, 'float64Array.byteOffset');
  this.assertEquals(a.byteLength, 64, 'float64Array.byteLength');
});


test('typed array constructors', function () {
  this.arrayEqual(new Int8Array({ length: 3 }), [0, 0, 0], 'array equal -1');
  var rawbuf = (new Uint8Array([0, 1, 2, 3, 4, 5, 6, 7])).buffer;

  var int8 = new Int8Array();
  this.assertEquals(int8.length, 0, 'no args 0');
  this.raises(function () { return new Int8Array(-1); }, 'bogus length');
  this.raises(function () { return new Int8Array(0x80000000); }, 'bogus length');

  int8 = new Int8Array(4);
  this.assertEquals(int8.BYTES_PER_ELEMENT, 1);
  this.assertEquals(int8.length, 4, 'length 1');
  this.assertEquals(int8.byteLength, 4, 'length 2');
  this.assertEquals(int8.byteOffset, 0, 'length 3');
  this.assertEquals(int8.get(-1), undefined, 'length, out of bounds 4');
  this.assertEquals(int8.get(4), undefined, 'length, out of bounds 5');

  int8 = new Int8Array([1, 2, 3, 4, 5, 6]);
  this.assertEquals(int8.length, 6, 'array 6');
  this.assertEquals(int8.byteLength, 6, 'array 7');
  this.assertEquals(int8.byteOffset, 0, 'array 8');
  this.assertEquals(int8.get(3), 4, 'array 9');
  this.assertEquals(int8.get(-1), undefined, 'array, out of bounds  10');
  this.assertEquals(int8.get(6), undefined, 'array, out of bounds 11');

  int8 = new Int8Array(rawbuf);
  this.assertEquals(int8.length, 8, 'buffer 12');
  this.assertEquals(int8.byteLength, 8, 'buffer 13');
  this.assertEquals(int8.byteOffset, 0, 'buffer 14');
  this.assertEquals(int8.get(7), 7, 'buffer 15');
  int8.set([111]);
  this.assertEquals(int8.get(0), 111, 'buffer 16');
  this.assertEquals(int8.get(-1), undefined, 'buffer, out of bounds 17');
  this.assertEquals(int8.get(8), undefined, 'buffer, out of bounds 18');

  int8 = new Int8Array(rawbuf, 2);
  this.assertEquals(int8.length, 6, 'buffer, byteOffset 19');
  this.assertEquals(int8.byteLength, 6, 'buffer, byteOffset 20');
  this.assertEquals(int8.byteOffset, 2, 'buffer, byteOffset 21');
  this.assertEquals(int8.get(5), 7, 'buffer, byteOffset 22');
  int8.set([112]);
  this.assertEquals(int8.get(0), 112, 'buffer 23');
  this.assertEquals(int8.get(-1), undefined, 'buffer, byteOffset, out of bounds 24');
  this.assertEquals(int8.get(6), undefined, 'buffer, byteOffset, out of bounds 25');
  
  int8 = new Int8Array(rawbuf, 8);
  this.assertEquals(int8.length, 0, 'buffer, byteOffset 26');

  this.raises(function () { return new Int8Array(rawbuf, -1); }, 'invalid byteOffset 27');
  this.raises(function () { return new Int8Array(rawbuf, 9); }, 'invalid byteOffset 28');
  this.raises(function () { return new Int32Array(rawbuf, -1); }, 'invalid byteOffset 29');
  this.raises(function () { return new Int32Array(rawbuf, 5); }, 'invalid byteOffset 30');

  int8 = new Int8Array(rawbuf, 2, 4);
  this.assertEquals(int8.length, 4, 'buffer, byteOffset, length 31');
  this.assertEquals(int8.byteLength, 4, 'buffer, byteOffset, length 32');
  this.assertEquals(int8.byteOffset, 2, 'buffer, byteOffset, length 33');
  this.assertEquals(int8.get(3), 5, 'buffer, byteOffset, length 34');
  int8.set([113]);
  this.assertEquals(int8.get(0), 113, 'buffer, byteOffset, length 35');
  this.assertEquals(int8.get(-1), undefined, 'buffer, byteOffset, length, out of bounds 36');
  this.assertEquals(int8.get(4), undefined, 'buffer, byteOffset, length, out of bounds 37');

  this.raises(function () { return new Int8Array(rawbuf, 0, 9); }, 'invalid byteOffset+length');
  this.raises(function () { return new Int8Array(rawbuf, 8, 1); }, 'invalid byteOffset+length');
  this.raises(function () { return new Int8Array(rawbuf, 9, -1); }, 'invalid byteOffset+length');
});

test('TypedArray clone constructor', function () {
  var src = new Int32Array([1, 2, 3, 4, 5, 6, 7, 8]);
  var dst = new Int32Array(src);
  this.arrayEqual(dst, [1, 2, 3, 4, 5, 6, 7, 8], '1');
  src.set([99]);
  this.arrayEqual(src, [99, 2, 3, 4, 5, 6, 7, 8], '2');
  this.arrayEqual(dst, [1, 2, 3, 4, 5, 6, 7, 8], '3');
});


test('conversions', function () {
  var uint8 = new Uint8Array([1, 2, 3, 4]),
      uint16 = new Uint16Array(uint8.buffer),
      uint32 = new Uint32Array(uint8.buffer);

  // Note: can't probe individual bytes without endianness awareness
  this.arrayEqual(uint8, [1, 2, 3, 4]);
  uint16.set([0xffff]);
  this.arrayEqual(uint8, [0xff, 0xff, 3, 4]);
  uint16.set([0xeeee], 1);
  this.arrayEqual(uint8, [0xff, 0xff, 0xee, 0xee]);
  uint32.set([0x11111111]);
  this.assertEquals(uint16.get(0), 0x1111);
  this.assertEquals(uint16.get(1), 0x1111);
  this.arrayEqual(uint8, [0x11, 0x11, 0x11, 0x11]);
});


test('signed/unsigned conversions', function () {

  var int8 = new Int8Array(1), uint8 = new Uint8Array(int8.buffer);
  uint8.set([123]);
  this.assertEquals(int8.get(0), 123, 'int8/uint8');
  uint8.set([161]);
  this.assertEquals(int8.get(0), -95, 'int8/uint8');
  int8.set([-120]);
  this.assertEquals(uint8.get(0), 136, 'uint8/int8');
  int8.set([-1]);
  this.assertEquals(uint8.get(0), 0xff, 'uint8/int8');

  var int16 = new Int16Array(1), uint16 = new Uint16Array(int16.buffer);
  uint16.set([3210]);
  this.assertEquals(int16.get(0), 3210, 'int16/uint16');
  uint16.set([49232]);
  this.assertEquals(int16.get(0), -16304, 'int16/uint16');
  int16.set([-16384]);
  this.assertEquals(uint16.get(0), 49152, 'uint16/int16');
  int16.set([-1]);
  this.assertEquals(uint16.get(0), 0xffff, 'uint16/int16');

  var int32 = new Int32Array(1), uint32 = new Uint32Array(int32.buffer);
  uint32.set([0x80706050]);
  this.assertEquals(int32.get(0), -2140118960, 'int32/uint32');
  int32.set([-2023406815]);
  this.assertEquals(uint32.get(0), 0x87654321, 'uint32/int32');
  int32.set([-1]);
  this.assertEquals(uint32.get(0), 0xffffffff, 'uint32/int32');
});


test('IEEE754 single precision unpacking', function () {
  function fromBytes(bytes) {
    var uint8 = new Uint8Array(bytes),
        dv = new DataView(uint8.buffer);
    return dv.getFloat32(0);
  }

  this.assertEquals(isNaN(fromBytes([0xff, 0xff, 0xff, 0xff])), true, 'Q-NaN');
  this.assertEquals(isNaN(fromBytes([0xff, 0xc0, 0x00, 0x01])), true, 'Q-NaN');

  this.assertEquals(isNaN(fromBytes([0xff, 0xc0, 0x00, 0x00])), true, 'Indeterminate');

  this.assertEquals(isNaN(fromBytes([0xff, 0xbf, 0xff, 0xff])), true, 'S-NaN');
  this.assertEquals(isNaN(fromBytes([0xff, 0x80, 0x00, 0x01])), true, 'S-NaN');

  this.assertEquals(fromBytes([0xff, 0x80, 0x00, 0x00]), -Infinity, '-Infinity');

  this.assertEquals(fromBytes([0xff, 0x7f, 0xff, 0xff]), -3.4028234663852886E+38, '-Normalized');
  this.assertEquals(fromBytes([0x80, 0x80, 0x00, 0x00]), -1.1754943508222875E-38, '-Normalized');
  this.assertEquals(fromBytes([0xff, 0x7f, 0xff, 0xff]), -3.4028234663852886E+38, '-Normalized');
  this.assertEquals(fromBytes([0x80, 0x80, 0x00, 0x00]), -1.1754943508222875E-38, '-Normalized');

  // TODO: Denormalized values fail on Safari on iOS/ARM
  this.assertEquals(fromBytes([0x80, 0x7f, 0xff, 0xff]), -1.1754942106924411E-38, '-Denormalized');
  this.assertEquals(fromBytes([0x80, 0x00, 0x00, 0x01]), -1.4012984643248170E-45, '-Denormalized');

  this.assertEquals(fromBytes([0x80, 0x00, 0x00, 0x00]), -0, '-0');
  this.assertEquals(fromBytes([0x00, 0x00, 0x00, 0x00]), +0, '+0');

  // TODO: Denormalized values fail on Safari on iOS/ARM
  this.assertEquals(fromBytes([0x00, 0x00, 0x00, 0x01]), 1.4012984643248170E-45, '+Denormalized');
  this.assertEquals(fromBytes([0x00, 0x7f, 0xff, 0xff]), 1.1754942106924411E-38, '+Denormalized');

  this.assertEquals(fromBytes([0x00, 0x80, 0x00, 0x00]), 1.1754943508222875E-38, '+Normalized');
  this.assertEquals(fromBytes([0x7f, 0x7f, 0xff, 0xff]), 3.4028234663852886E+38, '+Normalized');

  this.assertEquals(fromBytes([0x7f, 0x80, 0x00, 0x00]), +Infinity, '+Infinity');

  this.assertEquals(isNaN(fromBytes([0x7f, 0x80, 0x00, 0x01])), true, 'S+NaN');
  this.assertEquals(isNaN(fromBytes([0x7f, 0xbf, 0xff, 0xff])), true, 'S+NaN');

  this.assertEquals(isNaN(fromBytes([0x7f, 0xc0, 0x00, 0x00])), true, 'Q+NaN');
  this.assertEquals(isNaN(fromBytes([0x7f, 0xff, 0xff, 0xff])), true, 'Q+NaN');
});

test('IEEE754 single precision packing', function () {

  function toBytes(v) {
    var uint8 = new Uint8Array(4), dv = new DataView(uint8.buffer);
    dv.setFloat32(0, v);
    var bytes = [];
    for (var i = 0; i < 4; i += 1) {
      bytes.push(uint8.get(i));
    }
    return bytes;
  }

  this.arrayEqual(toBytes(-Infinity), [0xff, 0x80, 0x00, 0x00], '-Infinity');

  this.arrayEqual(toBytes(-3.4028235677973366e+38), [0xff, 0x80, 0x00, 0x00], '-Overflow');
  this.arrayEqual(toBytes(-3.402824E+38), [0xff, 0x80, 0x00, 0x00], '-Overflow');

  this.arrayEqual(toBytes(-3.4028234663852886E+38), [0xff, 0x7f, 0xff, 0xff], '-Normalized');
  this.arrayEqual(toBytes(-1.1754943508222875E-38), [0x80, 0x80, 0x00, 0x00], '-Normalized');

  // TODO: Denormalized values fail on Safari iOS/ARM
  this.arrayEqual(toBytes(-1.1754942106924411E-38), [0x80, 0x7f, 0xff, 0xff], '-Denormalized');
  this.arrayEqual(toBytes(-1.4012984643248170E-45), [0x80, 0x00, 0x00, 0x01], '-Denormalized');

  this.arrayEqual(toBytes(-7.006492321624085e-46), [0x80, 0x00, 0x00, 0x00], '-Underflow');

  this.arrayEqual(toBytes(-0), [0x80, 0x00, 0x00, 0x00], '-0');
  this.arrayEqual(toBytes(0), [0x00, 0x00, 0x00, 0x00], '+0');

  this.arrayEqual(toBytes(7.006492321624085e-46), [0x00, 0x00, 0x00, 0x00], '+Underflow');

  // TODO: Denormalized values fail on Safari iOS/ARM
  this.arrayEqual(toBytes(1.4012984643248170E-45), [0x00, 0x00, 0x00, 0x01], '+Denormalized');
  this.arrayEqual(toBytes(1.1754942106924411E-38), [0x00, 0x7f, 0xff, 0xff], '+Denormalized');

  this.arrayEqual(toBytes(1.1754943508222875E-38), [0x00, 0x80, 0x00, 0x00], '+Normalized');
  this.arrayEqual(toBytes(3.4028234663852886E+38), [0x7f, 0x7f, 0xff, 0xff], '+Normalized');

  this.arrayEqual(toBytes(+3.402824E+38), [0x7f, 0x80, 0x00, 0x00], '+Overflow');
  this.arrayEqual(toBytes(+3.402824E+38), [0x7f, 0x80, 0x00, 0x00], '+Overflow');
  this.arrayEqual(toBytes(+Infinity), [0x7f, 0x80, 0x00, 0x00], '+Infinity');

  // Allow any NaN pattern (exponent all 1's, fraction non-zero)
  var nanbytes = toBytes(NaN),
      sign = extractbits(nanbytes, 31, 31),
      exponent = extractbits(nanbytes, 23, 30),
      fraction = extractbits(nanbytes, 0, 22);
  this.assertEquals(exponent === 255 && fraction !== 0, true, 'NaN');
});


test('IEEE754 double precision unpacking', function () {

  function fromBytes(bytes) {
    var uint8 = new Uint8Array(bytes),
        dv = new DataView(uint8.buffer);
    return dv.getFloat64(0);
  }

  this.assertEquals(isNaN(fromBytes([0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff])), true, 'Q-NaN');
  this.assertEquals(isNaN(fromBytes([0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01])), true, 'Q-NaN');

  this.assertEquals(isNaN(fromBytes([0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])), true, 'Indeterminate');

  this.assertEquals(isNaN(fromBytes([0xff, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff])), true, 'S-NaN');
  this.assertEquals(isNaN(fromBytes([0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01])), true, 'S-NaN');

  this.assertEquals(fromBytes([0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]), -Infinity, '-Infinity');

  this.assertEquals(fromBytes([0xff, 0xef, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff]), -1.7976931348623157E+308, '-Normalized');
  this.assertEquals(fromBytes([0x80, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]), -2.2250738585072014E-308, '-Normalized');

  // TODO: Denormalized values fail on Safari iOS/ARM
  this.assertEquals(fromBytes([0x80, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff]), -2.2250738585072010E-308, '-Denormalized');
  this.assertEquals(fromBytes([0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01]), -4.9406564584124654E-324, '-Denormalized');

  this.assertEquals(fromBytes([0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]), -0, '-0');
  this.assertEquals(fromBytes([0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]), +0, '+0');

  // TODO: Denormalized values fail on Safari iOS/ARM
  this.assertEquals(fromBytes([0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01]), 4.9406564584124654E-324, '+Denormalized');
  this.assertEquals(fromBytes([0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff]), 2.2250738585072010E-308, '+Denormalized');

  this.assertEquals(fromBytes([0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]), 2.2250738585072014E-308, '+Normalized');
  this.assertEquals(fromBytes([0x7f, 0xef, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff]), 1.7976931348623157E+308, '+Normalized');

  this.assertEquals(fromBytes([0x7f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]), +Infinity, '+Infinity');

  this.assertEquals(isNaN(fromBytes([0x7f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01])), true, 'S+NaN');
  this.assertEquals(isNaN(fromBytes([0x7f, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff])), true, 'S+NaN');

  this.assertEquals(isNaN(fromBytes([0x7f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])), true, 'Q+NaN');
  this.assertEquals(isNaN(fromBytes([0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff])), true, 'Q+NaN');
});


test('IEEE754 double precision packing', function () {

  function toBytes(v) {
    var uint8 = new Uint8Array(8),
        dv = new DataView(uint8.buffer);
    dv.setFloat64(0, v);
    var bytes = [];
    for (var i = 0; i < 8; i += 1) {
      bytes.push(uint8.get(i));
    }
    return bytes;
  }

  this.arrayEqual(toBytes(-Infinity), [0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00], '-Infinity');

  this.arrayEqual(toBytes(-1.7976931348623157E+308), [0xff, 0xef, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff], '-Normalized');
  this.arrayEqual(toBytes(-2.2250738585072014E-308), [0x80, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00], '-Normalized');

  // TODO: Denormalized values fail on Safari iOS/ARM
  this.arrayEqual(toBytes(-2.2250738585072010E-308), [0x80, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff], '-Denormalized');
  this.arrayEqual(toBytes(-4.9406564584124654E-324), [0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01], '-Denormalized');

  this.arrayEqual(toBytes(-0), [0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00], '-0');
  this.arrayEqual(toBytes(0), [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00], '+0');

  // TODO: Denormalized values fail on Safari iOS/ARM
  this.arrayEqual(toBytes(4.9406564584124654E-324), [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01], '+Denormalized');
  this.arrayEqual(toBytes(2.2250738585072010E-308), [0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff], '+Denormalized');

  this.arrayEqual(toBytes(2.2250738585072014E-308), [0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00], '+Normalized');
  this.arrayEqual(toBytes(1.7976931348623157E+308), [0x7f, 0xef, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff], '+Normalized');

  this.arrayEqual(toBytes(+Infinity), [0x7f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00], '+Infinity');

  // Allow any NaN pattern (exponent all 1's, fraction non-zero)
  var nanbytes = toBytes(NaN),
      sign = extractbits(nanbytes, 63, 63),
      exponent = extractbits(nanbytes, 52, 62),
      fraction = extractbits(nanbytes, 0, 51);
  this.assertEquals(exponent === 2047 && fraction !== 0, true, 'NaN');
});

test('Int32Array round trips', function () {
  var i32 = new Int32Array([0]);
  var data = [
    0,
    1,
      -1,
    123,
      -456,
    0x80000000 >> 0,
    0x7fffffff >> 0,
    0x12345678 >> 0,
    0x87654321 >> 0
  ];

  for (var i = 0; i < data.length; i += 1) {
    var datum = data[i];
    i32.set([datum]);
    this.assertEquals(datum, i32.get(0), String(datum));
  }
});


test('Int16Array round trips', function () {
  var i16 = new Int16Array([0]);
  var data = [
    0,
    1,
      -1,
    123,
      -456,
    0xffff8000 >> 0,
    0x00007fff >> 0,
    0x00001234 >> 0,
    0xffff8765 >> 0
  ];

  for (var i = 0; i < data.length; i += 1) {
    var datum = data[i];
    i16.set([datum]);
    this.assertEquals(datum, i16.get(0), String(datum));
  }
});


test('Int8Array round trips', function () {
  var i8 = new Int8Array([0]);
  var data = [
    0,
    1,
      -1,
    123,
      -45,
    0xffffff80 >> 0,
    0x0000007f >> 0,
    0x00000012 >> 0,
    0xffffff87 >> 0
  ];

  for (var i = 0; i < data.length; i += 1) {
    var datum = data[i];
    i8.set([datum]);
    this.assertEquals(datum, i8.get(0), String(datum));
  }
});

test('TypedArray setting', function () {

  var a = new Int32Array([1, 2, 3, 4, 5]);
  var b = new Int32Array(5);
  b.set(a);
  this.arrayEqual(b, [1, 2, 3, 4, 5], '1');
  this.raises(function () { b.set(a, 1); });

  b.set(new Int32Array([99, 98]), 2);
  this.arrayEqual(b, [1, 2, 99, 98, 5], '2');

  b.set(new Int32Array([99, 98, 97]), 2);
  this.arrayEqual(b, [1, 2, 99, 98, 97], '3');

  this.raises(function () { b.set(new Int32Array([99, 98, 97, 96]), 2); });
  this.raises(function () { b.set([101, 102, 103, 104], 4); });

  //  ab = [ 0, 1, 2, 3, 4, 5, 6, 7 ]
  //  a1 = [ ^, ^, ^, ^, ^, ^, ^, ^ ]
  //  a2 =             [ ^, ^, ^, ^ ]
  var ab = new ArrayBuffer(8);
  var a1 = new Uint8Array(ab);
  for (var i = 0; i < a1.length; i += 1) { a1.set([i], i); }
  var a2 = new Uint8Array(ab, 4);
  a1.set(a2, 2);
  this.arrayEqual(a1, [0, 1, 4, 5, 6, 7, 6, 7]);
  this.arrayEqual(a2, [6, 7, 6, 7]);
});


test('TypedArray.subarray', function () {

  var a = new Int32Array([1, 2, 3, 4, 5]);
  this.arrayEqual(a.subarray(3), [4, 5]);
  this.arrayEqual(a.subarray(1, 3), [2, 3]);
  this.arrayEqual(a.subarray(-3), [3, 4, 5]);
  this.arrayEqual(a.subarray(-3, -1), [3, 4]);
  this.arrayEqual(a.subarray(3, 2), []);
  this.arrayEqual(a.subarray(-2, -3), []);
  this.arrayEqual(a.subarray(4, 1), []);
  this.arrayEqual(a.subarray(-1, -4), []);
  this.arrayEqual(a.subarray(1).subarray(1), [3, 4, 5]);
  this.arrayEqual(a.subarray(1, 4).subarray(1, 2), [3]);

  var a = new Float32Array(16);
  a[0] = -1;
  a[15] = 1/8;
  this.assertEquals(a.length, 16);
  this.assertEquals(a.byteLength, a.length * a.BYTES_PER_ELEMENT);

  this.assertEquals(a.subarray(-12).length, 12, '[-12,)');
  this.arrayEqual(a.subarray(-16), a, '[-16,)');
  this.arrayEqual(a.subarray(12, 16), [0,0,0,1/8], '[12,16)');
  this.arrayEqual(a.subarray(0, 4), [-1,0,0,0],'[0,4)');
  this.arrayEqual(a.subarray(-16, -12), [-1,0,0,0],'[-16,-12)');
  this.assertEquals(a.subarray(0, -12).length, 4,'[0,-12)');
  this.arrayEqual(a.subarray(-10), a.subarray(6),'[-10,)');
});


test('DataView constructors', function () {

  var d = new DataView(new ArrayBuffer(8));

  d.setUint32(0, 0x12345678);
  this.assertEquals(d.getUint32(0), 0x12345678, 'big endian/big endian');

  d.setUint32(0, 0x12345678, true);
  this.assertEquals(d.getUint32(0, true), 0x12345678, 'little endian/little endian');

  d.setUint32(0, 0x12345678, true);
  this.assertEquals(d.getUint32(0), 0x78563412, 'little endian/big endian');

  d.setUint32(0, 0x12345678);
  this.assertEquals(d.getUint32(0, true), 0x78563412, 'big endian/little endian');

  // Chrome allows no arguments, throws if non-ArrayBuffer
  //stricterEqual(new DataView().buffer.byteLength, 0, 'no arguments');

  // Safari (iOS 5) does not
  //raises(function () { return new DataView(); }, TypeError, 'no arguments');

  // Chrome raises TypeError, Safari iOS5 raises isDOMException(INDEX_SIZE_ERR)
  this.raises(function () { return new DataView({}); }, 'non-ArrayBuffer argument');

  this.raises(function () { return new DataView("bogus"); }, TypeError, 'non-ArrayBuffer argument');
});



test('DataView accessors', function () {
  var u = new Uint8Array(8), d = new DataView(u.buffer);
  this.arrayEqual(u, [0, 0, 0, 0, 0, 0, 0, 0], '1');

  d.setUint8(0, 255);
  this.arrayEqual(u, [0xff, 0, 0, 0, 0, 0, 0, 0], '2');

  d.setInt8(1, -1);
  this.arrayEqual(u, [0xff, 0xff, 0, 0, 0, 0, 0, 0], '3');

  d.setUint16(2, 0x1234);
  this.arrayEqual(u, [0xff, 0xff, 0x12, 0x34, 0, 0, 0, 0]), '4';

  d.setInt16(4, -1);
  this.arrayEqual(u, [0xff, 0xff, 0x12, 0x34, 0xff, 0xff, 0, 0], '5');

  d.setUint32(1, 0x12345678);
  this.arrayEqual(u, [0xff, 0x12, 0x34, 0x56, 0x78, 0xff, 0, 0], '6');

  d.setInt32(4, -2023406815);
  this.arrayEqual(u, [0xff, 0x12, 0x34, 0x56, 0x87, 0x65, 0x43, 0x21], '7');
  
  d.setFloat32(2, 1.2E+38);
  this.arrayEqual(u, [0xff, 0x12, 0x7e, 0xb4, 0x8e, 0x52, 0x43, 0x21], '8');

  d.setFloat64(0, -1.2345678E+301);
  this.arrayEqual(u, [0xfe, 0x72, 0x6f, 0x51, 0x5f, 0x61, 0x77, 0xe5], '9');

  u.set([0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87]);
  this.assertEquals(d.getUint8(0), 128, '10');
  this.assertEquals(d.getInt8(1), -127, '11');
  this.assertEquals(d.getUint16(2), 33411, '12');
  this.assertEquals(d.getInt16(3), -31868, '13');
  this.assertEquals(d.getUint32(4), 2223343239, '14');
  this.assertEquals(d.getInt32(2), -2105310075, '15');
  this.assertEquals(d.getFloat32(2), -1.932478247535851e-37, '16');
  this.assertEquals(d.getFloat64(0), -3.116851295377095e-306, '17');

});


test('DataView endian', function () {
  var rawbuf = (new Uint8Array([0, 1, 2, 3, 4, 5, 6, 7])).buffer;
  var d;

  d = new DataView(rawbuf);
  this.assertEquals(d.byteLength, 8, 'buffer');
  this.assertEquals(d.byteOffset, 0, 'buffer');
  this.raises(function () { d.getUint8(-2); }); // Chrome bug for index -, DOMException, 'bounds for buffer'?
  this.raises(function () { d.getUint8(8); }, 'bounds for buffer');
  this.raises(function () { d.setUint8(-2, 0); }, 'bounds for buffer');
  this.raises(function () { d.setUint8(8, 0); }, 'bounds for buffer');

  d = new DataView(rawbuf, 2);
  this.assertEquals(d.byteLength, 6, 'buffer, byteOffset');
  this.assertEquals(d.byteOffset, 2, 'buffer, byteOffset');
  this.assertEquals(d.getUint8(5), 7, 'buffer, byteOffset');
  this.raises(function () { d.getUint8(-2); }, 'bounds for buffer, byteOffset');
  this.raises(function () { d.getUint8(6); }, 'bounds for buffer, byteOffset');
  this.raises(function () { d.setUint8(-2, 0); }, 'bounds for buffer, byteOffset');
  this.raises(function () { d.setUint8(6, 0); }, 'bounds for buffer, byteOffset');

  d = new DataView(rawbuf, 8);
  this.assertEquals(d.byteLength, 0, 'buffer, byteOffset');

  this.raises(function () { return new DataView(rawbuf, -1); }, 'invalid byteOffset');
  this.raises(function () { return new DataView(rawbuf, 9); }, 'invalid byteOffset');
  this.raises(function () { return new DataView(rawbuf, -1); }, 'invalid byteOffset');

  d = new DataView(rawbuf, 2, 4);
  this.assertEquals(d.byteLength, 4, 'buffer, byteOffset, length');
  this.assertEquals(d.byteOffset, 2, 'buffer, byteOffset, length');
  this.assertEquals(d.getUint8(3), 5, 'buffer, byteOffset, length');
  this.raises(function () { return d.getUint8(-2); }, 'bounds for buffer, byteOffset, length');
  this.raises(function () { d.getUint8(4); }, 'bounds for buffer, byteOffset, length');
  this.raises(function () { d.setUint8(-2, 0); }, 'bounds for buffer, byteOffset, length');
  this.raises(function () { d.setUint8(4, 0); }, 'bounds for buffer, byteOffset, length');

  this.raises(function () { return new DataView(rawbuf, 0, 9); }, 'invalid byteOffset+length');
  this.raises(function () { return new DataView(rawbuf, 8, 1); }, 'invalid byteOffset+length');
  this.raises(function () { return new DataView(rawbuf, 9, -1); }, 'invalid byteOffset+length');
});


test('Typed Array getters/setters', function () {
  // Only supported if Object.defineProperty() is fully supported on non-DOM objects.
  try {
    var o = {};
    Object.defineProperty(o, 'x', { get: function() { return 1; } });
    if (o.x !== 1) throw Error();
  } catch (_) {
    ok(true);
    return;
  }

  var bytes = new Uint8Array([1, 2, 3, 4]),
      uint32s = new Uint32Array(bytes.buffer);

  this.assertEquals(bytes[1], 2);
  uint32s[0] = 0xffffffff;
  this.assertEquals(bytes[1], 0xff);
});


test('Uint8ClampedArray', function () {
  this.assertEquals(Uint8ClampedArray.BYTES_PER_ELEMENT, 1, 'Uint8ClampedArray.BYTES_PER_ELEMENT');
  var a = new Uint8ClampedArray([-Infinity, -Number.MAX_VALUE, -1, -Number.MIN_VALUE, -0,
                                 0, Number.MIN_VALUE, 1, 1.1, 1.9, 255, 255.1, 255.9, 256, Number.MAX_VALUE, Infinity,
                                 NaN]);
  this.assertEquals(a.BYTES_PER_ELEMENT, 1);
  this.assertEquals(a.byteOffset, 0);
  this.assertEquals(a.byteLength, 17);
  this.arrayEqual(a, [0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0], "array test");
});

test('Regression Tests', function() {
  // Bug: https://github.com/inexorabletash/polyfill/issues/16
  var minFloat32 = 1.401298464324817e-45;
  var truncated = new Float32Array([-minFloat32 / 2 - Math.pow(2, -202)]).get(0);
  this.assertEquals(truncated, -minFloat32, 'smallest 32 bit float should not truncate to zero');
});

test('new TypedArray(ArrayBuffer).length Tests', function() {
  var uint8s = new Uint8Array([0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xff]),
      buffer = uint8s.buffer;

  this.assertEquals(buffer.byteLength, 8, 'buffer.length');

  var _this = this;
  [
    'Uint8Array', 'Uint16Array', 'Uint32Array', 'Int8Array', 'Int16Array', 'Int32Array',
    'Float32Array', 'Float64Array', 'Uint8ClampedArray'
  ].forEach(function(typeArrayName) {
    var typeArray = eval(typeArrayName);
    var a = new typeArray(buffer);
    _this.assertEquals(a.BYTES_PER_ELEMENT, typeArrayName.match(/\d+/)[0]/8, typeArrayName+'.BYTES_PER_ELEMENT');
    _this.assertEquals(a.byteLength, buffer.byteLength, typeArrayName+'.byteLength');
    _this.assertEquals(a.length, buffer.byteLength / typeArray.BYTES_PER_ELEMENT, typeArrayName+'.length');
  });
});

test('Native endianness check', function() {
  var buffer = ArrayBuffer(4);
  new Uint8Array(buffer).set([0xaa, 0xbb, 0xcc, 0xdd]);
  var endian = { aabbccdd: 'big', ddccbbaa: 'little' }[
    new Uint32Array(buffer)[0].toString(16)
  ];
  this.assertEquals(endian, 'little');
});

test('TypeArray byte order tests', function() {
  var uint8s = new Uint8Array([0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xff]),
      buffer = uint8s.buffer;

  this.arrayEqual(new Uint8Array(buffer), [0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xff], "Uint8Array");
  this.arrayEqual(new Uint16Array(buffer), [0x00ff, 0x0000, 0x0000, 0xff00], "Uint16Array");
  this.arrayEqual(new Uint32Array(buffer), [0x000000ff, 0xff000000], "Uint32Array");

  this.arrayEqual(new Int8Array(buffer), [-1,0,0,0,0,0,0,-1], "Int8Array");
  this.arrayEqual(new Int16Array(buffer), [255, 0, 0, -256], "Int16Array");

  this.arrayEqual(new Float32Array(buffer), [3.5733110840282835e-43, -1.7014118346046923e+38], "Float32Array");
  this.arrayEqual(new Float64Array(buffer), [-5.486124068793999e+303], "Float64Array");
});
