#include "ast.h"
#include "symbolTableDef.h"
#include "symbolTable.h"
#include "constructST.h"

functionTable* globalFuncTable;
variableTable* currentVarTable;
variableTable* driverVarTable;
int globalNestingLevel = 0;

struct expNode* expRoot;    // Expression Node

// For generateAB_Expression
char tempStr[50];
arr_index tempArrIndex;

int otherModPos;

int getType(struct astNode* root){  // initially root is expression!
    // 0,1,2,3,4 all good
    //type mismatch
    //array operation illegal
    //undeclared variable being used
    //means non array can't be accessed by index
    //array index is non integer
    //bounds incorrect
    if(root==NULL)
        return 0;
        
    if(root->left==NULL && root->right==NULL){
        if(strcmp(root->name,"NUM")==0){
            return 1;
        }
        else if(strcmp(root->name,"RNUM")==0){
            return 2;
        }
        else if(strcmp(root->name,"boolConstt")==0){
            return 3;
        }
        else if(strcmp(root->name,"ID")==0){
            if(searchNested(currentVarTable,root->val.s)){
                variableTableEntry* idEntry = searchNestedRetrieve(currentVarTable,root->val.s);

                if(idEntry->isArray){
                    return 3+idEntry->tag;
                    // 4 -Integer Array
                    // 5 - Float Array
                    // 6 - Boolean Array
                }

                return(idEntry->tag);
            }
            else{
                printf("Error: Variable %s not declared before use\n",root->val.s);
                return -1;
            }
        }
        else{
            printf("Unexpected %s\n",root->name);
        }
    }
    

    // TODO : ARRAY index
    if(strcmp(root->name,"ARRAY_ELE")==0){
        if(searchNested(currentVarTable,root->left->val.s)){
            variableTableEntry* arrIdEntry = searchNestedRetrieve(currentVarTable,root->left->val.s);
            if(arrIdEntry->isArray){
                // Check Bounds for the given index
                    if(root->right->tag==1){// index is NUM
                        int index = root->right->val.i;
                        if(arrIdEntry->isArrayStatic){
                            if(index>(arrIdEntry->lowerBound) && index<(arrIdEntry->upperBound)){  // Within Bounds
                                printf("ARRAY Element %s[%d] within bounds\n",root->left->val.s,index);
                                return arrIdEntry->tag;
                            }else{
                                printf("Error: Index %d out of Bounds for Array %s\n",index,root->left->val.s);
                                return -1;
                            }
                        }else{
                            // if dynamic created
                            return arrIdEntry->tag;
                        }
                    }else{
                        // A[b] - index is ID
                        char* indexId = root->val.s;
                        if(searchNested(currentVarTable,indexId)){
                            variableTableEntry* indexIdEntry = searchNestedRetrieve(currentVarTable,indexId);

                            if(indexIdEntry->isArray){
                                // A[B] - both A and B are array
                                printf("Error: Index %s cannot be an Array\n",indexId);
                                return -1;
                            }

                            if(indexIdEntry->tag==1){   // Index is Valid - Integer
                                return arrIdEntry->tag; // return array type
                            }else{// A[b] - b is not an integer type
                                printf("Error: Index %s is not an Integer Type\n",indexId);
                                return -1;
                            }
                        }
                        else{// A[b] - b not declared!
                            printf("Error: Index %s is not Declared\n",indexId);
                            return -1;
                        }
                    }
            }else{
                // Non array accessed by index, A[b], A is not array
                printf("Error: Non-Array %s cannot be accessed by an index\n",root->left->val.s);
                return -1;
            }
        }else{
            // if searchNested didn't found the Array ID
            printf("Error: ID %s is not declared\n",root->left->val.s);
            return -1;            
        }
    }

    int l=getType(root->left);
    int r=getType(root->right);

    if(l==-1 || r==-1){
        return -1;
    }
    if(r!=0 && l!=r){
        return -1;
    }
    if(l>=4 && r!=0){
        printf("Error: Array operations aren't permitted\n");
        return -1;
    }
    if(r>=4){
        printf("Error: Array operations aren't permitted\n");
        return -1;
    }
    if(l==r){
        if(strcmp(root->name,"relationalOp")==0){
            return 3;
        }
    }
    return l;
}

