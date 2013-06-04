//
//  OctalCode.h
//  hifi
//
//  Created by Stephen Birarda on 3/15/13.
//
//

#ifndef __hifi__OctalCode__
#define __hifi__OctalCode__

#include <string.h>

const int BITS_IN_BYTE  = 8;
const int BITS_IN_OCTAL = 3;
const int NUMBER_OF_COLORS = 3; // RGB!
const int SIZE_OF_COLOR_DATA = NUMBER_OF_COLORS * sizeof(unsigned char); // size in bytes
const int RED_INDEX   = 0;
const int GREEN_INDEX = 1;
const int BLUE_INDEX  = 2;

void printOctalCode(unsigned char * octalCode);
int bytesRequiredForCodeLength(unsigned char threeBitCodes);
bool isDirectParentOfChild(unsigned char *parentOctalCode, unsigned char * childOctalCode);
int branchIndexWithDescendant(unsigned char * ancestorOctalCode, unsigned char * descendantOctalCode);
unsigned char * childOctalCode(unsigned char * parentOctalCode, char childNumber);
int numberOfThreeBitSectionsInCode(unsigned char * octalCode);
unsigned char* chopOctalCode(unsigned char* originalOctalCode, int chopLevels);
unsigned char* rebaseOctalCode(unsigned char* originalOctalCode, unsigned char* newParentOctalCode, 
                               bool includeColorSpace = false);

// Note: copyFirstVertexForCode() is preferred because it doesn't allocate memory for the return
// but other than that these do the same thing.
float * firstVertexForCode(unsigned char * octalCode);
void copyFirstVertexForCode(unsigned char * octalCode, float* output);

typedef enum {
    ILLEGAL_CODE = -2,
    LESS_THAN = -1,
    EXACT_MATCH = 0,
    GREATER_THAN = 1
} OctalCodeComparison;

OctalCodeComparison compareOctalCodes(unsigned char* code1, unsigned char* code2);
#endif /* defined(__hifi__OctalCode__) */
