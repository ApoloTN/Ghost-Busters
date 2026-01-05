#ifndef ROOMSTACK_H
#define ROOMSTACK_H


#include <stdbool.h>

struct Room;  // forward declaration


struct RoomStackNode {
    struct Room* room;
    struct RoomStackNode* next;
};

struct RoomStack {
    struct RoomStackNode* top;
};

// Initialize an empty stack
void stack_init(struct RoomStack* stack);

// Push a room onto the stack
bool stack_push(struct RoomStack* stack, struct Room* room);

// Pop a room from the stack
struct Room* stack_pop(struct RoomStack* stack);

// Clear the entire stack
void stack_clear(struct RoomStack* stack);

#endif // ROOMSTACK_H
