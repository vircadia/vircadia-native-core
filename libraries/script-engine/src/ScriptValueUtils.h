//
//  ScriptValueUtils.h
//  libraries/shared/src
//
//  Created by Anthony Thibault on 4/15/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Utilities for working with QtScriptValues
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @addtogroup ScriptEngine
/// @{

#ifndef hifi_ScriptValueUtils_h
#define hifi_ScriptValueUtils_h

#include <QtCore/QVector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "ScriptValue.h"

bool isListOfStrings(const ScriptValue& value);


void registerMetaTypes(ScriptEngine* engine);

// Mat4
/*@jsdoc
 * A 4 x 4 matrix, typically containing a scale, rotation, and translation transform. See also the {@link Mat4(0)|Mat4} object.
 *
 * @typedef {object} Mat4
 * @property {number} r0c0 - Row 0, column 0 value.
 * @property {number} r1c0 - Row 1, column 0 value.
 * @property {number} r2c0 - Row 2, column 0 value.
 * @property {number} r3c0 - Row 3, column 0 value.
 * @property {number} r0c1 - Row 0, column 1 value.
 * @property {number} r1c1 - Row 1, column 1 value.
 * @property {number} r2c1 - Row 2, column 1 value.
 * @property {number} r3c1 - Row 3, column 1 value.
 * @property {number} r0c2 - Row 0, column 2 value.
 * @property {number} r1c2 - Row 1, column 2 value.
 * @property {number} r2c2 - Row 2, column 2 value.
 * @property {number} r3c2 - Row 3, column 2 value.
 * @property {number} r0c3 - Row 0, column 3 value.
 * @property {number} r1c3 - Row 1, column 3 value.
 * @property {number} r2c3 - Row 2, column 3 value.
 * @property {number} r3c3 - Row 3, column 3 value.
 */
ScriptValue mat4toScriptValue(ScriptEngine* engine, const glm::mat4& mat4);
void mat4FromScriptValue(const ScriptValue& object, glm::mat4& mat4);

/*@jsdoc
* A 2-dimensional vector.
*
* @typedef {object} Vec2
* @property {number} x - X-coordinate of the vector. Synonyms: <code>u</code>.
* @property {number} y - Y-coordinate of the vector. Synonyms: <code>v</code>.
* @example <caption>Vec2s can be set in multiple ways and modified with their aliases, but still stringify in the same way</caption>
* Entities.editEntity(<id>, { materialMappingPos: { x: 0.1, y: 0.2 }});          // { x: 0.1, y: 0.2 }
* Entities.editEntity(<id>, { materialMappingPos: { u: 0.3, v: 0.4 }});          // { x: 0.3, y: 0.4 }
* Entities.editEntity(<id>, { materialMappingPos: [0.5, 0.6] });                 // { x: 0.5, y: 0.6 }
* Entities.editEntity(<id>, { materialMappingPos: 0.7 });                        // { x: 0.7, y: 0.7 }
* var color = Entities.getEntityProperties(<id>).materialMappingPos;             // { x: 0.7, y: 0.7 }
* color.v = 0.8;                                                                 // { x: 0.7, y: 0.8 }
*/
ScriptValue vec2ToScriptValue(ScriptEngine* engine, const glm::vec2& vec2);
void vec2FromScriptValue(const ScriptValue& object, glm::vec2& vec2);

