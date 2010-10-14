#include "pic24_all.h"
#include "snsl.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    configBasic(HELLO_MSG);
    
    //char *name = 
    char name[MAX_NODE_NAME_LEN];
    //char *name = SNSL_GetNodeName();
    printf("Current node names: `%s`\n", SNSL_GetNodeName());
    //free(name);
    
    outString("\nEnter new node name: ");
    inStringEcho(name, MAX_NODE_NAME_LEN-1);

    printf("\nSetting Node Name to: `%s`", name);
    SNSL_SetNodeName(name);

    //name = SNSL_GetNodeName();
    printf("Current node names: `%s`\n", SNSL_GetNodeName());
    //free(name);
    while(1){}
    return 0;
}
