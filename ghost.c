#include "defs.h"
#include "helpers.h"
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

void* ghost_thread_fn(void* arg) {
    struct Ghost* g = (struct Ghost*)arg;
    if (!g || !g->current_room) return NULL;

    srand(time(NULL) ^ g->id);
    g->running = true;
    log_ghost_init(g->id, g->current_room->name, g->type);

    int boredom = 0;
    int loop_count = 0;

    while (boredom < ENTITY_BOREDOM_MAX && g->running) {
        loop_count++;
        
        // Check for hunters in room
        bool hunters_present = room_has_hunters(g->current_room);
        if (hunters_present) {
            boredom = 0; // Ghost stays to scare hunters
        } else {
            boredom++;
        }

        // Exit if bored (but not too early - give hunters time to explore)
        if (boredom >= ENTITY_BOREDOM_MAX && loop_count > 50) {
            log_ghost_exit(g->id, boredom, g->current_room->name);
            g->running = false;
            break;
        }

        // Random action: idle, haunt, move
        int action = rand_int_threadsafe(0, 3); // 0=idle, 1=haunt, 2=move

        if (action == 0) {
            log_ghost_idle(g->id, boredom, g->current_room->name);
        } else if (action == 1) { // Haunt / place evidence
            EvidenceByte evidence_options = g->type;
            EvidenceByte placed = 1 << rand_int_threadsafe(0, 7);
            placed &= evidence_options;
            if (placed) {
                pthread_mutex_lock(&g->current_room->mutex);
                g->current_room->evidence_here |= placed;
                pthread_mutex_unlock(&g->current_room->mutex);
                log_ghost_evidence(g->id, boredom, g->current_room->name, placed);
                
                // EMF evidence gives hunters immediate fear reaction
                if (placed == EV_EMF && room_has_hunters(g->current_room)) {
                    // Hunter will react to this on their next loop
                }
            }
        } else if (action == 2 && g->current_room->conn_count > 0) { // Move
            // Ghost cannot move if hunters are present (they want to scare them)
            if (!hunters_present) {
                int idx = rand_int_threadsafe(0, g->current_room->conn_count);
                struct Room* old = g->current_room;
                g->current_room = g->current_room->connections[idx];
                log_ghost_move(g->id, boredom, old->name, g->current_room->name);
            } else {
                // Ghost stays put to scare hunters
                log_ghost_idle(g->id, boredom, g->current_room->name);
            }
        }

        usleep(300000); // 300ms delay - slightly slower than hunters
    }

    return NULL;
}