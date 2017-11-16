//
//  Quat.h
//  libraries/script-engine/src
//
//  Created by Brad Hefta-Gaub on 1/29/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Scriptable Quaternion class library.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Quat_h
#define hifi_Quat_h

#include <glm/gtc/quaternion.hpp>

#include <QObject>
#include <QString>
#include <QtScript/QScriptable>

/**jsdoc
 * A quaternion value. See also the [Quat]{@link Quat(0)} object.
 * @typedef {object} Quat
 * @property {number} x imaginary component i.
 * @property {number} y imaginary component j.
 * @property {number} z imaginary component k.
 * @property {number} w real component.
 */

/**jsdoc
 * The Quat API provides facilities for generating and manipulating quaternions.
 * @namespace Quat
 * @variation 0
 * @property IDENTITY {Quat} The identity rotation, i.e., no rotation.
 * @example <caption>Print the <code>IDENTITY</code> value.</caption>
 * print(JSON.stringify(Quat.IDENTITY)); // { x: 0, y: 0, z: 0, w: 1 }
 * print(JSON.stringify(Quat.safeEulerAngles(Quat.IDENTITY))); // { x: 0, y: 0, z: 0 }
 */

/// Scriptable interface a Quaternion helper class object. Used exclusively in the JavaScript API
class Quat : public QObject, protected QScriptable {
    Q_OBJECT
    Q_PROPERTY(glm::quat IDENTITY READ IDENTITY CONSTANT)

public slots:

    /**jsdoc
     * Multiply two quaternions.
     * @function Quat(0).multiply
     * @param {Quat} q1 - The first quaternion.
     * @param {Quat} q2 - The second quaternion.
     * @returns {Quat} <code>q1</code> multiplied with <code>q2</code>.
     */
    glm::quat multiply(const glm::quat& q1, const glm::quat& q2);

    /**jsdoc
     * Normalizes a quaternion.
     * @function Quat(0).normalize
     * @param {Quat} q - The quaternion to normalize.
     * @returns {Quat} <code>q</code> normalized to have unit length.
     */
    glm::quat normalize(const glm::quat& q);

    /**jsdoc
    * Calculate the conjugate of a quaternion. Synonym for [Quat.inverse]{@link Quat(0).inverse}.
    * @function Quat(0).conjugate
    * @param {Quat} q - The quaternion to conjugate.
    * @returns {Quat} The conjugate (inverse) of <code>q</code>.
    * @example <caption>A quaternion multiplied by its conjugate is a zero rotation.</caption>
    * var quaternion = Quat.fromPitchYawRollDegrees(10, 20, 30);
    * Quat.print("quaternion", quaternion, true); // dvec3(10.000000, 20.000004, 30.000004)
    * var conjugate = Quat.conjugate(quaternion);
    * Quat.print("conjugate", conjugate, true); // dvec3(1.116056, -22.242186, -28.451778)
    * var identity = Quat.multiply(conjugate, quaternion);
    * Quat.print("identity", identity, true); // dvec3(0.000000, 0.000000, 0.000000)
    */
    glm::quat conjugate(const glm::quat& q);

    /**jsdoc
     * Calculate a camera orientation given eye position, point of interest, and "up" direction. The camera's negative z-axis is
     * the forward direction. The result has zero roll about its forward direction with respect to the given "up" direction.
     * @function Quat(0).lookAt
     * @param {Vec3} eye - The eye position.
     * @param {Vec3} center - A point that the eye's looking at; the center of the scene.
     * @param {Vec3} up - The "up" direction.
     * @returns {Quat} A quaternion that orients the negative z-axis to point along the eye-to-center vector and the x-axis to
     * be the cross product of the eye-to-center and up vectors.
     */
    glm::quat lookAt(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up);

    /**jsdoc
    * Calculate a camera orientation given eye position and point of interest. The camera's negative z-axis is the forward 
    * direction. The result has zero roll about its forward direction.
    * @function Quat(0).lookAtSimple
    * @param {Vec3} eye - The eye position.
    * @param {Vec3} center - A point that the eye's looking at; the center of the scene.
    * @returns {Quat} A quaternion that orients the negative z-axis to point along the eye-to-center vector and the x-axis to be the
    *     cross product of the eye-to-center and an "up" vector. The "up" vector is the y-axis unless the eye-to-center vector
    *     is nearly aligned with it, in which case the x-axis is used as the "up" vector.
    * @example <caption>Demonstrate that the calculation orients the negative z-axis.</caption>
    * var eye = Vec3.ZERO;
    * var center = { x: 0, y: 0, z: 1 }
    * var orientation = Quat.lookAtSimple(eye, center);
    * var eulers = Quat.safeEulerAngles(orientation);
    * print(JSON.stringify(eulers)); // {"x":0,"y":-180,"z":0}
    */
    glm::quat lookAtSimple(const glm::vec3& eye, const glm::vec3& center);

