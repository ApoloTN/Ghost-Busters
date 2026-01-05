#include "roomstack.h"
#include <stdlib.h>

void stack_init(struct RoomStack* stack) {
    stack->top = NULL;
}

bool stack_push(struct RoomStack* stack, struct Room* room) {
    struct RoomStackNode* node = malloc(sizeof(struct RoomStackNode));
    if (!node) return false;
    node->room = room;
    node->next = stack->top;
    stack->top = node;
    return true;
}

struct Room* stack_pop(struct RoomStack* stack) {
    if (!stack->top) return NULL;
    struct RoomStackNode* node = stack->top;
    struct Room* r = node->room;
    stack->top = node->next;
    free(node);
    return r;
}

void stack_clear(struct RoomStack* stack) {
    while(stack_pop(stack));
}