void printExpression(struct expNode* root){

    printf("Expression : \t");

    while(root!=NULL){

        switch (root->tag)
        {
        case 0:
            printf("%s ",root->name);
            break;
        
        case 1:
            printf("NUM ");
            break;

        case 2:
            printf("RNUM ");
            break;

        case 3:
            printf("BOOL ");
            break;


        case 4:
            printf("%s ",root->name);
            break;

        case 5:
            if(root->isDynamic)
            printf("%s[%s] ",root->name,root->index.s);
            else
            printf("%s[%d] ",root->name,root->index.i);
            break;

        default:
            break;
        }


        root = root->next;
    }


    printf("\n\n");
}

bool checkIfArrayType(struct astNode* node){

    if(node->child==NULL)
    return false;
    else{
        return true;
    }
}

struct expNode* makeExpNode(int tag,char* name,bool isDynamic,arr_index index){
    struct expNode* node = (struct expNode*) malloc(sizeof(struct expNode));
    node->tag = tag;

    if(tag==4 || tag==0) // id or op
    strcpy(node->name,name);

    if(tag==5){
        node->isDynamic = isDynamic;

        if(isDynamic==false)
        node->index.i = index.i;
        else
        strcpy(node->index.s,index.s);
    }

    node->next = NULL;

    return node;
}