    /**jsdoc
    * Calculate the shortest rotation from a first vector onto a second.
    * @function Quat(0).rotationBetween
    * @param {Vec3} v1 - The first vector.
    * @param {Vec3} v2 - The second vector.
    * @returns {Quat} The rotation from <code>v1</code> onto <code>v2</code>.
    */
    glm::quat rotationBetween(const glm::vec3& v1, const glm::vec3& v2);

    /**jsdoc
     * Generate a quaternion from a {@link Vec3} of Euler angles in degrees.
     * @function Quat(0).fromVec3Degrees
     * @param {Vec3} vector - A vector of three Euler angles in degrees.
     * @returns {Quat} A quaternion created from the Euler angles in <code>vector</code>.
     */
    glm::quat fromVec3Degrees(const glm::vec3& vec3);

    /**jsdoc
     * Generate a quaternion from a {@link Vec3} of Euler angles in radians.
     * @function Quat(0).fromVec3Radians
     * @param {Vec3} vector - A vector of three Euler angles in radians.
     * @returns {Quat} A quaternion created using the Euler angles in <code>vector</code>.
     */
    glm::quat fromVec3Radians(const glm::vec3& vec3);

    /**jsdoc
    * Generate a quaternion from pitch, yaw, and roll values in degrees.
    * @function Quat(0).fromPitchYawRollDegrees
    * @param {number} pitch - The pitch angle in degrees.
    * @param {number} yaw - The yaw angle in degrees.
    * @param {number} roll - The roll angle in degrees.
    * @returns {Quat} A quaternion created using the <code>pitch</code>, <code>yaw</code>, and <code>roll</code> angles.
    */
    glm::quat fromPitchYawRollDegrees(float pitch, float yaw, float roll);

    /**jsdoc
    * Generate a quaternion from pitch, yaw, and roll values in radians.
    * @function Quat(0).fromPitchYawRollRadians
    * @param {number} pitch - The pitch angle in radians.
    * @param {number} yaw - The yaw angle in radians.
    * @param {number} roll - The roll angle in radians.
    * @returns {Quat} A quaternion created from the <code>pitch</code>, <code>yaw</code>, and <code>roll</code> angles.
    */
    glm::quat fromPitchYawRollRadians(float pitch, float yaw, float roll);

    /**jsdoc
     * Calculate the inverse of a quaternion. Synonym for [Quat.conjugate]{@link Quat(0).conjugate}.
     * @function Quat(0).inverse
     * @param {Quat} q - The quaternion.
     * @returns {Quat} The inverse (conjugate) of <code>q</code>.
     */
    glm::quat inverse(const glm::quat& q);

    /**jsdoc
     * Get the negative z-axis for the quaternion. This is a synonym for [Quat.getForward]{@link Quat(0).getForward}.
     * @function Quat(0).getFront
     * @param {Quat} orientation - A quaternion representing an orientation.
     * @returns {Vec3} The negative z-axis rotated by <code>orientation</code>.
     */
    glm::vec3 getFront(const glm::quat& orientation) { return getForward(orientation); }

    /**jsdoc
     * Get the negative z-axis for the quaternion. This is a synonym for [Quat.getFront]{@link Quat(0).getFront}.
     * @function Quat(0).getForward
     * @param {Quat} orientation - A quaternion representing an orientation.
     * @returns {Vec3} The negative z-axis rotated by <code>orientation</code>.
     * @example <caption>Demonstrate that the "forward" vector is for the negative z-axis.</caption>
     * var forward = Quat.getForward(Quat.IDENTITY);
     * print(JSON.stringify(forward)); // {"x":0,"y":0,"z":-1}
     */
    glm::vec3 getForward(const glm::quat& orientation);

    /**jsdoc
     * Get the x-axis for the quaternion.
     * @function Quat(0).getRight
     * @param {Quat} orientation - A quaternion representing an orientation.
     * @returns {Vec3} The x-axis rotated by <code>orientation</code>.
     */
    glm::vec3 getRight(const glm::quat& orientation);

