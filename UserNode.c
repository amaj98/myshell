#include<stdio.h> 
#include<stdlib.h> 
#include<string.h>
#include "sh.h"

void addUser(struct User* first, char* name){
    User* u = first;

    if(first != NULL){
        while(u->next != NULL){
            u = u->next;
        }
    }

    User* tmp = (User*)malloc(sizeof(User));

    tmp->name = name;
    tmp->on = 0;

    tmp->next = NULL;
    strcpy(tmp->name, name);

    if(first != NULL){
        u->next = tmp;
    }
    else
    {
        first = tmp;
    }

    return first;
}

int userLogin(User* first, char* name){
    User* u = first;

    while(u != NULL){
        if(strcmp(name, u->name) == 0 && !u->on){
            u->on = 1;
            return 1;
        }
        u = u->next;
    }

    return 0;
}

void removeUser(User* first, char* name){
    User* u = first;

    //At first of list
    if(strcmp(u->name, name) == 0){
        first = u->next;
        free(first->name);
        free(first);
        return first;
    }

    while(u != NULL){
        User* next = u->next;

        if(next != NULL){
            if(strcmp(next->name, name) == 0){
                u->next = next->next;
                free(next->name);
                free(next);
            }
        }

        u = next;
    }

    return first;
}

void freeUsers( User* first){
    User* u = first;
    while(u != NULL){
        User* tmp = u;
        u = u->next;
        free(tmp->name);
        free(tmp);
    }
}