void constructST(struct astNode* root){

        printf("constructST : %s\n",root->name);

        if(root==NULL)
        {
            return;
            // Report Error!
            printf("Root is NULL");
            exit(0);
        }

        struct astNode* temp = root; // Using this for traversal

        if(strcmp(root->name,"program")==0){
            // <program>  -->  <moduleDeclarations> <otherModules><driverModule><otherModules>
            temp = temp->child; // <moduleDeclarations>
            constructST(temp);
            otherModPos = 1;
            temp = temp->next; // <otherModules1>
            constructST(temp);
            temp = temp->next; // <driverModule>
            constructST(temp);
            otherModPos = 2;
            temp = temp->next; // <otherModules2>
            constructST(temp); // Treat this in a different manner!

            return; // Not necessary!!
        }   

        if(strcmp(root->name,"moduleDeclarations")==0){
           // <moduleDeclarations>  -->  <moduleDeclaration><moduleDeclarations> 
           // <moduleDeclaration>  -->  DECLARE MODULE ID SEMICOL 
           // <moduleDeclarations>  -->  ε

            temp = temp->child; // ID or NULL

            while(temp!=NULL){
                // DONE - Wil get Module ID from here!
                char funcName[40];
                strcpy(funcName,temp->val.s);
                //printf("%s\n",funcName);
                
                if(strcmp("ε",funcName)!=0)
                {
                    if(searchInFunTable(globalFuncTable,funcName))
                        printf("Error: Redeclaration of function %s at line number %d \n",funcName,temp->lineno);
                    else{
                        insertInFunTable(globalFuncTable,funcName,NULL,NULL);
                        updateDefineBool(globalFuncTable,funcName,false);
                    }
                        
                }
                //printFunTable(globalFuncTable); // TODO - take care of input and output list
                //TODO - redeclaration error
                temp = temp->next; // ID or NULL;
            }
            //printFunTable(globalFuncTable);
            return;
        }

        if(strcmp(root->name,"driverModule")==0){
            temp = temp->child; // <moduleDef>
            // <driverModule> -->  DRIVERDEF DRIVER PROGRAM DRIVERENDDEF <moduleDef>
            // <moduleDef>   -->  START <statements> END
            // <statements>  -->  <statement> <statements>  
            // ...

            //DONE
            currentVarTable = driverVarTable;
            constructST(temp); // if for moduleDef written below!
        }


        if(strcmp(root->name,"otherModules")==0){
            // <otherModules>   -->  ε
            // <otherModules>   -->  <module> <otherModules>
            while(temp->child!=NULL){ 
                constructST(temp->child); // module
                temp = temp->child->next; // otherModules
            }
        }

        if(strcmp(root->name,"module")==0){
            // <module> -->  DEF MODULE ID ENDDEF TAKES INPUT SQBO <input_plist> SQBC SEMICOL <ret> <moduleDef>
        
            temp = temp->child; // ID
            char funcName[40];
            strcpy(funcName,temp->val.s);

            if(otherModPos==1)
            {
                //TODO - Redeclaration
                if(searchInFunTable(globalFuncTable,funcName) )
                {
                    // f1 func defin
                    // f1 func defin  ERROR 
                    functionTableEntry* ftemp = retrieveFunTable(globalFuncTable,funcName);
                    if(ftemp->isDefined)
                    {
                        printf("Error: Redefinition of function %s at line number %d \n",funcName,temp->lineno);
                        return; // Re-defined Function Skipped 
                    }else{
                        updateDefineBool(globalFuncTable,funcName,true);
                    }
                    
                    // TODO1 : Tell Symbol Table that this function was first came by definition
                    // f1 declare
                    // f1 func defin NOT ERROR 
                    
                }
                //DONE - Simply Add to Global Func Table
                //printf("%s\n",funcName);
                else{
                    insertInFunTable(globalFuncTable,funcName,NULL,NULL); // TODO - check for errors, take care of input and output lists
                    updateDefineBool(globalFuncTable,funcName,true);
                }    
                //printFunTable(globalFuncTable);

            }else if(otherModPos==2){
                // DONE - Check if definition was prev there or not! If not, then error will be printed!
                
                if(!searchInFunTable(globalFuncTable,funcName)){

                    printf("Error: On line number %d , Function %s Not Declared Before Driver.\n",temp->lineno,funcName);
                    insertInFunTable(globalFuncTable,funcName,NULL,NULL); 
                    updateDefineBool(globalFuncTable,funcName,true);

                }else{
                    functionTableEntry* ftemp = retrieveFunTable(globalFuncTable,funcName);
                    if(ftemp->isDefined){
                        printf("Error: On line number %d , Redefinition of function %s\n",temp->lineno,funcName);
                        return; // Re-defined Function Skipped 
                    }else{
                        updateDefineBool(globalFuncTable,funcName,true);
                    }

                }
                //printf("%s\n",funcName);
                //printFunTable(globalFuncTable);
            }

            // DONE: currentVarTable = current var table of curr Function!
            currentVarTable = (retrieveFunTable(globalFuncTable,funcName))->localVarTable;
            //printVarTable(currentVarTable);

            temp = temp->next; // <input_plist>
            // Code for input_plist
            // <input_plist>  -->  ID COLON <dataType><N1>

            temp = temp->child; // First ID
            char idName[40];
            strcpy(idName,temp->val.s);
            //printf("INPUTINPUTINPUT - %s\n",idName);
            // TODO:  Add this ID = temp->child to function table (input)  with type at temp->next
            temp = temp->next; // <dataType>
            struct astNode* temp2 = temp;    // <N1>
            // Datatype
            
            if(!checkIfArrayType(temp)){//NON ARRAY
                // temp->tag;
                insertInVarTable(currentVarTable,idName,false,temp->tag,0); // doing for function, nesting level will always be zero. // TODO - check for errors
                functionTableEntry* ft = retrieveFunTable(globalFuncTable,funcName);
                ft->inputList = initializeParameter(idName,false,temp->tag);
                //printParameterList(ft->inputList);
                //printVarTable(currentVarTable);
            }else{//ARRAY
                // <dataType>  -->   ARRAY SQBO <range_arrays> SQBC OF <type>
                // <range_arrays>  -->  <index> RANGEOP <index>
                temp2 = temp2->child; // <range_arrays>
                int type = temp2->next->tag;    
                int low,high;
                char lowId[40], highId[40];

                // printf("Check51 : %d!\n",temp->tag);

                temp2 = temp2->child;


                if(temp2->tag==1){ //static
                    low = temp2->val.i;
                    high = temp2->next->val.i;
                    //printf("low = %d , high = %d \n",low,high);
                    insertInVarTable(currentVarTable,idName,true,type,0);
                    updateArrayVarStatic(currentVarTable,idName,low,high);

                    functionTableEntry* ft = retrieveFunTable(globalFuncTable,funcName);
                    ft->inputList = initializeParameter(idName,true,temp2->tag);
                    updateParameterArrayStatic(ft->inputList,low,high);
                
                }else{ //dynamic
                    strcpy(lowId,temp2->val.s);
                    strcpy(highId,temp2->next->val.s);

                    insertInVarTable(currentVarTable,idName,true,type,0);
                    updateArrayVarDynamic(currentVarTable,idName,lowId,highId);

                    functionTableEntry* ft = retrieveFunTable(globalFuncTable,funcName);
                    ft->inputList = initializeParameter(idName,true,temp2->tag);
                    updateParameterArrayDynamic(ft->inputList,lowId,highId);
                }
            }

            //printVarTable(currentVarTable);

            temp = temp->next;  // <N1> = ID <dataType> ID <dataType> ...
            // <N1>  -->  COMMA ID COLON <dataType> <N1>
            while(temp!=NULL){
                // temp = ID
                //printf("VALUE N1 - %s \n",temp->val.s);
                strcpy(idName,temp->val.s);

                if(searchInVarTable(currentVarTable,idName))
                {
                    printf("Error: On line number-%d, Variable Name %s has already been used in input list.",temp->lineno,idName);
                    temp = temp->next->next;
                    continue;
                }
                // Redeclaration Error - Variable already defined!

                // DONE:  Add this ID = temp->child to function table (input)  with type at temp->next
                temp = temp->next; // <dataType>
                temp2 = temp;

                if(!checkIfArrayType(temp)){
                    // temp->tag;
                    insertInVarTable(currentVarTable,idName,false,temp->tag,0); // doing for function, nesting level will always be zero. // TODO - check for errors
                    functionTableEntry* ft = retrieveFunTable(globalFuncTable,funcName);
                    parameter* ptemp = initializeParameter(idName,false,temp->tag);
                    addParametertoList(ft->inputList,ptemp);
                    //printVarTable(currentVarTable);
                }
                else{
                    temp2 = temp2->child; // <range_arrays>
                    int type = temp2->next->tag;    
                    int low,high;
                    char lowId[40], highId[40];

                    // printf("Check51 : %d!\n",temp->tag);

                    temp2 = temp2->child;


                    if(temp2->tag==1)
                    {
                        low = temp2->val.i;
                        high = temp2->next->val.i;
                        //printf("low = %d , high = %d \n",low,high);
                        insertInVarTable(currentVarTable,idName,true,type,0);
                        updateArrayVarStatic(currentVarTable,idName,low,high);

                        functionTableEntry* ft = retrieveFunTable(globalFuncTable,funcName);
                        parameter* ptemp = initializeParameter(idName,true,temp2->tag);
                        updateParameterArrayStatic(ptemp,low,high);
                        addParametertoList(ft->inputList,ptemp);
                    }
                    else
                    {
                        strcpy(lowId,temp2->val.s);
                        strcpy(highId,temp2->next->val.s);

                        insertInVarTable(currentVarTable,idName,true,type,0);
                        updateArrayVarDynamic(currentVarTable,idName,lowId,highId);

                        functionTableEntry* ft = retrieveFunTable(globalFuncTable,funcName);
                        parameter* ptemp = initializeParameter(idName,true,temp2->tag);
                        updateParameterArrayDynamic(ptemp,lowId,highId);
                        addParametertoList(ft->inputList,ptemp);
                    }
                }  
                temp = temp->next; // N1 or NULL
            }

            //printVarTable(currentVarTable);
            // Code for input_plist ends

            // <module> -->  DEF MODULE ID ENDDEF TAKES INPUT SQBO <input_plist> SQBC SEMICOL <ret> <moduleDef>
            temp = root->child; // ID
            temp = temp->next->next;    // <ret>

            // constructST(temp);
            // temp = temp->next; // <ret> = <output_plist>
            // Code for output_plist starts
            // <output_plist>  -->  ID COLON <type><N2>
            // ID type ID type .....
            // <ret>  = <output_plist> -->  ε

                if(temp->child!=NULL){
                
                temp = temp->child; // ID
                char idName[40];
                strcpy(idName,temp->val.s);

                
                //printf("OUTPUT VAR - %s\n",temp->val.s);
                
                // TODO:  Add this ID = temp->child to function table (output)  with type at temp->next
                // printf("Check9 : %s\n",temp->name);
                temp = temp->next; // <type>
                temp2 = temp;

                if(!checkIfArrayType(temp))
                {
                    insertInVarTable(currentVarTable,idName,false,temp->tag,0); 
                    // doing for function, nesting level will always be zero. // TODO - check for errors
                }
                else
                {
                    temp2 = temp2->child; // <range_arrays>
                    int type = temp2->next->tag;    
                    int low,high;
                    char lowId[40], highId[40];

                    // printf("Check51 : %d!\n",temp->tag);

                    temp2 = temp2->child;


                    if(temp2->tag==1)
                    {
                        low = temp2->val.i;
                        high = temp2->next->val.i;
                        //printf("low = %d , high = %d \n",low,high);
                        insertInVarTable(currentVarTable,idName,true,type,0);
                        updateArrayVarStatic(currentVarTable,idName,low,high);
                    }

                    else
                    {
                        strcpy(lowId,temp2->val.s);
                        strcpy(highId,temp2->next->val.s);

                        insertInVarTable(currentVarTable,idName,true,type,0);
                        updateArrayVarDynamic(currentVarTable,idName,lowId,highId);
                    }
                }
                // printf("Check10 : %s\n",temp->name);
                temp = temp->next; // <N2> = ID <type> ID <type> ...
                while(temp!=NULL){
                    // TODO:  Add this ID = temp->child to function table (output)  with type at temp->next
                    // temp is ID
                    strcpy(idName,temp->val.s);
                    temp = temp->next; // <type>
                    temp2 = temp;

                    if(!checkIfArrayType(temp))
                    {
                        insertInVarTable(currentVarTable,idName,false,temp->tag,0); 
                        // doing for function, nesting level will always be zero. // TODO - check for errors
                    }
                    
                    else
                    {
                        temp2 = temp2->child; // <range_arrays>
                        int type = temp2->next->tag;    
                        int low,high;
                        char lowId[40], highId[40];

                        // printf("Check51 : %d!\n",temp->tag);

                        temp2 = temp2->child;

                        if(temp2->tag==1)
                        {
                            low = temp2->val.i;
                            high = temp2->next->val.i;
                            //printf("low = %d , high = %d \n",low,high);
                            insertInVarTable(currentVarTable,idName,true,type,0);
                            updateArrayVarStatic(currentVarTable,idName,low,high);
                        }

                        else
                        {
                            strcpy(lowId,temp2->val.s);
                            strcpy(highId,temp2->next->val.s);

                            insertInVarTable(currentVarTable,idName,true,type,0);
                            updateArrayVarDynamic(currentVarTable,idName,lowId,highId);
                        }
                    }

                    temp = temp->next; // ID or NULL
                }
                // printVarTable(currentVarTable);
                   // Code for output_plist ends
                }


            temp = root->child; // ID
            temp = temp->next->next->next; // <moduleDef>  
            constructST(temp);
        }

        if(strcmp(root->name,"moduleDef")==0){
            // <moduleDef>   -->  START <statements> END
            temp = temp->child; // <statement>
            while(temp!=NULL){
                // TODO : Start Different Scope!
                constructST(temp);
                temp = temp->next;
            }
        }

        if(strcmp(root->name,"statements")==0){
            // <statements>  -->  <statement> <statements> 
            temp = temp->child;
            while(temp!=NULL){
                // printf("Statements Check %s\n",temp->name); 
                constructST(temp);
                temp = temp->next;
            }
        }


        if(strcmp(root->name,"ioStmt1")==0){
            // Input Stmt
            // Nothing much with Symbol Table

            temp = temp->child; // ID 
            if(strcmp(temp->name,"ID")==0){
                // TODO:  Check if ID exists in var table! If not, give error!
                // <ioStmt>  -->  GET_VALUE BO ID BC SEMICOL
                // ID will already be there in symbol table!
                return;
            }
        }

        if(strcmp(root->name,"ioStmt2")==0){
            // Output Stmt
            // Nothing much with Symbol Table

            return;
        }

        if(strcmp(root->name,"simpleStmt")==0){
            root = root->child;
            temp = root;
            if(strcmp(root->name,"assignmentStmt")==0){
                // <assignmentStmt>   -->   ID <whichStmt>
                
                temp = temp->child; // ID - type aur exp match !

                variableTableEntry* idEntry;

                // Not-Declared Error Check 
                if(searchNested(currentVarTable,temp->val.s)){
                    idEntry = searchNestedRetrieve(currentVarTable,temp->val.s);
                }else{
                    printf("Error : Identifier %s undeclared\n",temp->val.s);
                    return;
                }

                // <whichStmt>  -->  <lvalueIDStmt>
                // <whichStmt>  -->  <lvalueARRStmt>
                temp = temp->next;  // either of these two            

                if(strcmp(temp->name,"lvalueIDStmt")==0){
                    temp = temp->child; // ASSIGNOP
                    temp = temp->next;  // expression
                    int expType = getType(temp);
                    if(expType==-1){
                        printf("Error: RHS Types don't match\n"); 
                        return;
                    }

                    if(idEntry->isArray){
                        if(expType>=4){
                            // RHS ID is an Array of type = (expType-3)
                            // A = B - both are arrays
                            if(idEntry->tag==(expType-3)){
                                // Type of Array A and B are same
                                // Bound checking
                                // Here A = B
                                if(idEntry->isArrayStatic){

                                    if(searchNested(currentVarTable,temp->child->val.s)){
                                        variableTableEntry* idRHSEntry = searchNestedRetrieve(currentVarTable,temp->child->val.s);
                                        if(idRHSEntry->isArrayStatic){
                                            if(idEntry->lowerBound!=idRHSEntry->lowerBound || idEntry->upperBound!=idRHSEntry->upperBound){
                                                // Bounds of Static Arrays doesn't match
                                                printf("Error : Bounds of Array %s doesn't match with Array %s\n",idEntry->key,idRHSEntry->key);
                                                return;
                                            }
                                        }
                                    }
                                    return;
                                }
                            }
                            else{
                                // Type of Array A and B are not same
                                printf("Error : ARRAY %s cannot be assigned to a different type\n",idEntry->key);
                                return;
                            }        
                        }
                        else{
                            // RHS ID is not an array type
                            printf("Error : ARRAY %s can only perform assignment operation with an ARRAY of same type\n",idEntry->key);
                            return;
                        }
                    }else{
                        // Normal Type Checking between Ids!
                        if(expType!=idEntry->tag){
                            // LHS RHS Type Mismatch
                            printf("Error : %s LHS RHS Type Mismatch\n",idEntry->key);
                        }
                        return ;
                    }

                }else if(strcmp(temp->name,"lvalueARRStmt")==0){
                    // <lvalueARRStmt>  -->  SQBO <index> SQBC ASSIGNOP <expression> SEMICOL
                    temp = temp->child; // index

                    if(temp->tag==1){
                        // Check Bounds if array static
                        if(idEntry->isArrayStatic){
                            if(temp->val.i<idEntry->lowerBound || temp->val.i>idEntry->upperBound){
                                // Out of Bounds
                                printf("Array Index Out of Bounds for %s\n",idEntry->key);
                                return;
                            }
                        }
                    }else if(temp->tag==4){
                        // Dynamic index!
                        if(searchNested(currentVarTable,temp->val.s)){
                            variableTableEntry* indexEntry = searchNestedRetrieve(currentVarTable,temp->val.s);
                            if(indexEntry->tag!=1){
                                printf("Array index %s is not an Integer type\n",temp->val.s);
                                return;
                            }
                        }else{
                            printf("Index %s not declared\n",temp->val.s);
                            return;
                        }
                    }else{
                        printf("Unexpected index type\n");
                    }

                    temp = temp->next;  // ASSIGNOP
                    temp = temp->next;  // expression
                    int expType = getType(temp);

                    // A[i] = expression!
                    if(idEntry->tag!=expType){
                        // LHS RHS Type Mismatch
                        printf("Error : %s LHS RHS Type Mismatch\n",idEntry->key);
                    }
                    return;
                }else{
                    printf("Unexpected Error at whichStmt\n");
                    return;
                }

        }
            
        if(strcmp(root->name,"moduleReuseStmt")==0){
            // <moduleReuseStmt>  -->  <optional> USE MODULE ID WITH PARAMETERS <idList> SEMICOL

            temp = temp->child; // optional
                //  TODO : Check if func parameters size and type are correct! 

            if(temp->child==NULL){
                // <optional>  -->  ε
                temp = temp->next;  // ID
                // TODO : This is the function ID

                temp = temp->next;  //  <idList>
                temp = temp->child; // ID->ID->ID....

                while(temp!=NULL){
                    // temp has an ID(parameter of function)
                    // TODO : Function Parameters
                    temp = temp->next;
                }
                return;
            }
            else{
                // TODO : Return type of MODULE(ID) matches with L.H.S if optional isn't ε
                // <optional>  -->  SQBO <idList> SQBC ASSIGNOP
                temp = temp->child; // <idList>
                temp = temp->child; // ID->ID->ID....
                while(temp!=NULL){
                    // temp has an IDs where return value of function is stored
                    // TODO : Return Storing Parameters
                    temp = temp->next;
                }
            }

                temp = root->child;  //moduleReuseStmt
                temp = temp->child->next;   // ID
                // TODO : This is the function ID

                temp = temp->next;  //  <idList>
                temp = temp->child; // ID->ID->ID....
                while(temp!=NULL){
                    // temp has an ID(parameter of function)
                    // TODO : Function Parameters
                    temp = temp->next;
                }
            }

        }


        if(strcmp(root->name,"declareStmt")==0){
            // return;
            // <declareStmt>  -->  DECLARE <idList> COLON <dataType> SEMICOL
            // TODO : Get dataType from right <dataType> and set Ids type to that! ADD to Symbol Table(Nothing to add here)

            temp = temp->child; // <idList>
            temp = temp->next; // <dataType>
            int datatype;

            // printf("Check1\n");

            if(temp->child==NULL){
                // Not an Array 
                // printf("Check2\n");
                datatype = temp->tag; 
                //printf(" DECLARE - %d\n",datatype);
                /*
                    1 - INTEGER
                    2 - REAL/FLOAT
                    3 - BOOLEAN
                */
                temp = root->child; // <idList>
                //<idList>  -->  ID <N3>
                temp = temp->child; // ID
                // printf("Check22\n");
                while(temp!=NULL){
                    
                    // DONE: Now temp is a linked list of IDs - Add them to Symbol Table
                    //printf("DECLARE - IDS - %s\n",temp->val.s);
                    char varName[40];
                    strcpy(varName,temp->val.s);

                    insertInVarTable(currentVarTable,varName,false,datatype,globalNestingLevel); // TODO - Check for errors
                    temp = temp->next;
                }

            }else{
                // printf("Check3\n");
                // <dataType>  -->   ARRAY SQBO <range_arrays> SQBC OF <type>
                // <range_arrays>  -->  <index> RANGEOP <index>
                temp = temp->child; // <range_arrays>  -->  <index> RANGEOP <index>

                datatype = temp->next->tag; // <type> -> Array datatype

                temp = temp->child; // <index>
                int low_tag,high_tag;
                int low,high;
                char lowId[40],highId[40];

                low_tag = temp->tag; // 1 or 4
                high_tag =  temp->next->tag;
                
                // If 4, it's an ID 
                // If 1, it's an Integer
                if(low_tag==1){
                    low = temp->val.i;
                }else{
                    strcpy(lowId,temp->val.s);
                }
                
                if(high_tag==1){
                    high = temp->next->val.i;
                }else{
                    strcpy(highId,temp->next->val.s);
                }

                // TODO : How to handle dynamic and static arrays?


                temp = root->child; // <idList>
                //<idList>  -->  ID <N3>
                temp = temp->child; // ID

                while(temp!=NULL){

                    // DONE: Now temp is a linked list of IDs - Add them to Symbol Table
                    char varName[40];
                    strcpy(varName,temp->val.s);

                    if(low_tag==1) // STATIC ARRAY
                    {
                        insertInVarTable(currentVarTable,varName,true,datatype,globalNestingLevel);
                        updateArrayVarStatic(currentVarTable,varName,low,high);
                    }

                    if(low_tag==4)
                    {
                        insertInVarTable(currentVarTable,varName,true,datatype,globalNestingLevel);
                        updateArrayVarDynamic(currentVarTable,varName,lowId,highId);
                    }
                    temp = temp->next;
                }

                // <type>  -->  INTEGER
                // <type>  -->  REAL
                // <type>  -->  BOOLEAN
            }

            //printVarTable(currentVarTable);

        } // declareStmt Ends

        if(strcmp(root->name,"conditionalStmt")==0){
            // <conditionalStmt>  -->  SWITCH BO ID BC START <caseStmts> <default> END
            // DONE : Create another var table for conditionalStmt

            variableTable* tempTable = currentVarTable;
            globalNestingLevel++;
            if(tempTable->child == NULL)
            {

                variableTable* newTable = initializeVarTable();
                newTable->parent = tempTable;
                tempTable->child = newTable;
                currentVarTable = newTable;
            }
            else
            {

                variableTable* traverse = tempTable->child;

                while(traverse->next != NULL)
                    traverse = traverse->next;

                variableTable* newTable = initializeVarTable();
                traverse->next = newTable;
                newTable->parent = tempTable;
                currentVarTable = newTable;
            }
            // <caseStmts>  -->  CASE <value> COLON <statements> BREAK SEMICOL <N9>
            
            temp = temp->child; 
            temp = temp->next;  // <caseStmts>
            
            while(temp!=NULL){
                temp = temp->child; // <value>
                // <value>  -->  NUM
                // <value>  -->  TRUE
                // <value>  -->  FALSE
                // TODO : Check on Value Type Consistent - NUM/Boolean
                // TODO : Check if Boolean Value Type then no Default Stmt?
                temp = temp->next; // <statements>
                constructST(temp);

                temp = temp->next; // N9
                // <N9>  -->  CASE <value> COLON <statements> BREAK SEMICOL <N9>
                // <N9>  -->  ε
            }

            temp = root->child->next;  // <default> = <statements>

            constructST(temp);
             // <default>  -->  DEFAULT COLON <statements> BREAK SEMICOL
             // <default>  -->  ε 

            // DONE : Change Scope!
            currentVarTable = tempTable;
            globalNestingLevel--;
        } // conditionalStmt Ends

        if(strcmp(root->name,"iterativeStmt")==0){

            // <iterativeStmt>  -->  FOR BO ID IN <range> BC START <statements> END
            // <iterativeStmt>  -->  WHILE BO <arithmeticOrBooleanExpr> BC START <statements> END

            variableTable* tempTable = currentVarTable;
            globalNestingLevel++;
            if(tempTable->child == NULL)
            {
                variableTable* newTable = initializeVarTable();
                newTable->parent = tempTable;
                tempTable->child = newTable;
                currentVarTable = newTable;
            }
            else
            {
                variableTable* traverse = tempTable->child;

                while(traverse->next != NULL)
                    traverse = traverse->next;

                variableTable* newTable = initializeVarTable();
                traverse->next = newTable;
                newTable->parent = tempTable;
                currentVarTable = newTable;
            }

            if(strcmp(root->val.s,"FOR")==0){

                printf("FOR STARTS\n");
                temp = temp->child; // <ID>
                // TODO : ID for iterativeStmt 
                temp = temp->next; // <range>
                // <range>  -->  NUM1 RANGEOP NUM2
                int low,high;
                temp = temp->child; 

                low = temp->val.i;  // NUM1
                high = temp->next->val.i; // NUM2

                temp = root->child; // <ID>
                temp = temp->next;  // <range>
                temp = temp->next;  // <statements>

                // printf("Check1 : %s\n",temp->name);

                constructST(temp);
                printf("FOR ENDS\n");
            }

            else if(strcmp(root->val.s,"WHILE")==0){
                printf("WHILE STARTS\n");
                temp = temp->child; // <arithmeticOrBooleanExpr>
                // struct expNode* exp = generateExpression(temp);
                // printExpression(exp);
                // TODO : Type Checking!
                temp = temp->next;  // <statements>
                constructST(temp);
                printf("WHILE ENDS\n");
            }

            // DONE : Change Scope!
            printVarTable(currentVarTable);
            currentVarTable = tempTable;
            globalNestingLevel--;
        } // iterativeStmt Ends

}