    /**jsdoc
     * Get the y-axis for the quaternion.
     * @function Quat(0).getUp
     * @param {Quat} orientation - A quaternion representing an orientation.
     * @returns {Vec3} The y-axis rotated by <code>orientation</code>.
     */
    glm::vec3 getUp(const glm::quat& orientation);

    /**jsdoc
     * Calculate the Euler angles for the quaternion, in degrees. (The "safe" in the name signifies that the angle results will
     * not be garbage even when the rotation is particularly difficult to decompose.)
     * @function Quat(0).safeEulerAngles
     * @param {Quat} orientation - A quaternion representing an orientation.
     * @returns {Vec3} A {@link Vec3} of Euler angles for the <code>orientation</code>, in degrees.
     */
    glm::vec3 safeEulerAngles(const glm::quat& orientation);

    /**jsdoc
     * Generate a quaternion given an angle to rotate through and an axis to rotate about.
     * @function Quat(0).angleAxis
     * @param {number} angle - The angle to rotate through, in degrees.
     * @param {Vec3} axis - The axis to rotate about.
     * @returns {Quat} A quaternion that is a rotation through <code>angle</code> degrees about the <code>axis</code>. 
     * <strong>WARNING:</strong> This value is in degrees whereas the value returned by [Quat.angle]{@link Quat(0).angleAxis} is
     * in radians.
     */
    glm::quat angleAxis(float angle, const glm::vec3& v);

    /**jsdoc
     * Get the rotation axis for a quaternion.
     * @function Quat(0).axis
     * @param {Quat} q - The quaternion.
     * @returns {Vec3} The normalized rotation axis for <code>q</code>.
     */
    glm::vec3 axis(const glm::quat& orientation);

    /**jsdoc
     * Get the rotation angle for a quaternion.
     * @function Quat(0).angle
     * @param {Quat} q - The quaternion.
     * @returns {Vec3} The rotation angle for <code>q</code>, in radians. <strong>WARNING:</strong> This value is in radians 
     * whereas the value used by [Quat.angleAxis]{@link Quat(0).angleAxis} is in degrees.
     */
    float angle(const glm::quat& orientation);

    // spherical linear interpolation
    // alpha: 0.0 to 1.0?
    /**jsdoc
     * Compute a spherical linear interpolation between two rotations, safely handling two rotations that are very similar.
     * See also, [Quat.slerp]{@link Quat(0).slerp}.
     * @function Quat(0).mix
     * @param {Quat} q1 - The beginning rotation.
     * @param {Quat} q2 - The ending rotation.
     * @param {number} alpha - The mixture coefficient between 0.0 and 1.0.
     * @example <caption>Animate between one rotation and another.</caption>
     * var dt = amountOfTimeThatHasPassed;
     * var mixFactor = amountOfTimeThatHasPassed / TIME_TO_COMPLETE;
     * if (mixFactor) > 1) {
     *     mixFactor = 1;
     * }
     * var newRotation = Quat.mix(startRotation, endRotation, mixFactor);
     */
    glm::quat mix(const glm::quat& q1, const glm::quat& q2, float alpha);

    /**jsdoc
     * Compute a spherical linear interpolation between two rotations.
     * See also, [Quat.mix]{@link Quat(0).mix}.
     * @function Quat(0).slerp
     * @param {Quat} q1 - The beginning rotation.
     * @param {Quat} q2 - The ending rotation.
     * @param {number} alpha - The mixture coefficient between 0.0 and 1.0.
     */
    glm::quat slerp(const glm::quat& q1, const glm::quat& q2, float alpha);

    /**jsdoc
     * Compute a spherical quadratic interpolation between two rotations.
     * @function Quat(0).squad
     * @param {Quat} q1 - The rotation before to the beginning rotation.
     * @param {Quat} q2 - The beginning rotation.
     * @param {Quat} q3 - The ending rotation.
     * @param {Quat} q4 - The rotation after the ending rotation.
     * @param {number} alpha - The mixture coefficient between 0.0 and 1.0.
     */
    glm::quat squad(const glm::quat& q1, const glm::quat& q2, const glm::quat& s1, const glm::quat& s2, float h);

    /**jsdoc
     * Calculate the dot product of two quaternions.
     * A zero value means the rotations are completely orthogonal to each other. The closer the rotations are to each other the
     * more non-zero the value is (either positive or negative). Identical rotations have a dot product of +/- 1.
     * @function Quat(0).dot
     * @param {Quat} q1 - The first quaternion.
     * @param {Quat} q2 - The second quaternion.
     * @returns {Quat} The dot product of <code>q1</code> and <code>q2</code>.
     */
    float dot(const glm::quat& q1, const glm::quat& q2);
    
