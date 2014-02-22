//
//  main.cpp
//  physics-tests
//

//#include <QDebug>

#include "ShapeColliderTests.h"
#include "CollisionInfoTests.h"

int main(int argc, char** argv) {
    CollisionInfoTests::runAllTests();
    ShapeColliderTests::runAllTests();
    return 0;
}
