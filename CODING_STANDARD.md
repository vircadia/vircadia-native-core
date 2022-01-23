# Coding Standards

Note that the current code base does not necessarily follow this with 100% consistency. It will be an ongoing process to try and sanitize the existing code to match these guidelines.

Basically taken directly from [http://geosoft.no/development/cppstyle.html](http://geosoft.no/development/cppstyle.html) with some subtle changes and omissions.

## [1] Naming

### [1.1] General Naming Conventions

#### [1.1.1] Names representing types must be in mixed case starting with upper case.

```cpp
Coach, PenaltyBox

```

#### [1.1.2] Private class variables must be in mixed case prefixed with an underscore.

```cpp
_puck, _team

```

#### [1.1.3] Local variables must be in mixed case (and NOT prefixed with an underscore).

```cpp
redLine, icingFrequency

```

#### [1.1.4] Constants must be all uppercase using underscore to separate words.

```cpp
MAX_RINK_LENGTH, COLOR_RED_LINE

```

#### [1.1.5] Methods or functions must be verbs and written in mixed case starting with lower case.

```cpp
getPlayerNumber(), computeGoalsAgainstAverage()

```

#### [1.1.6] Names representing namespaces should be all lowercase.

```cpp
puck::geometry, ice::math

```

#### [1.1.7] Names representing template types should be a single uppercase letter.

```cpp
template<class R>, template<class B>, template<class K>

```

This makes template names stand out relative to all other names used.

#### [1.1.8] Abbreviations and acronyms must be uppercase when used in a name or lowercase when used at the beginning of a variable

```cpp
showNHLStandings(); // not showNhlStandings();
exportASCIIStanleyCup(); // not exportAsciiStanleyCup();
UDPSocket udpSocket; // not UDPSocket uDPSocket;

```

#### [1.1.9] Global variables should always be referred to using the :: operator.

```cpp
::jumbotron.powerOn();
::league.lockout();

```

#### [1.1.10] Generic variables should have the same name as their type.

```cpp
void setPuckLogo(Logo* logo) // not void setPuckLogo(Logo* aLogo)

```

These will be discernible from class member variables since they are not prefixed with an underscore.

#### [1.1.11] All names should be written in English.

```cpp
int hockeyStick;   // NOT: bastonDeHockey

```

#### [1.1.12] The name of the object is implicit, and should be avoided in a method name.

```cpp
puck.getDensity();    // NOT: puck.getPuckDensity();

```

### [1.2] Specific Naming Conventions

#### [1.2.1] The terms get/set must be used where an attribute is accessed directly.

```cpp
player.getNumber();
player.setNumber(number);
stick.getFlex();
stick.setFlex(flex);

```

There is an exception for boolean getters. Naming for boolean attributes should follow [section 1.2.10](https://wiki.highfidelity.com/wiki/Coding_Standards#1-2-10-the-prefix-is-should-be-used-for-boolean-variables-and-methods-). The getter for a boolean attribute does not need to be prefixed with 'get', and should simply match the name of the boolean attribute. The following example is for a member variable `_isCaptain` on the `crosby` object.

```cpp
crosby.setIsCaptain(true);
crosby.isCaptain();

```

#### [1.2.2] The term compute can be used in methods where something is computed.

```cpp
team->computePowerPlayPercentage();
player->computePointsPerGame();

```

Give the reader the immediate clue that this is a potentially time-consuming operation, and if used repeatedly, she might consider caching the result. Consistent use of the term enhances readability.

#### [1.2.3] The term find can be used in methods where something is looked up.

```cpp
net.findGoalLinePosition();
team.findHeaviestPlayer();

```

Give the reader the immediate clue that this is a simple look up method with a minimum of computations involved. Consistent use of the term enhances readability.

#### [1.2.4] The term initialize can be used where an object or a concept is established.

```
rink.initializePaintedLines();
video.initializeOnScreenScore();

```

#### [1.2.5] Variables representing GUI components should be suffixed by the component type name.

```cpp
scoreboardText, mainWindow, fileMenu

```

#### [1.2.6] Plural form should be used on names representing a collection of objects.

```cpp
std::vector<Player> players;
float savePercentages[];

```

#### [1.2.7] The prefix num should be used for variables representing a number of objects.

```cpp
numGoals, numAssists

```

#### [1.2.8] The suffix Num should be used for variables representing an entity number.

```cpp
playerNum, teamNum

```

#### [1.2.9] Iterator variables should be called i, j, k etc.

```cpp
for (int i = 0; i < numGoals); i++) {
    goals[i].playVideo();
}

```

#### [1.2.10] The prefix is should be used for boolean variables and methods.

isGoodGoal, isRetired, isWinningTeam Occasionally the has, can, should, and want prefixes will be better choices.

*Note: "want" should generally be used for optional items that are specified by some third party action, e.g. command line or menu options that enable additional functionality, or protocol versioning where negotiation occurs between client and server.*

```cpp
hasWonStanleyCup, canPlay, shouldPass, wantDebugLogging

```

#### [1.2.11] Complement names must be used for complement operations

```cpp
get/set, add/remove, create/destroy, start/stop

```

#### [1.2.12] Abbreviations in names should be avoided.

```cpp
computeGoalsAgainstAverage();  // NOT: compGlsAgstAvg();

```

There are domain specific phrases that are more naturally known through their abbreviations/acronym. These phrases should be kept abbreviated.

Use `html` instead of `hypertextMarkupLanguage`.

#### [1.2.13] Naming pointers specifically should be avoided.

```cpp
Puck* puck; // NOT: Puck * puckPtr;

```

Many variables in a C/C++ environment are pointers, so a convention like this is almost impossible to follow. Also objects in C++ are often oblique types where the specific implementation should be ignored by the programmer. Only when the actual type of an object is of special significance, the name should emphasize the type.

#### [1.2.14] Negated boolean variable names must be avoided.

```cpp
bool isRetired; // NOT: isNotRetired or isNotPlaying

```

This is done to avoid double negatives when used in conjunction with the logical negation operator.

#### [1.2.15] Enumeration constants can be prefixed by a common type name.

```cpp
enum Jersey {
    JERSEY_HOME,
    JERSEY_AWAY,
    JERSEY_ALTERNATE
};

```

#### [1.2.16] Exception classes should be suffixed with Exception.

```cpp
class GoalException {
    ...
};

```

## [2] Files

### [2.1] Source Files

#### [2.1.1] C++ header files should have the extension .h. Source files should have the extension .cpp.

```cpp
Puck.h, Puck.cpp

```

#### [2.1.2] A class should always be declared in a header file and defined in a source file where the name of the files match the name of the class.

`class Puck` defined in `Puck.h`, `Puck.cpp`

#### [2.1.3] Most function implementations should reside in the source file.

The header files should declare an interface, the source file should implement it. When looking for an implementation, the programmer should always know that it is found in the source file.

- Simple getters and setters that just access private member variables should appear inline in the class definition in the header file.
- Simple methods like those making slight mutations (that can fit on the same line in the definition and don't require additional includes in the header file) can be inlined in the class definition.
- Methods that will be called multiple times in tight-loops or other high-performance situations and must be high performance can be included in the header file BELOW the class definition marked as inline.
- All other methods must be in a cpp file.

```cpp
class Puck {
public:
    // simple getters/setters should appear in the header file
    int getRadius() const { return _radius; }
    void setRadius(int radius) { _radius = radius; }

    // Allowed, ok to include this simple mutation in line
    void addBlaToList(Blah* bla) { _blas.append(bla); }

    // Allowed, because this is a simple method
    int calculateCircumference() { return PI * pow(_radius, 2.0); }

    // this routine needs to be fast, we'll inline it below
    void doSomethingHighPerformance() const;
    ...
private:
    int _radius;
}

inline void Puck::doSomethingHighPerformance() const {
    ...
}

```

#### [2.1.4] File content must be kept within 128 columns.

#### [2.1.5] Special characters like TAB and page break must be avoided.

Use four spaces for indentation.

#### [2.1.6] The incompleteness of split lines must be made obvious.

```cpp
teamGoals = iginlaGoals + crosbyGoals +
            malkinGoals;

addToScoreSheet(scorer, directAssister,
                indirectAssister);

setHeadline("Crosby scores 4"
            " to force game 7.");

for (int teamNum = 0; teamNum < numTeams;
     teamNum++) {
    ...
}

```

Split lines occurs when a statement exceed the 128 column limit given above. It is difficult to provide rigid rules for how lines should be split, but the examples above should give a general hint.

In general: Break after a comma. Break after an operator. Align the new line with the beginning of the expression on the previous line.

### [2.2] Include Files and Include Statements

#### [2.2.1] Header files must contain an include guard.

Include guards should be in the following format: hifi_$BASENAME_h.

```cpp
#ifndef hifi_SharedUtil_h
#define hifi_SharedUtil_h

...

#endif // hifi_SharedUtil_h

```

#### [2.2.2] Include statements should be sorted and grouped. Sorted by their hierarchical position in the system with low level files included first. Leave an empty line between groups of include statements.

```cpp
#include <fstream>
#include <cstring>

#include <qt/qbutton.h>
#include <qt/qtextfield.h>

#include "Puck.h"
#include "PenaltyBox.h"

```

#### [2.2.3] Include statements must be located at the top of a file only.

## [3] Statements

### [3.1] Types

#### [3.1.1] The parts of a class must be sorted public, protected and private. All sections must be identified explicitly. Not applicable sections should be left out.

The ordering is "most public first" so people who only wish to use the class can stop reading when they reach the protected/private sections.

#### [3.1.2] Never rely on implicit type conversion. // NOT: floatValue = intValue;

##### [3.1.2.1] Primitive types should use C style casting:

```cpp
int foo = 1;
float bar = (float)foo;
// NOT this: float fubar = float(foo);

uint8_t* barDataAt = (uint8_t*)&bar; // pointers to primitive types also use C style casting.

```

##### [3.1.2.2] Class pointers must use C++ style casting:

```cpp
Player* player = getPlayer("forward");
Forward* forward = static_cast<Forward*>(player);

```

For more info about C++ type casting: [http://stackoverflow.com/questions/1609163/what-is-the-difference-between-static-cast-and-c-style-casting](http://stackoverflow.com/questions/1609163/what-is-the-difference-between-static-cast-and-c-style-casting)

#### [3.1.3] Use of *const*

##### [3.1.3.1] Use const types for variables, parameters, return types, and methods whenever possible

```cpp
void exampleBarAndFoo(const Bar& bar, const char* foo); // doesn't modify bar and foo, use const types
void ClassBar::spam() const { } // doesn't modify instance of ClassBar, use const method

```

##### [3.1.3.2] Place the const keyword before the type

```cpp
void foo(const Bar& bar);
// NOT: void foo(Bar const& bar);
void spam(const Foo* foo);
// NOT: void foo(Foo const* foo);

```

##### [3.1.3.3] When implementing a getter for a class that returns a class member that is a complex data type, return a const& to that member.

```cpp
const glm::vec3& AABox::getCorner() const;
// NOT: glm::vec3 AABox::getCorner() const;

```

#### [3.1.4] Type aliases

##### [3.1.4.1] When creating a type alias, prefer the using keyword.

```cpp
template<class T>
using Vec = std::vector<T, Alloc<T>>;
using Nodes = Vec <Node>;
// NOT: typedef std::vector<Node> Nodes;

```

### [3.2] Variables

#### [3.2.1] Variables should be initialized where they are declared.

This ensures that variables are valid at any time.

Sometimes it is impossible to initialize a variable to a valid value where it is declared:

```cpp
Player crosby, dupuis, kunitz;
getLineStats(&crosby, &dupuis, &kunitz);

```

In these cases it should be left uninitialized rather than initialized to some phony value.

#### [3.2.2] Initialization of member variables with default values

When possible, initialization of default values for class members should be included in the header file where the member variable is declared, as opposed to the constructor. Use the Universal Initializer format (brace initialization) rather than the assignment operator (equals).

```cpp
private:
    float _goalsPerGame { 0.0f }; // NOT float _goalsPerGame = 0.0f;

```

However, brace initialization should be used with care when using container types that accept an initializer list as a constructor parameters. For instance,

```cpp
std::vector<int> _foo { 4, 100 }

```

Might refer to `std::vector<T>::vector<T>(std::initializer_list<T>)` or it might refer to `std::vector (size_type n, const T& val = value_type())`. Although the rules of precedence dictate that it will resolve to one of these, it's not immediately obvious to other developers which it is, so avoid such ambiguities.

Classes that are forward declared and only known to the implementation may be initialized to a default value in the constructor initialization list.

#### [3.2.3] Use of global variables should be minimized

[http://stackoverflow.com/questions/484635/are-global-variables-bad](http://stackoverflow.com/questions/484635/are-global-variables-bad)

#### [3.2.4] Class variables should never be declared public

Use private variables and access functions instead.

One exception to this rule is when the class is essentially a data structure, with no behavior (equivalent to a C struct). In this case it is appropriate to make the class' instance variables public.

*Note that structs are kept in C++ for compatibility with C only, and avoiding them increases the readability of the code by reducing the number of constructs used. Use a class instead.*

#### [3.2.5] C++ pointers and references should have their reference symbol next to the type rather than to the name.

```cpp
float* savePercentages;
// NOT: float *savePercentages; or float * savePercentages;

void checkCups(int& numCups);
// NOT: int &numCups or int & numCups

```

The pointer-ness or reference-ness of a variable is a property of the type rather than the name. Also see [rule 3.1.3.2](https://wiki.highfidelity.com/wiki/Coding_Standards#constplacement) regarding placement the const keyword before the type.

#### [3.2.6] Implicit test for 0 should not be used other than for boolean variables or non-NULL pointers.

```cpp
if (numGoals != 0)  // NOT: if (numGoals)
if (savePercentage != 0.0) // NOT: if (savePercentage)

// Testing pointers for non-NULL is prefered, e.g. where
// childNode is Node* and you’re testing for non NULL
if (childNode)

// Testing for null is also preferred
if (!childNode)

```

It is not necessarily defined by the C++ standard that ints and floats 0 are implemented as binary 0.

#### [3.2.7] Variables should be declared in the smallest scope possible.

Keeping the operations on a variable within a small scope, it is easier to control the effects and side effects of the variable.

### [3.3] Loops

#### [3.3.1] Loop variables should be initialized immediately before the loop.

#### [3.3.2] The form while (true) should be used for infinite loops.

```cpp
while (true) {
    :
}

// NOT:

for (;;) {
    :
}

while (1) {
    :
}

```

### [3.4] Conditionals

#### [3.4.1] The nominal case should be put in the if-part and the exception in the else-part of an if statement

```cpp
bool isGoal = pastGoalLine(position);

if (isGoal) {
    ...
} else {
    ...
}

```

Makes sure that the exceptions don't obscure the normal path of execution. This is important for both the readability and performance.

#### [3.4.2] The conditional should be put on a separate line and wrapped in braces.

```cpp
if (isGoal) {
    lightTheLamp();
}

// NOT: if (isGoal) lightTheLamp();

```

#### [3.4.3]  Write the expression of a conditional similar to how you would speak it out loud.

```cpp
if (someVariable == 0) {
    doSomething();
}
// NOT: if (0 == someVariable)

```

### [3.5] Miscellaneous

#### [3.5.1] Constants and Magic Numbers

##### [3.5.1.1] The use of magic numbers in the code should be avoided.

- Numbers other than 0 and 1 should be considered declared as named constants instead.
- If the number does not have an obvious meaning by itself, the readability is enhanced by introducing a named constant instead.
- A different approach is to introduce a method from which the constant can be accessed.

##### [3.5.1.2] Declare constants closest to the scope of their use.

```cpp
bool checkValueLimit(int value) {
    const int ValueLimit = 10; // I only use this constant here, define it here in context
    return (value > ValueLimit);
}

```

##### [3.5.1.3] Use const typed variables instead of #define

```cpp
const float LARGEST_VALUE = 10000.0f;
// NOT:  #define LARGEST_VALUE 10000.0f

```

#### [3.5.2] Floating point constants should always be written with decimal point and at least one decimal.

```cpp
double stickLength = 0.0;    // NOT:  double stickLength = 0;

double penaltyMinutes;
...
penaltyMinutes = (minor + misconduct) * 2.0;

```

#### [3.5.3] Floating point constants should always be written with a digit before the decimal point.

```cpp
double penaltyMinutes = 0.5;  // NOT:  double penaltyMinutes = .5;

```

#### [3.5.4] When using a single precision float type, include the trailing f.

```cpp
float penaltyMinutes = 0.5f;  // NOT:  float penaltyMinutes = 0.5;

```

## [4] Layout and Comments

### [4.1] Layout

#### [4.1.1] Basic indentation should be 4.

```cpp
if (player.isCaptain) {
    player.yellAtReferee();
}

```

#### [4.1.2] Use inline braces for block layout

```cpp
while (!puckHeld) {
    lookForRebound();
}

// NOT:
// while (!puckHeld)
// {
//     lookForRebound();
// }

```

#### [4.1.3] The class declarations should have the following form:

```cpp
class GoalieStick : public HockeyStick {
public:
    ...
protected:
    ...
private:
    ...
};

```

#### [4.1.4] Method definitions should have the following form:

```cpp
void goalCelebration() {
    ...
}

```

#### [4.1.5] The if-else class of statements should have the following form:

```cpp
if (isScorer) {
    scoreGoal();
}

if (isScorer) {
    scoreGoal();
} else {
    saucerPass();
}

if (isScorer) {
    scoreGoal();
} else if (isPlaymaker) {
    saucerPass();
} else {
    startFight();
}

```

#### [4.1.6] A for statement should have the following form:

```cpp
for (int i = 0; i < GRETZKY_NUMBER; i++) {
    getActivePlayerWithNumber(i);
}

```

#### [4.1.7] A while statement should have the following form:

```cpp
while (!whistle) {
    keepPlaying();
}

```

#### [4.1.8] A do-while statement should have the following form:

```cpp
do {
    skate();
} while (!tired);

```

#### [4.1.9] Switch/Case Statements:

A switch statements should follow the following basic formatting rules:

- The case statements are indented one indent (4 spaces) from the switch.
- The code for each case should be indented one indent (4 spaces) from the case statement.
- Each separate case should have a break statement, unless it is explicitly intended for the case to fall through to the subsequent cases. In the event that a case statement executes some code, then falls through to the next case, you must include an explicit comment noting that this is intentional.
- Break statements should be aligned with the code of the case, e.g. indented 4 spaces from the case statement.
- In the event that brackets are required to create local scope, the open bracket should appear on the same line as the case, and the close bracket should appear on the line immediately following the break aligned with the case statement.

Examples of acceptable form are:

```cpp
switch (foo) {
    case BAR:
        doBar();
        break;

    // notice brackets below follow the standard bracket placement for other control structures
    case SPAM: {
        int spam = 0;
        doSomethingElse(spam);
        break;
    }

    case SPAZZ:
    case BAZZ:
        doSomething();
        // fall through to next case

    case RAZZ:
    default:
        doSomethingElseEntirely();
        break;
}

// or in cases where returns occur at each case, this form is also accpetable
switch (jerseyNumber) {
    case 87:
        return crosby;
    case 66:
        return lemieux;
    case 99:
        return gretzky;
    default:
        return NULL;
}

```

#### [4.1.10] A try-catch statement should have the following form:

```cpp
try {
    tradePlayer();
} catch (const NoTradeClauseException& exception) {
    negotiateNoTradeClause();
}

```

#### [4.1.11] Single statement if-else, for or while statements must be written with braces.

```cpp
// GOOD:
for (int i = 0; i < numItems; i++) {
    item[i].manipulate();
}

// BAD: braces are missing
for (int i = 0; i < numItems; i++)
    item[i].manipulate();
```

### [4.2] White space

#### [4.2.1] Conventional operators should be surrounded by a space character, except in cases like mathematical expressions where it is easier to visually parse when spaces are used to enhance the grouping.

```cpp
potential = (age + skill) * injuryChance;
// NOT: potential = (age+skill)*injuryChance;

// Assignment operators always have spaces around them.
x = 0;

// Other binary operators usually have spaces around them, but it's
// OK to remove spaces around factors.  Parentheses should have no
// internal padding.
v = w * x + y / z;
v = w*x + y/z;
v = w * (x + z);

```

#### [4.2.2] C++ reserved words should be followed by a white space.

```cpp
setLine(leftWing, center, rightWing, leftDefense, rightDefense);
// NOT: setLine(leftWing,center,rightWing,leftDefense,rightDefense);

```

#### [4.2.3] Semicolons in for statments should be followed by a space character.

```cpp
for (i = 0; i < 10; i++) {  // NOT: for(i=0;i<10;i++){

```

#### [4.2.4] Declaring and Calling Functions

- Function names should not be followed by a white space.
- And there should be no space between the open parenthesis and the first parameter, and no space between the last parameter and the close parenthesis.

Examples:

```cpp
setCaptain(ovechkin);
// NOT: setCaptain (ovechkin);
// NOT: doSomething( int foo, float bar );

```

#### [4.2.6] Logical units within a block should be separated by one blank line.

```cpp
Team penguins = new Team();

Player crosby = new Player();
Player fleury = new Player();

penguins.setCaptain(crosby);
penguins.setGoalie(fleury);

penguins.hireCoach();

```

#### [4.2.6] Avoid adding optional spaces across multi-line statements and adjacent statements.

Avoid the following:

```
oddsToWin = (averageAge     * veteranWeight) +
            (numStarPlayers * starPlayerWeight) +
            (goalieOverall  * goalieWeight);

theGreatOneSlapShotSpeed  = computeShot(stickFlex, chara);
charaSlapShotSpeed        = computeShot(stickFlex, weber);

```

A change to the length of a variable in these sections causes unnecessary changes to the other lines.

#### [4.2.7] Multi-line statements must have all n+1 lines indented at least one level (four spaces).

Align all n+2 lines with the indentation of the n+1 line.

When the multiple lines are bound by parentheses (as in arguments to a function call), the prefered style has no whitespace after the opening parenthesis or before the closing parenthesis. The n+1 lines are generally indented to the column immediately after the opening parenthesis (following the style for split expressions in 2.1.6).

When the multiple lines are bound by braces (as in C++ initializers or JavaScript object notation), the preferred style has a newline after the opening brace and newline before the closing brace. The final line should not end in a comma, and no line should begin with a comma. The closing brace should begin in the same colum as the line that has the opening brace (following the style for split control statements in 4.1).

Expressions, including C++ initializers and JavaScript object notation literals, can be placed on a single line if they are not deeply nested and end well within the column limit (2.1.4).

The following are all acceptable:

```cpp
shootOnNet(puckVelocity,
    playerStrength,
    randomChance);

shootOnNet(puckVelocty,
           playerStrength,
           randomChance);

if (longBooleanThatHasToDoWithHockey
    && anotherBooleanOnANewLine);

isGoodGoal = playerSlapShotVelocity > 100
    ? true
    : false;

var foo = {
    spam: 1.0,
    bar: "bar",
    complex: {
        red: 1,
        white: 'blue'
    },
    blah: zed
};

aJavascriptFunctionOfTwoFunctions(function (entity) {
    print(entity);
    foo(entity, 3);
}, function (entity) {
    print('in second function');
    bar(entity, 4);
});

aCPlusPlusFunctionOfTwoLambdas([](gpu::Batch& batch) {
    batch.setFramebuffer(nullptr);
}, [this](int count, float amount) {
    frob(count, amount);
});

```

### [4.3] Comments

#### [4.3.1] All comments should be written in English

In an international environment English is the preferred language.

#### [4.3.2] Use // for all comments, including multi-line comments.

An exception to this rule applies to JSDoc and Doxygen comments.

```cpp
// Comment spanning
// more than one line.

```

There should be a space between the "//" and the actual comment

#### [4.3.3] Comments should be included relative to their position in the code

```cpp
while (true) {
    // crosby is always injured
    crosbyInjury();
}

// NOT:
// crosby is always injured
while (true) {
    crosbyInjury();
}

```

#### [4.3.4] Source files (header and implementation) must include a boilerplate.

Boilerplates should include the filename, creator, copyright Vircadia contributors, and Apache 2.0 License information. 
This should be placed at the top of the file. If editing an existing file that is copyright High Fidelity, add a second 
copyright line, copyright Vircadia contributors.

```cpp
//
//  NodeList.h
//
//  Created by Stephen Birarda on 15 Feb 2013.
//  Copyright 2013 High Fidelity, Inc.
//  Copyright 2020 Vircadia contributors.
//
//  This is where you could place an optional one line comment about the file.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

```

#### [4.3.5] Never include Horizontal "line break" style comment blocks

These types of comments are explicitly not allowed. If you need to break up sections of code, just leave an extra blank line.

```cpp
//////////////////////////////////////////////////////////////////////////////////

/********************************************************************************/

//--------------------------------------------------------------------------------
```

#### [4.3.6] Doxygen comments should use "///"

Use the `///` style of [Doxygen](https://www.doxygen.nl/index.html) comments when documenting public interfaces.

Some editors can automatically create a Doxygen documentation stub if you type `///` in the line above the item to be
documented.

**Visual Studio:** To configure Visual Studio's Doxygen commenting behavior, search for "Doxygen" in Tools > Options.
