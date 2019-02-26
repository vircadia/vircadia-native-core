---
title: 'Coding Standards'
---

Note that the current code base does not necessarily follow this with 100% consistency. It will be an ongoing process to try and sanitize the existing code to match these guidelines.

Basically taken directly from [http://geosoft.no/development/cppstyle.html](http://geosoft.no/development/cppstyle.html) with some subtle changes and omissions.

## Naming

### General Naming Conventions

#### Names representing types must be in mixed case starting with upper case.

```
Coach, PenaltyBox

```

#### Private class variables must be in mixed case prefixed with an underscore.

```
_puck, _team

```

#### Local variables must be in mixed case (and NOT prefixed with an underscore).

```
redLine, icingFrequency

```

#### Constants must be all uppercase using underscore to separate words.

```
MAX_RINK_LENGTH, COLOR_RED_LINE

```

#### Methods or functions must be verbs and written in mixed case starting with lower case.

```
getPlayerNumber(), computeGoalsAgainstAverage()

```

#### Names representing namespaces should be all lowercase.

```
puck::geometry, ice::math

```

#### Names representing template types should be a single uppercase letter.

```
template<class R>, template<class B>, template<class K>

```

This makes template names stand out relative to all other names used.

#### Abbreviations and acronyms must be uppercase when used in a name or lowercase when used at the beginning of a variable

```
showNHLStandings(); // not showNhlStandings();
exportASCIIStanleyCup(); // not exportAsciiStanleyCup();
UDPSocket udpSocket; // not UDPSocket uDPSocket;

```

#### Global variables should always be referred to using the :: operator.

```
::jumbotron.powerOn(), ::league.lockout();

```

#### Generic variables should have the same name as their type.

```
void setPuckLogo(Logo* logo) // not void setPuckLogo(Logo* aLogo)

```

These will be discernible from class private variables since they are not prefixed with an underscore.

#### All names should be written in English.

```
hockeyStick;   // NOT: bastonDeHockey

```

#### The name of the object is implicit, and should be avoided in a method name.

```
puck.getDensity();    // NOT: puck.getPuckDensity();

```

### Specific Naming Conventions

#### The terms get/set must be used where an attribute is accessed directly.

```
player.getNumber();
player.setNumber(number);
stick.getFlex();
stick.setFlex(flex);

```

There is an exception for boolean getters. Naming for boolean attributes should follow [section 1.2.10](https://wiki.highfidelity.com/wiki/Coding_Standards#1-2-10-the-prefix-is-should-be-used-for-boolean-variables-and-methods-). The getter for a boolean attribute does not need to be prefixed with 'get', and should simply match the name of the boolean attribute. The following example is for a member variable `_isCaptain` on the `crosby` object.

```
crosby.setIsCaptain(true);
crosby.isCaptain();

```

#### The term compute can be used in methods where something is computed.

```
team->computePowerPlayPercentage();
player->computePointsPerGame();

```

Give the reader the immediate clue that this is a potentially time-consuming operation, and if used repeatedly, she might consider caching the result. Consistent use of the term enhances readability.

#### The term find can be used in methods where something is looked up.

```
net.findGoalLinePosition();
team.findHeaviestPlayer();

```

Give the reader the immediate clue that this is a simple look up method with a minimum of computations involved. Consistent use of the term enhances readability.

#### The term initialize can be used where an object or a concept is established.

```
rink.initializePaintedLines();
video.initializeOnScreenScore();

```

#### Variables representing GUI components should be suffixed by the component type name.

```
scoreboardText, mainWindow, fileMenu

```

#### Plural form should be used on names representing a collection of objects.

```
vector<Player> players;
float savePercentages[];

```

#### The prefix num should be used for variables representing a number of objects.

```
numGoals, numAssists

```

#### The suffix Num should be used for variables representing an entity number.

```
playerNum, teamNum

```

#### Iterator variables should be called i, j, k etc.

```
for (int i = 0; i < numGoals); i++) {
    goals[i].playVideo();
}

```

#### The prefix is should be used for boolean variables and methods.

isGoodGoal, isRetired, isWinningTeam Occasionally the has, can, should, and want prefixes will be better choices.

*Note: "want" should generally be used for optional items that are specified by some third party action, e.g. command line or menu options that enable additional functionality, or protocol versioning where negotiation occurs between client and server.*

```
hasWonStanleyCup, canPlay, shouldPass, wantDebugLogging

```

#### Complement names must be used for complement operations

```
get/set, add/remove, create/destroy, start/stop

```

#### Abbreviations in names should be avoided.

```
computeGoalsAgainstAverage();  // NOT: compGlsAgstAvg();

```

There are domain specific phrases that are more naturally known through their abbreviations/acronym. These phrases should be kept abbreviated.

Use `html` instead of `hypertextMarkupLanguage`.

#### Naming pointers specifically should be avoided.

```
Puck* puck; // NOT: Puck * puckPtr;

```

Many variables in a C/C++ environment are pointers, so a convention like this is almost impossible to follow. Also objects in C++ are often oblique types where the specific implementation should be ignored by the programmer. Only when the actual type of an object is of special significance, the name should emphasize the type.

#### Negated boolean variable names must be avoided.

```
bool isRetired; // NOT: isNotRetired or isNotPlaying

```

This is done to avoid double negatives when used in conjunction with the logical negation operator.

#### Enumeration constants can be prefixed by a common type name.

```
enum Jersey {
    JERSEY_HOME,
    JERSEY_AWAY,
    JERSEY_ALTERNATE
};

```

#### Exception classes should be suffixed with Exception.

```
class GoalException {
    ...
};

```

## Files

### Source Files

#### C++ header files should have the extension .h. Source files should have the extension .cpp.

```
Puck.h, Puck.cpp

```

#### A class should always be declared in a header file and defined in a source file where the name of the files match the name of the class.

`class Puck` defined in `Puck.h`, `Puck.cpp`

#### Most function implementations should reside in the source file.

The header files should declare an interface, the source file should implement it. When looking for an implementation, the programmer should always know that it is found in the source file.

- Simple getters and setters that just access private member variables should appear inline in the class definition in the header file.
- Simple methods like those making slight mutations (that can fit on the same line in the definition and don't require additional includes in the header file) can be inlined in the class definition.
- Methods that will be called multiple times in tight-loops or other high-performance situations and must be high performance can be included in the header file BELOW the class definition marked as inline.
- All other methods must be in a cpp file.

```
class Puck {
public:
    // simple getters/setters should appear in the header file
    int getRadius() const { return _radius; }
    void setRadius(int radius) { _radius = radius; }

    // Allowed, ok to include this simple mutation in line
    void addBlaToList(Blah* bla) { _blas.append(bla); }

    // Allowed, because this is a simple method
    int calculateCircumference () { return PI * pow(_radius, 2.0); }

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

#### File content must be kept within 128 columns.

#### Special characters like TAB and page break must be avoided.

Use four spaces for indentation.

#### The incompleteness of split lines must be made obvious.

```
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

### Include Files and Include Statements

#### Header files must contain an include guard.

Include guards should be in the following format: hifi_$BASENAME_h.

```
#ifndef hifi_SharedUtil_h
#define hifi_SharedUtil_h

...

#endif // hifi_SharedUtil_h

```

#### Include statements should be sorted and grouped. Sorted by their hierarchical position in the system with low level files included first. Leave an empty line between groups of include statements.

```
#include <fstream>
#include <cstring>

#include <qt/qbutton.h>
#include <qt/qtextfield.h>

#include "Puck.h"
#include "PenaltyBox.h"

```

#### Include statements must be located at the top of a file only.

## Statements

### Types

#### The parts of a class must be sorted public, protected and private. All sections must be identified explicitly. Not applicable sections should be left out.

The ordering is "most public first" so people who only wish to use the class can stop reading when they reach the protected/private sections.

#### Never rely on implicit type conversion. // NOT: floatValue = intValue;

##### Primitive types should use C style casting:

```
int foo = 1;
float bar = (float)foo;
// NOT this: float fubar = float(foo);

uint8_t* barDataAt = (uint8_t*)&bar; // pointers to primitive types also use C style casting.

```

##### Class pointers must use C++ style casting:

```
Player* player = getPlayer("forward");
Forward* forward = static_cast<Forward*>(player);

```

For more info about C++ type casting: [http://stackoverflow.com/questions/1609163/what-is-the-difference-between-static-cast-and-c-style-casting](http://stackoverflow.com/questions/1609163/what-is-the-difference-between-static-cast-and-c-style-casting)

#### Use of *const*

##### Use const types for variables, parameters, return types, and methods whenever possible

```
void exampleBarAndFoo(const Bar& bar, const char* foo); // doesn't modify bar and foo, use const types
void ClassBar::spam() const { } // doesn't modify instance of ClassBar, use const method

```

##### Place the const keyword before the type

```
void foo(const Bar& bar);
// NOT: void foo(Bar const& bar);
void spam(const Foo* foo);
// NOT: void foo(Foo const* foo);

```

##### When implementing a getter for a class that returns a class member that is a complex data type, return a const& to that member.

```
const glm::vec3& AABox::getCorner() const;
// NOT: glm::vec3 AABox::getCorner() const;

```

#### Type aliases

##### When creating a type alias, prefer the using keyword.

```
template<class T>
using Vec = std::vector<T, Alloc<T>>;
using Nodes = Vec <Node>;
// NOT: typedef std::vector<Node> Nodes;

```

### Variables

#### Variables should be initialized where they are declared.

This ensures that variables are valid at any time.

Sometimes it is impossible to initialize a variable to a valid value where it is declared:

```
Player crosby, dupuis, kunitz;
getLineStats(&crosby, &dupuis, &kunitz);

```

In these cases it should be left uninitialized rather than initialized to some phony value.

#### Initialization of member variables with default values

When possible, initialization of default values for class members should be included in the header file where the member variable is declared, as opposed to the constructor. Use the Universal Initializer format (brace initialization) rather than the assignment operator (equals).

```
private:
    float _goalsPerGame { 0.0f }; // NOT float _goalsPerGame = 0.0f;

```

However, brace initialization should be used with care when using container types that accept an initializer list as a constructor parameters. For instance,

```
std::vector<int> _foo { 4, 100 }

```

Might refer to `std::vector<T>::vector<T>(std::initializer_list<T>)` or it might refer to `std::vector (size_type n, const T& val = value_type())`. Although the rules of precedence dictate that it will resolve to one of these, it's not immediately obvious to other developers which it is, so avoid such ambiguities.

Classes that are forward declared and only known to the implementation may be initialized to a default value in the constructor initialization list.

#### Use of global variables should be minimized

[http://stackoverflow.com/questions/484635/are-global-variables-bad](http://stackoverflow.com/questions/484635/are-global-variables-bad)

#### Class variables should never be declared public

Use private variables and access functions instead.

One exception to this rule is when the class is essentially a data structure, with no behavior (equivalent to a C struct). In this case it is appropriate to make the class' instance variables public.

*Note that structs are kept in C++ for compatibility with C only, and avoiding them increases the readability of the code by reducing the number of constructs used. Use a class instead.*

#### C++ pointers and references should have their reference symbol next to the type rather than to the name.

```
float* savePercentages;
// NOT: float *savePercentages; or float * savePercentages;

void checkCups(int& numCups);
// NOT: int &numCups or int & numCups

```

The pointer-ness or reference-ness of a variable is a property of the type rather than the name. Also see [rule 3.1.3.2](https://wiki.highfidelity.com/wiki/Coding_Standards#constplacement) regarding placement the const keyword before the type.

#### Implicit test for 0 should not be used other than for boolean variables or non-NULL pointers.

```
if (numGoals != 0)  // NOT: if (numGoals)
if (savePercentage != 0.0) // NOT: if (savePercentage)

// Testing pointers for non-NULL is prefered, e.g. where
// childNode is Node* and you’re testing for non NULL
if (childNode)

// Testing for null is also preferred
if (!childNode)

```

It is not necessarily defined by the C++ standard that ints and floats 0 are implemented as binary 0.

#### Variables should be declared in the smallest scope possible.

Keeping the operations on a variable within a small scope, it is easier to control the effects and side effects of the variable.

### Loops

#### Loop variables should be initialized immediately before the loop.

#### The form while (true) should be used for infinite loops.

```
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

### Conditionals

#### The nominal case should be put in the if-part and the exception in the else-part of an if statement

```
bool isGoal = pastGoalLine(position);

if (isGoal) {
    ...
} else {
    ...
}

```

Makes sure that the exceptions don't obscure the normal path of execution. This is important for both the readability and performance.

#### The conditional should be put on a separate line and wrapped in braces.

```
if (isGoal) {
    lightTheLamp();
}

// NOT: if (isGoal) lightTheLamp();

```

#### Write the expression of a conditional similar to how you would speak it out loud.

```
if (someVariable == 0) {
    doSomething();
}
// NOT: if (0 == someVariable)

```

### Miscellaneous

#### Constants and Magic Numbers

#### The use of magic numbers in the code should be avoided.

- Numbers other than 0 and 1 should be considered declared as named constants instead.
- If the number does not have an obvious meaning by itself, the readability is enhanced by introducing a named constant instead.
- A different approach is to introduce a method from which the constant can be accessed.

##### Declare constants closest to the scope of their use.

```
bool checkValueLimit(int value) {
    const int ValueLimit = 10; // I only use this constant here, define it here in context
    return (value > ValueLimit);
}

```

##### Use const typed variables instead of #define

```
const float LARGEST_VALUE = 10000.0f;
// NOT:  #define LARGEST_VALUE 10000.0f

```

#### Floating point constants should always be written with decimal point and at least one decimal.

```
double stickLength = 0.0;    // NOT:  double stickLength = 0;

double penaltyMinutes;
...
penaltyMinutes = (minor + misconduct) * 2.0;

```

#### Floating point constants should always be written with a digit before the decimal point.

```
double penaltyMinutes = 0.5;  // NOT:  double penaltyMinutes = .5;

```

#### When using a single precision float type, include the trailing f.

```
float penaltyMinutes = 0.5f;  // NOT:  float penaltyMinutes = 0.5;

```

## Layout and Comments

### Layout

#### Basic indentation should be 4.

```
if (player.isCaptain) {
    player.yellAtReferee();
}

```

#### Use inline braces for block layout

```
while (!puckHeld) {
    lookForRebound();
}

// NOT:
// while (!puckHeld)
// {
//     lookForRebound();
// }

```

#### The class declarations should have the following form:

```
class GoalieStick : public HockeyStick {
public:
    ...
protected:
    ...
private:
    ...
};

```

#### Method definitions should have the following form:

```
void goalCelebration() {
    ...
}

```

#### The if-else class of statements should have the following form:

```
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

#### A for statement should have the following form:

```
for (int i = 0; i < GRETZKY_NUMBER; i++) {
    getActivePlayerWithNumber(i);
}

```

#### A while statement should have the following form:

```
while (!whistle) {
    keepPlaying();
}

```

#### A do-while statement should have the following form:

```
do {
    skate();
} while (!tired);

```

#### Switch/Case Statements:

A switch statements should follow the following basic formatting rules:

- The case statements are indented one indent (4 spaces) from the switch.
- The code for each case should be indented one indent (4 spaces) from the case statement.
- Each separate case should have a break statement, unless it is explicitly intended for the case to fall through to the subsequent cases. In the event that a case statement executes some code, then falls through to the next case, you must include an explicit comment noting that this is intentional.
- Break statements should be aligned with the code of the case, e.g. indented 4 spaces from the case statement.
- In the event that brackets are required to create local scope, the open bracket should appear on the same line as the case, and the close bracket should appear on the line immediately following the break aligned with the case statement.

Examples of acceptable form are:

```
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

#### A try-catch statement should have the following form:

```
try {
    tradePlayer();
} catch (const NoTradeClauseException& exception) {
    negotiateNoTradeClause();
}

```

#### Single statement if-else, for or while statements must be written with brackets.

### 4.2 White space

#### Conventional operators should be surrounded by a space character, except in cases like mathematical expressions where it is easier to visually parse when spaces are used to enhance the grouping.

```
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

#### C++ reserved words should be followed by a white space.Commas should be followed by a white space.

```
setLine(leftWing, center, rightWing, leftDefense, rightDefense);
// NOT: setLine(leftWing,center,rightWing,leftDefense,rightDefense);

```

#### Semicolons in for statments should be followed by a space character.

```
for (i = 0; i < 10; i++) {  // NOT: for(i=0;i<10;i++){

```

#### Declaring and Calling Functions

- Function names should not be followed by a white space.
- And there should be no space between the open parenthesis and the first parameter, and no space between the last parameter and the close parenthesis.

Examples:

```
setCaptain(ovechkin);
// NOT: setCaptain (ovechkin);
// NOT: doSomething( int foo, float bar );

```

#### Logical units within a block should be separated by one blank line.

```
Team penguins = new Team();

Player crosby = new Player();
Player fleury = new Player();

penguins.setCaptain(crosby);
penguins.setGoalie(fleury);

penguins.hireCoach();

```

#### Avoid adding optional spaces across multi-line statements and adjacent statements.

Avoid the following:

```
oddsToWin = (averageAge     * veteranWeight) +
            (numStarPlayers * starPlayerWeight) +
            (goalieOverall  * goalieWeight);

theGreatOneSlapShotSpeed  = computeShot(stickFlex, chara);
charaSlapShotSpeed        = computeShot(stickFlex, weber);

```

A change to the length of a variable in these sections causes unnecessary changes to the other lines.

#### Multi-line statements must have all n+1 lines indented at least one level (four spaces).

Align all n+2 lines with the indentation of the n+1 line.

When the multiple lines are bound by parentheses (as in arguments to a function call), the prefered style has no whitespace after the opening parenthesis or before the closing parenthesis. The n+1 lines are generally indented to the column immediately after the opening parenthesis (following the style for split expressions in 2.1.6).

When the multiple lines are bound by braces (as in C++ initializers or JavaScript object notation), the preferred style has a newline after the opening brace and newline before the closing brace. The final line should not end in a comma, and no line should begin with a comma. The closing brace should begin in the same colum as the line that has the opening brace (following the style for split control statements in 4.1).

Expressions, including C++ initializers and JavaScript object notation literals, can be placed on a single line if they are not deeply nested and end well within the column limit (2.1.4).

The following are all acceptable:

```
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

### 4.3 Comments

#### All comments should be written in English

In an international environment English is the preferred language.

#### Use // for all comments, including multi-line comments.

An exception to this rule applies for Doxygen comments where there are three slashes.

```
// Comment spanning
// more than one line.

```

There should be a space between the "//" and the actual comment

#### Comments should be included relative to their position in the code

```
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

#### Source files (header and implementation) must include a boilerplate.

Boilerplates should include the filename, location, creator, copyright, and Apache 2.0 License information and be placed at the top of the file.

```
//
//  NodeList.h
//  libraries/shared/src
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  This is where you could place an optional one line comment about the file.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

```

#### Never include Horizontal "line break" style comment blocks

These types of comments are explicitly not allowed. If you need to break up sections of code, just leave an extra blank line.

```
//////////////////////////////////////////////////////////////////////////////////

/********************************************************************************/

//--------------------------------------------------------------------------------
```