/*@jsdoc
* A 3-dimensional vector. See also the {@link Vec3(0)|Vec3} object.
*
* @typedef {object} Vec3
* @property {number} x - X-coordinate of the vector. Synonyms: <code>r</code>, <code>red</code>.
* @property {number} y - Y-coordinate of the vector. Synonyms: <code>g</code>, <code>green</code>.
* @property {number} z - Z-coordinate of the vector. Synonyms: <code>b</code>, <code>blue</code>.
* @example <caption>Vec3 values can be set in multiple ways and modified with their aliases, but still stringify in the same 
*     way.</caption>
* Entities.editEntity(<id>, { position: { x: 1, y: 2, z: 3 }});                 // { x: 1, y: 2, z: 3 }
* Entities.editEntity(<id>, { position: { r: 4, g: 5, b: 6 }});                 // { x: 4, y: 5, z: 6 }
* Entities.editEntity(<id>, { position: { red: 7, green: 8, blue: 9 }});        // { x: 7, y: 8, z: 9 }
* Entities.editEntity(<id>, { position: [10, 11, 12] });                        // { x: 10, y: 11, z: 12 }
* Entities.editEntity(<id>, { position: 13 });                                  // { x: 13, y: 13, z: 13 }
* var position = Entities.getEntityProperties(<id>).position;                   // { x: 13, y: 13, z: 13 }
* position.g = 14;                                                              // { x: 13, y: 14, z: 13 }
* position.blue = 15;                                                           // { x: 13, y: 14, z: 15 }
* Entities.editEntity(<id>, { position: "red"});                                // { x: 255, y: 0, z: 0 }
* Entities.editEntity(<id>, { position: "#00FF00"});                            // { x: 0, y: 255, z: 0 }
*/
ScriptValue vec3ToScriptValue(ScriptEngine* engine, const glm::vec3& vec3);
ScriptValue vec3ColorToScriptValue(ScriptEngine* engine, const glm::vec3& vec3);
void vec3FromScriptValue(const ScriptValue& object, glm::vec3& vec3);

/*@jsdoc
 * A color vector. See also the {@link Vec3(0)|Vec3} object.
 *
 * @typedef {object} Color
 * @property {number} red - Red component value. Integer in the range <code>0</code> - <code>255</code>.  Synonyms: <code>r</code>, <code>x</code>.
 * @property {number} green - Green component value. Integer in the range <code>0</code> - <code>255</code>.  Synonyms: <code>g</code>, <code>y</code>.
 * @property {number} blue - Blue component value. Integer in the range <code>0</code> - <code>255</code>.  Synonyms: <code>b</code>, <code>z</code>.
 * @example <caption>Colors can be set in multiple ways and modified with their aliases, but still stringify in the same way</caption>
 * Entities.editEntity(<id>, { color: { x: 1, y: 2, z: 3 }});                 // { red: 1, green: 2, blue: 3 }
 * Entities.editEntity(<id>, { color: { r: 4, g: 5, b: 6 }});                 // { red: 4, green: 5, blue: 6 }
 * Entities.editEntity(<id>, { color: { red: 7, green: 8, blue: 9 }});        // { red: 7, green: 8, blue: 9 }
 * Entities.editEntity(<id>, { color: [10, 11, 12] });                        // { red: 10, green: 11, blue: 12 }
 * Entities.editEntity(<id>, { color: 13 });                                  // { red: 13, green: 13, blue: 13 }
 * var color = Entities.getEntityProperties(<id>).color;                      // { red: 13, green: 13, blue: 13 }
 * color.g = 14;                                                              // { red: 13, green: 14, blue: 13 }
 * color.blue = 15;                                                           // { red: 13, green: 14, blue: 15 }
 * Entities.editEntity(<id>, { color: "red"});                                // { red: 255, green: 0, blue: 0 }
 * Entities.editEntity(<id>, { color: "#00FF00"});                            // { red: 0, green: 255, blue: 0 }
 */
/*@jsdoc
 * A color vector with real values. Values may also be <code>null</code>. See also the {@link Vec3(0)|Vec3} object.
 *
 * @typedef {object} ColorFloat
 * @property {number} red - Red component value. Real in the range <code>0</code> - <code>255</code>.  Synonyms: <code>r</code>, <code>x</code>.
 * @property {number} green - Green component value. Real in the range <code>0</code> - <code>255</code>.  Synonyms: <code>g</code>, <code>y</code>.
 * @property {number} blue - Blue component value. Real in the range <code>0</code> - <code>255</code>.  Synonyms: <code>b</code>, <code>z</code>.
 * @example <caption>ColorFloats can be set in multiple ways and modified with their aliases, but still stringify in the same way</caption>
 * Entities.editEntity(<id>, { color: { x: 1, y: 2, z: 3 }});                 // { red: 1, green: 2, blue: 3 }
 * Entities.editEntity(<id>, { color: { r: 4, g: 5, b: 6 }});                 // { red: 4, green: 5, blue: 6 }
 * Entities.editEntity(<id>, { color: { red: 7, green: 8, blue: 9 }});        // { red: 7, green: 8, blue: 9 }
 * Entities.editEntity(<id>, { color: [10, 11, 12] });                        // { red: 10, green: 11, blue: 12 }
 * Entities.editEntity(<id>, { color: 13 });                                  // { red: 13, green: 13, blue: 13 }
 * var color = Entities.getEntityProperties(<id>).color;                      // { red: 13, green: 13, blue: 13 }
 * color.g = 14;                                                              // { red: 13, green: 14, blue: 13 }
 * color.blue = 15;                                                           // { red: 13, green: 14, blue: 15 }
 * Entities.editEntity(<id>, { color: "red"});                                // { red: 255, green: 0, blue: 0 }
 * Entities.editEntity(<id>, { color: "#00FF00"});                            // { red: 0, green: 255, blue: 0 }
 */