void runConstructST(FILE* testFile, FILE* parseTreeFile){
    driverVarTable = initializeVarTable();
    globalFuncTable = initializeFunTable();
    currentVarTable = driverVarTable;
    printFunTable(globalFuncTable);

    // Same as runAst
    printf("Starting runAST\n");
    parserFree();

    FILE* fp = fopen("grammar.txt","r");

    readGrammerTextFile(fp);

    // printAllGrammar();

    if(printFlag)
    printf("\nComputing Firsts And Follows Started\n");

    computeFirstAndFollow();

    if(printFlag)
    printf("\nComputing Firsts And Follows Ended\n\n");

    if(printFlag)
    printf("\nComputing Parse Table Started\n");

    createParseTable();

    if(printFlag)
    printf("\nComputing Parse Table Ended\n\n");

    populateKeywordTable();

    programFile = testFile;
    programFile = getStream(programFile);

    if(first){
        populateKeywordTable();
        first = false;
    }


    while(true)
    {
        getNextToken();
        if(currentToken->tokterm == 60)
        break;
    }
    
    if(printFlag)
    printf("Parsing Input Source Started\n");    

    struct treeNode* rootParseTree = parseInputSourceCode(head->next, Table, grammar, firstFollowSets,parseTreeFile);
    
    // inOrderParseTree(rootParseTree,parseTreeFile);    

    printf("\nParseTreeComputed\n");

    struct astNode* rootAstNode = generateAST(rootParseTree);
    // printLevelOrder(rootAstNode);
    //generateAST(rootParseTree);
    if(printFlag)
    printf("\nAST Complete\n");

    // runAst()

    printf("\nGenerating Symbol Table\n");
    
    constructST(rootAstNode);

    printAllTables(globalFuncTable, driverVarTable);

}
