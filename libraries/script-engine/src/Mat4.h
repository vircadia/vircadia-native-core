//
//  Mat4.h
//  libraries/script-engine/src
//
//  Created by Anthony Thibault on 3/7/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Scriptable 4x4 Matrix class library.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Mat4_h
#define hifi_Mat4_h

#include <QObject>
#include <QString>
#include <QtScript/QScriptable>
#include <QVector>
#include <glm/glm.hpp>
#include "RegisteredMetaTypes.h"

/**jsdoc
 * @namespace Mat4
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 * @hifi-server-entity
 * @hifi-assignment-client
 */

/// Scriptable Mat4 object.  Used exclusively in the JavaScript API
class Mat4 : public QObject, protected QScriptable {
    Q_OBJECT

public slots:

    /**jsdoc
     * @function Mat4.multiply
     * @param {Mat4} m1
     * @param {Mat4} m2
     * @returns {Mat4}
     */
    glm::mat4 multiply(const glm::mat4& m1, const glm::mat4& m2) const;


    /**jsdoc
     * @function Mat4.createFromRotAndTrans
     * @param {Quat} rot
     * @param {Vec3} trans
     * @returns {Mat4}
     */
    glm::mat4 createFromRotAndTrans(const glm::quat& rot, const glm::vec3& trans) const;

    /**jsdoc
     * @function Mat4.createFromScaleRotAndTrans
     * @param {Vec3} scale
     * @param {Quat} rot
     * @param {Vec3} trans
     * @returns {Mat4}
     */
    glm::mat4 createFromScaleRotAndTrans(const glm::vec3& scale, const glm::quat& rot, const glm::vec3& trans) const;

    /**jsdoc
     * @function Mat4.createFromColumns
     * @param {Vec4} col0
     * @param {Vec4} col1
     * @param {Vec4} col2
     * @param {Vec4} col
     * @returns {Mat4}
     */
    glm::mat4 createFromColumns(const glm::vec4& col0, const glm::vec4& col1, const glm::vec4& col2, const glm::vec4& col3) const;

    /**jsdoc
     * @function Mat4.createFromArray
     * @param {number[]} numbers
     * @returns {Mat4}
     */
    glm::mat4 createFromArray(const QVector<float>& floats) const;


    /**jsdoc
     * @function Mat4.extractTranslation
     * @param {Mat4} m
     * @returns {Vec3}
     */
    glm::vec3 extractTranslation(const glm::mat4& m) const;

    /**jsdoc
     * @function Mat4.extractRotation
     * @param {Mat4} m
     * @returns {Vec3}
     */
    glm::quat extractRotation(const glm::mat4& m) const;

    /**jsdoc
     * @function Mat4.extractScale
     * @param {Mat4} m
     * @returns {Vec3}
     */
    glm::vec3 extractScale(const glm::mat4& m) const;


    /**jsdoc
     * @function Mat4.transformPoint
     * @param {Mat4} m
     * @param {Vec3} point
     * @returns {Vec3}
     */
    glm::vec3 transformPoint(const glm::mat4& m, const glm::vec3& point) const;

    /**jsdoc
     * @function Mat4.transformVector
     * @param {Mat4} m
     * @param {Vec3} vector
     * @returns {Vec3}
     */
    glm::vec3 transformVector(const glm::mat4& m, const glm::vec3& vector) const;


    /**jsdoc
     * @function Mat4.inverse
     * @param {Mat4} m
     * @returns {Mat4}
     */
    glm::mat4 inverse(const glm::mat4& m) const;


    /**jsdoc
     * @function Mat4.getFront
     * @param {Mat4} m
     * @returns {Vec3}
     */
    // redundant, calls getForward which better describes the returned vector as a direction
    glm::vec3 getFront(const glm::mat4& m) const { return getForward(m); }

    /**jsdoc
     * @function Mat4.getForward
     * @param {Mat4} m
     * @returns {Vec3}
     */
    glm::vec3 getForward(const glm::mat4& m) const;

    /**jsdoc
     * @function Mat4.getRight
     * @param {Mat4} m
     * @returns {Vec3}
     */
    glm::vec3 getRight(const glm::mat4& m) const;

    /**jsdoc
     * @function Mat4.getUp
     * @param {Mat4} m
     * @returns {Vec3}
     */
    glm::vec3 getUp(const glm::mat4& m) const;

    /**jsdoc
     * @function Mat4.print
     * @param {string} label
     * @param {Mat4} m
     * @param {boolean} [transpose=false]
     */
    void print(const QString& label, const glm::mat4& m, bool transpose = false) const;
};

#endif // hifi_Mat4_h