ScriptValue u8vec3ToScriptValue(ScriptEngine* engine, const glm::u8vec3& vec3);
ScriptValue u8vec3ColorToScriptValue(ScriptEngine* engine, const glm::u8vec3& vec3);
void u8vec3FromScriptValue(const ScriptValue& object, glm::u8vec3& vec3);

/*@jsdoc
 * A 4-dimensional vector.
 *
 * @typedef {object} Vec4
 * @property {number} x - X-coordinate of the vector.
 * @property {number} y - Y-coordinate of the vector.
 * @property {number} z - Z-coordinate of the vector.
 * @property {number} w - W-coordinate of the vector.
 */
ScriptValue vec4toScriptValue(ScriptEngine* engine, const glm::vec4& vec4);
void vec4FromScriptValue(const ScriptValue& object, glm::vec4& vec4);

// Quaternions
ScriptValue quatToScriptValue(ScriptEngine* engine, const glm::quat& quat);
void quatFromScriptValue(const ScriptValue& object, glm::quat& quat);

/*@jsdoc
 * Defines a rectangular portion of an image or screen, or similar.
 * @typedef {object} Rect
 * @property {number} x - Left, x-coordinate value.
 * @property {number} y - Top, y-coordinate value.
 * @property {number} width - Width of the rectangle.
 * @property {number} height - Height of the rectangle.
 */
class QRect;
ScriptValue qRectToScriptValue(ScriptEngine* engine, const QRect& rect);
void qRectFromScriptValue(const ScriptValue& object, QRect& rect);

class QRectF;
ScriptValue qRectFToScriptValue(ScriptEngine* engine, const QRectF& rect);
void qRectFFromScriptValue(const ScriptValue& object, QRectF& rect);

// QColor
class QColor;
ScriptValue qColorToScriptValue(ScriptEngine* engine, const QColor& color);
void qColorFromScriptValue(const ScriptValue& object, QColor& color);

class QUrl;
ScriptValue qURLToScriptValue(ScriptEngine* engine, const QUrl& url);
void qURLFromScriptValue(const ScriptValue& object, QUrl& url);

// vector<vec3>
Q_DECLARE_METATYPE(QVector<glm::vec3>)
ScriptValue qVectorVec3ToScriptValue(ScriptEngine* engine, const QVector<glm::vec3>& vector);
ScriptValue qVectorVec3ColorToScriptValue(ScriptEngine* engine, const QVector<glm::vec3>& vector);
void qVectorVec3FromScriptValue(const ScriptValue& array, QVector<glm::vec3>& vector);
QVector<glm::vec3> qVectorVec3FromScriptValue(const ScriptValue& array);

// vector<quat>
Q_DECLARE_METATYPE(QVector<glm::quat>)
ScriptValue qVectorQuatToScriptValue(ScriptEngine* engine, const QVector<glm::quat>& vector);
void qVectorQuatFromScriptValue(const ScriptValue& array, QVector<glm::quat>& vector);
QVector<glm::quat> qVectorQuatFromScriptValue(const ScriptValue& array);

// vector<bool>
ScriptValue qVectorBoolToScriptValue(ScriptEngine* engine, const QVector<bool>& vector);
void qVectorBoolFromScriptValue(const ScriptValue& array, QVector<bool>& vector);
QVector<bool> qVectorBoolFromScriptValue(const ScriptValue& array);

// vector<float>
ScriptValue qVectorFloatToScriptValue(ScriptEngine* engine, const QVector<float>& vector);
void qVectorFloatFromScriptValue(const ScriptValue& array, QVector<float>& vector);
QVector<float> qVectorFloatFromScriptValue(const ScriptValue& array);

