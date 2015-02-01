#include "MatrixStack.h"

QMatrix4x4 fromGlm(const glm::mat4 & m) {
  return QMatrix4x4(&m[0][0]).transposed();
  return QMatrix4x4(&m[0][0]);
}

