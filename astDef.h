#ifndef ASTDEF_H
#define ASTDEF_H

#include "lexerDef.h"

struct astNode{
    char name[50];
    
    int tag;
    value val;
    
    struct astNode* child;
    struct astNode* next;

    struct astNode* left;
    struct astNode* right;
    int type; // 1 - INTEGER , 2 - FLOAT , 3 - BOOLEAN 
    int lineno;
} astNode;

int gLineNo = -1;

#endif