// vector<uint32_t>
ScriptValue qVectorIntToScriptValue(ScriptEngine* engine, const QVector<uint32_t>& vector);
void qVectorIntFromScriptValue(const ScriptValue& array, QVector<uint32_t>& vector);

ScriptValue qVectorQUuidToScriptValue(ScriptEngine* engine, const QVector<QUuid>& vector);
void qVectorQUuidFromScriptValue(const ScriptValue& array, QVector<QUuid>& vector);
QVector<QUuid> qVectorQUuidFromScriptValue(const ScriptValue& array);

class AACube;
ScriptValue aaCubeToScriptValue(ScriptEngine* engine, const AACube& aaCube);
void aaCubeFromScriptValue(const ScriptValue& object, AACube& aaCube);

class PickRay;
ScriptValue pickRayToScriptValue(ScriptEngine* engine, const PickRay& pickRay);
void pickRayFromScriptValue(const ScriptValue& object, PickRay& pickRay);

class Collision;
ScriptValue collisionToScriptValue(ScriptEngine* engine, const Collision& collision);
void collisionFromScriptValue(const ScriptValue& object, Collision& collision);

/*@jsdoc
 * UUIDs (Universally Unique IDentifiers) are used to uniquely identify entities, avatars, and the like. They are represented 
 * in JavaScript as strings in the format, <code>"{nnnnnnnn-nnnn-nnnn-nnnn-nnnnnnnnnnnn}"</code>, where the "n"s are
 * hexadecimal digits.
 * @typedef {string} Uuid
 */
//Q_DECLARE_METATYPE(QUuid) // don't need to do this for QUuid since it's already a meta type
ScriptValue quuidToScriptValue(ScriptEngine* engine, const QUuid& uuid);
void quuidFromScriptValue(const ScriptValue& object, QUuid& uuid);

//Q_DECLARE_METATYPE(QSizeF) // Don't need to to this becase it's arleady a meta type
class QSizeF;
ScriptValue qSizeFToScriptValue(ScriptEngine* engine, const QSizeF& qSizeF);
void qSizeFFromScriptValue(const ScriptValue& object, QSizeF& qSizeF);

class AnimationDetails;
ScriptValue animationDetailsToScriptValue(ScriptEngine* engine, const AnimationDetails& event);
void animationDetailsFromScriptValue(const ScriptValue& object, AnimationDetails& event);

class MeshProxy;
ScriptValue meshToScriptValue(ScriptEngine* engine, MeshProxy* const& in);
void meshFromScriptValue(const ScriptValue& value, MeshProxy*& out);

class MeshProxyList;
ScriptValue meshesToScriptValue(ScriptEngine* engine, const MeshProxyList& in);
void meshesFromScriptValue(const ScriptValue& value, MeshProxyList& out);

class MeshFace;
ScriptValue meshFaceToScriptValue(ScriptEngine* engine, const MeshFace& meshFace);
void meshFaceFromScriptValue(const ScriptValue& object, MeshFace& meshFaceResult);
ScriptValue qVectorMeshFaceToScriptValue(ScriptEngine* engine, const QVector<MeshFace>& vector);
void qVectorMeshFaceFromScriptValue(const ScriptValue& array, QVector<MeshFace>& result);

enum class StencilMaskMode;
ScriptValue stencilMaskModeToScriptValue(ScriptEngine* engine, const StencilMaskMode& stencilMode);
void stencilMaskModeFromScriptValue(const ScriptValue& object, StencilMaskMode& stencilMode);

class MiniPromise;
void promiseFromScriptValue(const ScriptValue& object, std::shared_ptr<MiniPromise>& promise);
ScriptValue promiseToScriptValue(ScriptEngine* engine, const std::shared_ptr<MiniPromise>& promise);

class EntityItemID;
ScriptValue EntityItemIDtoScriptValue(ScriptEngine* engine, const EntityItemID& properties);
void EntityItemIDfromScriptValue(const ScriptValue& object, EntityItemID& properties);
QVector<EntityItemID> qVectorEntityItemIDFromScriptValue(const ScriptValue& array);

#endif  // #define hifi_ScriptValueUtils_h

/// @}