    /**jsdoc
     * Print to the program log a text label followed by a quaternion's pitch, yaw, and roll Euler angles.
     * @function Quat(0).print
     * @param {string} label - The label to print.
     * @param {Quat} q - The quaternion to print.
     * @param {boolean} [asDegrees=false] - Whether to print the angles in degrees.
     * @example <caption>Two ways of printing a label plus a quaternion's Euler angles.</caption>
     * var quaternion = Quat.fromPitchYawRollDegrees(0, 45, 0);
     *
     * // Quaternion: dvec3(0.000000, 45.000004, 0.000000)
     * Quat.print("Quaternion:", quaternion,  true);
     *
     * // Quaternion: {"x":0,"y":45.000003814697266,"z":0}
     * print("Quaternion: " + JSON.stringify(Quat.safeEulerAngles(quaternion)));
     */
    void print(const QString& label, const glm::quat& q, bool asDegrees = false);

    /**jsdoc
     * Test whether two quaternions are equal. <strong>Note:</strong> The quaternions must be exactly equal in order for 
     * <code>true</code> to be returned; it is often better to use [Quat.dot]{@link Quat(0).dot} and test for closeness to +/-1.
     * @function Quat(0).equal
     * @param {Quat} q1 - The first quaternion.
     * @param {Quat} q2 - The second quaternion.
     * @returns {boolean} <code>true</code> if the quaternions are equal, otherwise <code>false</code>.
     * @example <caption>Testing quaternions for equality.</caption>
     * var q1 = Quat.fromPitchYawRollDegrees(0, 0, 0);
     * var q2 = Quat.fromPitchYawRollDegrees(0, 0, 0);
     * print(Quat.equal(q1, q2)); // true
     * var q3 = Quat.fromPitchYawRollDegrees(0, 0, 359.95);
     * print(Quat.equal(q1, q3)); // false
     *
     * var dot = Quat.dot(q1, q3);
     * print(dot); // -0.9999999403953552
     * var equal = Math.abs(1 - Math.abs(dot)) < 0.000001;
     * print(equal); // true
     */
    bool equal(const glm::quat& q1, const glm::quat& q2);

    /**jsdoc
     * Cancels out the roll and pitch component of a quaternion so that its completely horizontal with a yaw pointing in the 
     * given quaternion's direction.
     * @function Quat(0).cancelOutRollAndPitch
     * @param {Quat} orientation - A quaternion representing an orientation.
     * @returns {Quat}  <code>orientation</code> with its roll and pitch canceled out.
     * @example <caption>Two ways of calculating a camera orientation in the x-z plane with a yaw pointing in the direction of
     *     a given quaternion.</caption>
     * var quaternion = Quat.fromPitchYawRollDegrees(10, 20, 30);
     *
     * var noRollOrPitch = Quat.cancelOutRollAndPitch(quaternion);
     * Quat.print("", noRollOrPitch, true); // dvec3(0.000000, 22.245995, 0.000000)
     *
     * var front = Quat.getFront(quaternion);
     * var lookAt = Quat.lookAtSimple(Vec3.ZERO, { x: front.x, y: 0, z: front.z });
     * Quat.print("", lookAt, true); // dvec3(0.000000, 22.245996, 0.000000)
     *
     */
    glm::quat cancelOutRollAndPitch(const glm::quat& q);

    /**jsdoc
    * Cancels out the roll component of a quaternion so that its horizontal axis is level.
    * @function Quat(0).cancelOutRoll
    * @param {Quat} orientation - A quaternion representing an orientation.
    * @returns {Quat} <code>orientation</code> with its roll canceled out.
    * @example <caption>Two ways of calculating a camera orientation that points in the direction of a given quaternion but
    *     keeps the camera's horizontal axis level.</caption>
    * var quaternion = Quat.fromPitchYawRollDegrees(10, 20, 30);
    *
    * var noRoll = Quat.cancelOutRoll(quaternion);
    * Quat.print("", noRoll, true); // dvec3(-1.033004, 22.245996, -0.000000)
    *
    * var front = Quat.getFront(quaternion);
    * var lookAt = Quat.lookAtSimple(Vec3.ZERO, front);
    * Quat.print("", lookAt, true); // dvec3(-1.033004, 22.245996, -0.000000)
    */
    glm::quat cancelOutRoll(const glm::quat& q);

private:
    const glm::quat& IDENTITY() const { return Quaternions::IDENTITY; }

};

#endif // hifi_Quat_h
