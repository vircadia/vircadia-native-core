//
//  OctalCode.h
//  hifi
//
//  Created by Stephen Birarda on 3/15/13.
//
//

#ifndef __hifi__OctalCode__
#define __hifi__OctalCode__

#include <iostream>

void printOctalCode(unsigned char * octalCode);
int bytesRequiredForCodeLength(unsigned char threeBitCodes);
unsigned char * childOctalCode(unsigned char * parentOctalCode, char childNumber);

#endif /* defined(__hifi__OctalCode__) */
