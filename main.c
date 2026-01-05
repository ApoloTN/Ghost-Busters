// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <stdbool.h>

#include "defs.h"
#include "helpers.h"

// Forward declarations (threads)
void* ghost_thread_fn(void* arg);
void* hunter_thread_fn(void* arg);

// ---------- Main ----------
int main() {
    struct House house;
    house_populate_rooms(&house);

    // initialize casefile and house mutex
    house.casefile.collected = 0;
    house.casefile.solved = false;
    sem_init(&house.casefile.mutex, 0, 1);
    pthread_mutex_init(&house.house_mutex, NULL);

    // Ghost init
    struct Ghost ghost;
    ghost.id = DEFAULT_GHOST_ID;
    const enum GhostType* ghost_types = NULL;
    int g_count = get_all_ghost_types(&ghost_types);
    if (g_count <= 0) {
        fprintf(stderr, "No ghost types available\n");
        return 1;
    }
    ghost.type = ghost_types[rand_int_threadsafe(0, g_count)];
    ghost.current_room = &house.rooms[rand_int_threadsafe(0, house.room_count)];
    ghost.running = true;

    // Hunters input
    struct Hunter hunters[4];
    int hunter_count = 0;
    char name[MAX_HUNTER_NAME];
    int id;
    while (hunter_count < 4) {
        printf("Enter hunter %d name (or 'done'): ", hunter_count + 1);
        if (!fgets(name, sizeof(name), stdin)) break;
        name[strcspn(name, "\n")] = 0;
        if (strcmp(name, "done") == 0) break;

        printf("Enter hunter ID: ");
        if (scanf("%d", &id) != 1) break;
        while (getchar() != '\n'); // flush newline

        struct Hunter* h = &hunters[hunter_count];
        h->id = id;
        strncpy(h->name, name, MAX_HUNTER_NAME - 1);
        h->name[MAX_HUNTER_NAME - 1] = '\0';
        h->current_room = house.starting_room;
        h->boredom = 0;
        h->fear = 0;
        h->collected = 0;
        hunter_count++;
    }

    if (hunter_count == 0) {
        printf("No hunters provided. Exiting.\n");
        sem_destroy(&house.casefile.mutex);
        pthread_mutex_destroy(&house.house_mutex);
        return 0;
    }

    // Start ghost thread first
    if (pthread_create(&ghost.thread, NULL, ghost_thread_fn, &ghost) != 0) {
        perror("Failed to create ghost thread");
        sem_destroy(&house.casefile.mutex);
        pthread_mutex_destroy(&house.house_mutex);
        return 1;
    }

    // Start ALL hunter threads at once (they will run concurrently with each other and the ghost)
    for (int i = 0; i < hunter_count; i++) {
        if (pthread_create(&hunters[i].thread, NULL, hunter_thread_fn, &hunters[i]) != 0) {
            perror("Failed to create hunter thread");
            // If a thread creation fails, mark remaining as not created (you may want to handle cleanup more thoroughly)
            hunter_count = i;
            break;
        }
    
    }

    // Wait for ALL hunter threads to finish
    for (int i = 0; i < hunter_count; i++) {
        pthread_join(hunters[i].thread, NULL);
    }

    // After all hunters finish, stop the ghost and join it
    ghost.running = false;
    pthread_join(ghost.thread, NULL);

    // Collect results
    printf("\nSimulation Results:\n");

    bool ghost_caught = false;
    EvidenceByte total_evidence = 0;

    //Go through each hunters stats for the game
    for (int i = 0; i < hunter_count; i++) {
        printf("Hunter %s (ID %d) exited due to %s, collected: 0x%02X\n",
               hunters[i].name, hunters[i].id,
               exit_reason_to_string(hunters[i].exit_reason),
               hunters[i].collected);

        total_evidence |= hunters[i].collected;
        if (hunters[i].exit_reason == LR_EVIDENCE) {
            ghost_caught = true;
        }
    }

    printf("\n=== FINAL RESULTS ===\n");
    printf("Ghost type: %s\n", ghost_to_string(ghost.type));
    printf("Total evidence collected: 0x%02X\n", total_evidence);

    if ((total_evidence & (EvidenceByte)ghost.type) == ghost.type) {
        ghost_caught = true;
    }

    if (ghost_caught){
        printf("🎉 GHOST CAUGHT! The hunters successfully identified the ghost.");
    } else {
        printf("\n❌ Ghost escaped! Analysis of collected evidence:\n");
    

        // Show what evidence was found
        printf("Evidence types found: ");
        bool first = true; 
        for (int i = 0; i < 7; i++) {
            enum EvidenceType ev = 1 << i;
            if (total_evidence & ev) {
                if (!first) printf(", ");
                printf("%s", evidence_to_string(ev));
                first = false;
            }
        }
        if (first) printf("None");
        printf("\n");

        // Check potential ghost matches
        printf("\nPotential ghosts based on evidence:\n");
        const enum GhostType* all_ghost_types = NULL;
        int ghost_count = get_all_ghost_types(&all_ghost_types);
        bool matches_found = false;

        for (int i = 0; i < ghost_count; i++) {
            if ((total_evidence & (EvidenceByte)all_ghost_types[i]) == total_evidence && total_evidence != 0) {
                printf("  - %s (requires: ", ghost_to_string(all_ghost_types[i]));

                bool first_ev = true;
                for (int j = 0; j < 7; j++) {
                    enum EvidenceType ev = 1 << j;
                    if (all_ghost_types[i] & ev) {
                        if (!first_ev) printf(", ");
                        printf("%s", evidence_to_string(ev));
                        first_ev = false;
                    }
                }
                printf(")\n");
                matches_found = true;
            }
        }

        if (!matches_found) {
            printf("  No ghost types match the collected evidence.\n");
            printf("  Need more specific evidence to identify the ghost.\n");
        }

        // Show what evidence the actual ghost leaves
        printf("\nThe actual ghost (%s) leaves evidence: ", ghost_to_string(ghost.type));
        bool first_real = true;
        for (int i = 0; i < 7; i++) {
            enum EvidenceType ev = 1 << i;
            if (ghost.type & ev) {
                if (!first_real) printf(", ");
                printf("%s", evidence_to_string(ev));
                first_real = false;
            }
        }
        printf("\n");
    }

    sem_destroy(&house.casefile.mutex);
    pthread_mutex_destroy(&house.house_mutex);
    return 0;
}
