#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vr/gvr/capi/include/gvr.h>

namespace googlevr {

    // Convert a GVR matrix to GLM matrix
    glm::mat4 toGlm(const gvr::Mat4f &matrix) {
        glm::mat4 result;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result[j][i] = matrix.m[i][j];
            }
        }
        return result;
    }

    // Given a field of view in degrees, compute the corresponding projection
// matrix.
    glm::mat4 perspectiveMatrixFromView(const gvr::Rectf& fov, float z_near, float z_far) {
        const float x_left = -std::tan(fov.left * M_PI / 180.0f) * z_near;
        const float x_right = std::tan(fov.right * M_PI / 180.0f) * z_near;
        const float y_bottom = -std::tan(fov.bottom * M_PI / 180.0f) * z_near;
        const float y_top = std::tan(fov.top * M_PI / 180.0f) * z_near;
        const float Y = (2 * z_near) / (y_top - y_bottom);
        const float A = (x_right + x_left) / (x_right - x_left);
        const float B = (y_top + y_bottom) / (y_top - y_bottom);
        const float C = (z_near + z_far) / (z_near - z_far);
        const float D = (2 * z_near * z_far) / (z_near - z_far);

        glm::mat4 result { 0 };
        result[2][0] = A;
        result[1][1] = Y;
        result[2][1] = B;
        result[2][2] = C;
        result[3][2] = D;
        result[2][3] = -1;
        return result;
    }

    glm::quat toGlm(const gvr::ControllerQuat& q) {
        glm::quat result;
        result.w = q.qw;
        result.x = q.qx;
        result.y = q.qy;
        result.z = q.qz;
        return result;
    }

}
