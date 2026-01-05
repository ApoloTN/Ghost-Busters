#include "defs.h"
#include "helpers.h"
#include "roomstack.h"
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

void* hunter_thread_fn(void* arg) {
    struct Hunter* h = (struct Hunter*)arg;
    if (!h || !h->current_room) return NULL;

    // Initialize breadcrumb stack
    stack_init(&h->breadcrumb);

    // R-16.3: Hunter starts with random device
    const enum EvidenceType* evidence_types;
    int evidence_count = get_all_evidence_types(&evidence_types);
    h->current_device = evidence_types[rand_int_threadsafe(0, evidence_count)];

    // R-16.4: Add hunter to van room (with special case for first room)
    pthread_mutex_lock(&h->current_room->mutex);
    if (!h->current_room->is_exit || h->current_room->occupancy_count < MAX_ROOM_OCCUPANCY) {
        room_add_hunter(h->current_room, h);
    }
    pthread_mutex_unlock(&h->current_room->mutex);

    // Initialize stats
    h->boredom = 0;
    h->fear = 0;
    h->collected = 0;

    // Log hunter starting
    log_hunter_init(h->id, h->current_room->name, h->name, h->current_device);

    bool returning_to_van = false;

    while (h->boredom < ENTITY_BOREDOM_MAX && h->fear < HUNTER_FEAR_MAX) {
        // R-17: Update Stats (Ghost Check)
        pthread_mutex_lock(&h->current_room->mutex);
        
        // Check if ghost is in room (evidence presence indicates ghost was recently here)
        bool ghost_present = (h->current_room->evidence_here != 0);
        if (ghost_present) {
            h->boredom = 0;
            h->fear++;
            
            // Also check for EMF specifically as it's the most direct ghost indicator
            if (h->current_room->evidence_here & EV_EMF) {
                h->fear++; // Extra fear for EMF readings
            }
        } else {
            h->boredom++;
        }
        
        pthread_mutex_unlock(&h->current_room->mutex);

        // R-18: Van / Exit Room Check
        if (h->current_room->is_exit) {
            // R-18.1: Clear breadcrumb stack
            stack_clear(&h->breadcrumb);
            
            // R-18.2: Check for Victory
            // Check if evidence collection is complete
            if (evidence_has_three_unique(h->collected)) {
                room_remove_hunter(h->current_room, h);
                h->exit_reason = LR_EVIDENCE;
                break;
            }
            
            // R-18.3: Swap to new device
            h->current_device = evidence_types[rand_int_threadsafe(0, evidence_count)];
        }

        // R-19: Condition Check (Boredom / Fear)
        if (h->boredom >= ENTITY_BOREDOM_MAX) {
            room_remove_hunter(h->current_room, h);
            h->exit_reason = LR_BORED;
            break;
        }
        if (h->fear >= HUNTER_FEAR_MAX) {
            room_remove_hunter(h->current_room, h);
            h->exit_reason = LR_AFRAID;
            break;
        }

        // R-20: Attempt to Gather Evidence
        pthread_mutex_lock(&h->current_room->mutex);
        
        EvidenceByte matching_evidence = h->current_room->evidence_here & h->current_device;
        
        if (matching_evidence) {
            // R-20.1.1: Clear evidence bit from room
            h->current_room->evidence_here &= ~matching_evidence;
            
            // R-20.1.2: Add to hunter's collected evidence
            h->collected |= matching_evidence;
            
            // R-20.1 Set return flag unless already in exit room
            if (!h->current_room->is_exit) {
                returning_to_van = true;
            }
            
            log_evidence(h->id, h->boredom, h->fear, h->current_room->name, h->current_device);
        } else {
            // R-20.2: Small random chance to return to van for equipment change
            if (rand_int_threadsafe(0, 100) < 20) { // 20% chance (increased for better gameplay)
                returning_to_van = true;
            }
            
            // Smart strategy: If room has evidence that hunter's device can't detect,
            // hunter should consider coming back with different equipment
            if (h->current_room->evidence_here != 0) {
                // Try a different device next time at van
                if (rand_int_threadsafe(0, 100) < 30) { // 30% chance to swap devices
                    returning_to_van = true;
                }
            }
        }
        
        pthread_mutex_unlock(&h->current_room->mutex);

        

        // Movement logic
        if (h->current_room->conn_count > 0) {
            struct Room* old = h->current_room;
            
            // Remove from current room
            pthread_mutex_lock(&old->mutex);
            room_remove_hunter(old, h);
            pthread_mutex_unlock(&old->mutex);
            
            // Choose next room (prioritize van if returning)
            int next;
            if (returning_to_van) {
                // Try to find path to van (simple implementation)
                next = 0; // default to first connection
                bool found_exit = false;
                for (int i = 0; i < old->conn_count; i++) {
                    if (old->connections[i] && old->connections[i]->is_exit) {
                        next = i;
                        found_exit = true;
                        break;
                    }
                }
                if (!found_exit) {
                    next = rand_int_threadsafe(0, old->conn_count);
                }
            } else {
                next = rand_int_threadsafe(0, old->conn_count);
            }
            
            // Validate next connection exists
            if (next >= 0 && next < old->conn_count && old->connections[next]) {
                h->current_room = old->connections[next];
            } else {
                // Invalid connection, skip movement
                pthread_mutex_lock(&old->mutex);
                room_add_hunter(old, h);
                pthread_mutex_unlock(&old->mutex);
                usleep(200000);
                continue;
            }

            // Add to new room
            pthread_mutex_lock(&h->current_room->mutex);
            if (!room_add_hunter(h->current_room, h)) {
                // Room full, go back
                h->current_room = old;
                room_add_hunter(old, h);
            }
            pthread_mutex_unlock(&h->current_room->mutex);

            log_move(h->id, h->boredom, h->fear, old->name, h->current_room->name, h->current_device);

            // Push old room to breadcrumb stack if exploring
            if (!returning_to_van) {
                stack_push(&h->breadcrumb, old);
            } else if (h->current_room->is_exit) {
                returning_to_van = false;
            }
        }

        usleep(200000); // 200ms delay
    }

    // Log exit and clear breadcrumb stack
    log_exit(h->id, h->boredom, h->fear, h->current_room->name, h->current_device, h->exit_reason);
    stack_clear(&h->breadcrumb);

    return NULL;
}