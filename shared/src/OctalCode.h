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

void printOctalCode(unsigned char * octalCode);
int bytesRequiredForCodeLength(unsigned char threeBitCodes);
bool isDirectParentOfChild(unsigned char *parentOctalCode, unsigned char * childOctalCode);
int branchIndexWithDescendant(unsigned char * ancestorOctalCode, unsigned char * descendantOctalCode);
unsigned char * childOctalCode(unsigned char * parentOctalCode, char childNumber);
float * firstVertexForCode(unsigned char * octalCode);

#endif /* defined(__hifi__OctalCode__) */
