/************************************************************************************
 
 Authors     :   Bradley Austin Davis <bdavis@saintandreas.org>
 Copyright   :   Copyright Brad Davis. All Rights reserved.
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 
 ************************************************************************************/

#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stack>

#include <gpu/GPUConfig.h>



class MatrixStack : public std::stack<glm::mat4> {

public:

  MatrixStack() {
    push(glm::mat4());
  }

  explicit MatrixStack(const MatrixStack& other) : std::stack<glm::mat4>() {
    *((std::stack<glm::mat4>*)this) = *((std::stack<glm::mat4>*)&other);
  }

  operator const glm::mat4 & () const {
    return top();
  }

  MatrixStack & pop() {
    std::stack<glm::mat4>::pop();
    assert(!empty());
    return *this;
  }

  MatrixStack & push() {
    emplace(top());
    return *this;
  }

  MatrixStack & identity() {
    top() = glm::mat4();
    return *this;
  }

  MatrixStack & push(const glm::mat4 & mat) {
    std::stack<glm::mat4>::push(mat);
    return *this;
  }

  MatrixStack & rotate(const glm::mat3 & rotation) {
    return postMultiply(glm::mat4(rotation));
  }

  MatrixStack & rotate(const glm::quat & rotation) {
    return postMultiply(glm::mat4_cast(rotation));
  }

  MatrixStack & rotate(float theta, const glm::vec3 & axis) {
    return postMultiply(glm::rotate(glm::mat4(), theta, axis));
  }

  MatrixStack & translate(float translation) {
    return translate(glm::vec3(translation, 0, 0));
  }

  MatrixStack & translate(const glm::vec2 & translation) {
    return translate(glm::vec3(translation, 0));
  }

  MatrixStack & translate(const glm::vec3 & translation) {
    return postMultiply(glm::translate(glm::mat4(), translation));
  }

  MatrixStack & preTranslate(const glm::vec3 & translation) {
    return preMultiply(glm::translate(glm::mat4(), translation));
  }


  MatrixStack & scale(float factor) {
    return scale(glm::vec3(factor));
  }

  MatrixStack & scale(const glm::vec3 & scale) {
    return postMultiply(glm::scale(glm::mat4(), scale));
  }

  MatrixStack & transform(const glm::mat4 & xfm) {
    return postMultiply(xfm);
  }

  MatrixStack & preMultiply(const glm::mat4 & xfm) {
    top() = xfm * top();
    return *this;
  }

  MatrixStack & postMultiply(const glm::mat4 & xfm) {
    top() *= xfm;
    return *this;
  }

  // Remove the rotation component of a matrix.  useful for billboarding
  MatrixStack & unrotate() {
    glm::quat inverse = glm::inverse(glm::quat_cast(top()));
    top() = top() * glm::mat4_cast(inverse);
    return *this;
  }

  // Remove the translation component of a matrix.  useful for skyboxing
  MatrixStack & untranslate() {
    top()[3] = glm::vec4(0, 0, 0, 1);
    return *this;
  }

  template <typename Function>
  void withPush(Function f) {
    size_t startingDepth = size();
    push();
    f();
    pop();
    assert(startingDepth = size());
  }

  template <typename Function>
  void withIdentity(Function f) {
    withPush([&] {
      identity();
      f();
    });
  }

  static MatrixStack & projection() {
    static MatrixStack projection;
    return projection;
  }

  static MatrixStack & modelview() {
    static MatrixStack modelview;
    return modelview;
  }

  template <typename Function>
  static void withPushAll(Function f) {
    withPush(projection(), modelview(), f);
  }

  template <typename Function>
  static void withIdentityAll(Function f) {
    withPush(projection(), modelview(), [=] {
      projection().identity();
      modelview().identity();
      f();
    });
  }

  template <typename Function>
  static void withPush(MatrixStack& stack, Function f) {
    stack.withPush(f);
  }

  template <typename Function>
  static void withPush(MatrixStack& stack1, MatrixStack& stack2, Function f) {
    stack1.withPush([&]{
      stack2.withPush(f);
    });
  }

  template <typename Function> 
  static void withGlMatrices(Function f) {
    // Push the current stack, and then copy the values out of OpenGL
    withPushAll([&] {
      // Fetch the current matrices out of GL stack
      // FIXME, eliminate the usage of deprecated GL
      MatrixStack & mv = MatrixStack::modelview();
      MatrixStack & pr = MatrixStack::projection();
      glm::mat4 & mvm = mv.top();
      glGetFloatv(GL_MODELVIEW_MATRIX, &(mvm[0][0]));

      glm::mat4 & prm = pr.top();
      glGetFloatv(GL_PROJECTION_MATRIX, &(prm[0][0]));
      f();
    });
  }
};

