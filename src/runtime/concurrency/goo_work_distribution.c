/**
 * goo_work_distribution.c
 * 
 * Advanced work distribution algorithms for Goo's parallel execution system.
 * Implements work stealing and guided scheduling to optimize parallel workloads.
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>  // For sysconf
#include "parallel/goo_parallel.h"

#define MAX_THREADS 128
#define MIN_CHUNK_SIZE 1
#define DEFAULT_GUIDED_DIVISOR 2

// Thread-local storage for work distribution
typedef struct ThreadWorkState {
    uint64_t next_index;      // Next work item to process
    uint64_t end_index;       // End of current chunk
    int chunk_size;           // Current chunk size
    bool has_work;            // Whether this thread has work
    pthread_mutex_t mutex;    // Mutex for work stealing
    int thread_id;            // Thread ID
} ThreadWorkState;

// Work distribution context
typedef struct WorkDistribution {
    uint64_t start;             // Global start index
    uint64_t end;               // Global end index
    uint64_t step;              // Step size
    uint64_t total_work_items;  // Total number of work items
    uint64_t current_index;     // Current global index
    uint64_t remaining_work;    // Remaining work items
    GooScheduleType schedule;   // Scheduling strategy
    int initial_chunk_size;     // Initial chunk size specified
    int num_threads;            // Number of threads
    pthread_mutex_t mutex;      // Global mutex for work distribution
    ThreadWorkState thread_states[MAX_THREADS]; // Per-thread state
} WorkDistribution;

// Static work distribution context
static WorkDistribution work_dist = {0};

// Initialize the work distribution system
void goo_work_distribution_init(uint64_t start, uint64_t end, uint64_t step, 
                               GooScheduleType schedule, int chunk_size, int num_threads) {
    // Initialize global state
    work_dist.start = start;
    work_dist.end = end;
    work_dist.step = step;
    work_dist.total_work_items = (end - start + step - 1) / step; // Ceiling division
    work_dist.current_index = start;
    work_dist.remaining_work = work_dist.total_work_items;
    work_dist.schedule = schedule;
    work_dist.initial_chunk_size = chunk_size;
    work_dist.num_threads = num_threads > 0 ? num_threads : 1;
    
    pthread_mutex_init(&work_dist.mutex, NULL);
    
    // Initialize per-thread state
    for (int i = 0; i < work_dist.num_threads; i++) {
        ThreadWorkState *state = &work_dist.thread_states[i];
        memset(state, 0, sizeof(ThreadWorkState));
        state->thread_id = i;
        pthread_mutex_init(&state->mutex, NULL);
    }
    
    // For static schedule, calculate chunks upfront
    if (schedule == GOO_SCHEDULE_STATIC) {
        int chunk = chunk_size > 0 ? chunk_size : 
                    (work_dist.total_work_items + work_dist.num_threads - 1) / work_dist.num_threads;
        
        for (int i = 0; i < work_dist.num_threads; i++) {
            ThreadWorkState *state = &work_dist.thread_states[i];
            state->next_index = start + i * chunk * step;
            state->end_index = state->next_index + chunk * step;
            
            // Ensure end doesn't exceed global end
            if (state->end_index > end) {
                state->end_index = end;
            }
            
            state->has_work = (state->next_index < state->end_index);
        }
    }
}

// Clean up the work distribution system
void goo_work_distribution_cleanup(void) {
    pthread_mutex_destroy(&work_dist.mutex);
    
    for (int i = 0; i < work_dist.num_threads; i++) {
        pthread_mutex_destroy(&work_dist.thread_states[i].mutex);
    }
    
    memset(&work_dist, 0, sizeof(WorkDistribution));
}

// Get the next chunk of work for dynamic scheduling
static bool get_next_dynamic_chunk(int thread_id, uint64_t *start_index, uint64_t *end_index) {
    pthread_mutex_lock(&work_dist.mutex);
    
    // Check if work is done
    if (work_dist.current_index >= work_dist.end) {
        pthread_mutex_unlock(&work_dist.mutex);
        return false;
    }
    
    // Calculate chunk size
    int chunk_size = work_dist.initial_chunk_size > 0 ? 
                    work_dist.initial_chunk_size : 1;
    
    // Assign chunk
    *start_index = work_dist.current_index;
    *end_index = *start_index + chunk_size * work_dist.step;
    
    // Ensure end doesn't exceed global end
    if (*end_index > work_dist.end) {
        *end_index = work_dist.end;
    }
    
    // Update global state
    work_dist.current_index = *end_index;
    
    pthread_mutex_unlock(&work_dist.mutex);
    
    // Update thread state
    ThreadWorkState *state = &work_dist.thread_states[thread_id];
    pthread_mutex_lock(&state->mutex);
    state->next_index = *start_index;
    state->end_index = *end_index;
    state->has_work = true;
    pthread_mutex_unlock(&state->mutex);
    
    return true;
}

// Get the next chunk of work for guided scheduling
static bool get_next_guided_chunk(int thread_id, uint64_t *start_index, uint64_t *end_index) {
    pthread_mutex_lock(&work_dist.mutex);
    
    // Check if work is done
    if (work_dist.current_index >= work_dist.end) {
        pthread_mutex_unlock(&work_dist.mutex);
        return false;
    }
    
    // Calculate chunk size - guided decreases chunk size as work decreases
    // Use an adaptive divisor that changes based on remaining work percentage
    uint64_t total_items = work_dist.total_work_items;
    uint64_t remaining = (work_dist.end - work_dist.current_index + work_dist.step - 1) / work_dist.step;
    double remaining_ratio = (double)remaining / total_items;
    
    // Adaptive divisor - increases as work decreases for finer-grained scheduling
    int guided_divisor;
    
    if (remaining_ratio > 0.75) {
        // Early phase: use larger chunks (less overhead)
        guided_divisor = 2;
    } else if (remaining_ratio > 0.5) {
        // Middle phase: medium chunks
        guided_divisor = 3;
    } else if (remaining_ratio > 0.25) {
        // Late middle phase: smaller chunks
        guided_divisor = 4;
    } else {
        // Final phase: smallest chunks (best load balancing)
        guided_divisor = 8;
    }
    
    // Calculate chunk size based on remaining work and adaptive divisor
    int chunk_size = remaining / (work_dist.num_threads * guided_divisor);
    
    // Apply minimum chunk size constraints
    if (chunk_size < MIN_CHUNK_SIZE) {
        chunk_size = MIN_CHUNK_SIZE;
    }
    
    // Apply initial chunk size as maximum, if specified
    if (work_dist.initial_chunk_size > 0 && chunk_size > work_dist.initial_chunk_size) {
        chunk_size = work_dist.initial_chunk_size;
    }
    
    // For very small amounts of remaining work, distribute more aggressively
    if (remaining < work_dist.num_threads * 4) {
        // When little work remains, use smaller chunks to ensure work is distributed
        chunk_size = 1;
    }
    
    // Assign chunk
    *start_index = work_dist.current_index;
    *end_index = *start_index + chunk_size * work_dist.step;
    
    // Ensure end doesn't exceed global end
    if (*end_index > work_dist.end) {
        *end_index = work_dist.end;
    }
    
    // Update global state
    work_dist.current_index = *end_index;
    
    pthread_mutex_unlock(&work_dist.mutex);
    
    // Update thread state
    ThreadWorkState *state = &work_dist.thread_states[thread_id];
    pthread_mutex_lock(&state->mutex);
    state->next_index = *start_index;
    state->end_index = *end_index;
    state->chunk_size = chunk_size;
    state->has_work = true;
    pthread_mutex_unlock(&state->mutex);
    
    return true;
}

// Attempt to steal work from another thread
static bool steal_work_from_others(int thread_id, uint64_t *start_index, uint64_t *end_index) {
    ThreadWorkState *my_state = &work_dist.thread_states[thread_id];
    
    // Try to steal from other threads
    // First attempt to steal from threads with the most work
    uint64_t max_work = 0;
    int best_victim = -1;
    
    // First pass: find the thread with the most remaining work
    for (int i = 0; i < work_dist.num_threads; i++) {
        // Skip myself
        if (i == thread_id) continue;
        
        ThreadWorkState *other = &work_dist.thread_states[i];
        
        // Try to lock the other thread's work
        if (pthread_mutex_trylock(&other->mutex) == 0) {
            // Check if the other thread has work to steal
            if (other->has_work && other->next_index < other->end_index) {
                uint64_t other_remaining = (other->end_index - other->next_index) / work_dist.step;
                
                if (other_remaining > max_work) {
                    max_work = other_remaining;
                    best_victim = i;
                }
            }
            pthread_mutex_unlock(&other->mutex);
        }
    }
    
    // If we found a good victim, steal from them
    if (best_victim >= 0) {
        ThreadWorkState *victim = &work_dist.thread_states[best_victim];
        
        if (pthread_mutex_trylock(&victim->mutex) == 0) {
            // Check again if they still have work (could have changed)
            if (victim->has_work && victim->next_index < victim->end_index) {
                // Calculate how much to steal - more aggressive with larger workloads
                uint64_t other_remaining = (victim->end_index - victim->next_index) / work_dist.step;
                uint64_t steal_amount;
                
                if (other_remaining > 100) {
                    // For larger workloads, steal up to 75%
                    steal_amount = other_remaining * 3 / 4;
                } else if (other_remaining > 10) {
                    // For medium workloads, steal half
                    steal_amount = other_remaining / 2;
                } else {
                    // For small workloads, steal just one item
                    steal_amount = 1;
                }
                
                // Ensure we steal at least one item
                if (steal_amount < 1) {
                    steal_amount = 1;
                }
                
                // Calculate boundaries
                uint64_t steal_boundary = victim->next_index + (steal_amount * work_dist.step);
                *start_index = steal_boundary;
                *end_index = victim->end_index;
                
                // Update the victim's end point
                victim->end_index = steal_boundary;
                
                // Update my state
                pthread_mutex_lock(&my_state->mutex);
                my_state->next_index = *start_index;
                my_state->end_index = *end_index;
                my_state->has_work = true;
                pthread_mutex_unlock(&my_state->mutex);
                
                pthread_mutex_unlock(&victim->mutex);
                return true;
            }
            pthread_mutex_unlock(&victim->mutex);
        }
    }
    
    // Fallback to original algorithm if optimal approach failed
    for (int i = 0; i < work_dist.num_threads; i++) {
        // Skip myself
        if (i == thread_id) continue;
        
        ThreadWorkState *other = &work_dist.thread_states[i];
        
        // Try to lock the other thread's work
        if (pthread_mutex_trylock(&other->mutex) == 0) {
            // Check if the other thread has work to steal
            if (other->has_work && other->next_index < other->end_index) {
                // Calculate how much to steal (half of remaining work)
                uint64_t other_remaining = (other->end_index - other->next_index) / work_dist.step;
                uint64_t steal_amount = other_remaining / 2;
                
                // Ensure we steal at least one item
                if (steal_amount < 1) {
                    steal_amount = 1;
                }
                
                // Calculate boundaries
                uint64_t steal_boundary = other->next_index + (steal_amount * work_dist.step);
                *start_index = steal_boundary;
                *end_index = other->end_index;
                
                // Update the victim's end point
                other->end_index = steal_boundary;
                
                // Update my state
                pthread_mutex_lock(&my_state->mutex);
                my_state->next_index = *start_index;
                my_state->end_index = *end_index;
                my_state->has_work = true;
                pthread_mutex_unlock(&my_state->mutex);
                
                pthread_mutex_unlock(&other->mutex);
                return true;
            }
            
            pthread_mutex_unlock(&other->mutex);
        }
    }
    
    return false;
}

// Get the next item to work on, based on the scheduling strategy
bool goo_work_distribution_next(int thread_id, uint64_t *index) {
    if (thread_id < 0 || thread_id >= work_dist.num_threads || !index) {
        return false;
    }
    
    ThreadWorkState *state = &work_dist.thread_states[thread_id];
    bool has_next = false;
    
    switch (work_dist.schedule) {
        case GOO_SCHEDULE_STATIC: {
            // For static scheduling, just check the pre-assigned chunk
            pthread_mutex_lock(&state->mutex);
            if (state->next_index < state->end_index) {
                *index = state->next_index;
                state->next_index += work_dist.step;
                has_next = true;
            } else {
                state->has_work = false;
            }
            pthread_mutex_unlock(&state->mutex);
            break;
        }
        
        case GOO_SCHEDULE_DYNAMIC: {
            // Check if we have work in our current chunk
            pthread_mutex_lock(&state->mutex);
            if (state->next_index < state->end_index) {
                *index = state->next_index;
                state->next_index += work_dist.step;
                has_next = true;
                pthread_mutex_unlock(&state->mutex);
            } else {
                // Current chunk is done, get a new one
                state->has_work = false;
                pthread_mutex_unlock(&state->mutex);
                
                uint64_t start, end;
                has_next = get_next_dynamic_chunk(thread_id, &start, &end);
                if (has_next) {
                    *index = start;
                    
                    // Update state with the new position
                    pthread_mutex_lock(&state->mutex);
                    state->next_index = start + work_dist.step;
                    pthread_mutex_unlock(&state->mutex);
                }
            }
            break;
        }
        
        case GOO_SCHEDULE_GUIDED: {
            // Check if we have work in our current chunk
            pthread_mutex_lock(&state->mutex);
            if (state->next_index < state->end_index) {
                *index = state->next_index;
                state->next_index += work_dist.step;
                has_next = true;
                pthread_mutex_unlock(&state->mutex);
            } else {
                // Current chunk is done, get a new one
                state->has_work = false;
                pthread_mutex_unlock(&state->mutex);
                
                uint64_t start, end;
                has_next = get_next_guided_chunk(thread_id, &start, &end);
                if (has_next) {
                    *index = start;
                    
                    // Update state with the new position
                    pthread_mutex_lock(&state->mutex);
                    state->next_index = start + work_dist.step;
                    pthread_mutex_unlock(&state->mutex);
                }
            }
            break;
        }
        
        case GOO_SCHEDULE_AUTO:
        default: {
            // Enhanced auto scheduling with adaptive load balancing
            
            // Check if we have work in our current chunk
            pthread_mutex_lock(&state->mutex);
            if (state->next_index < state->end_index) {
                *index = state->next_index;
                state->next_index += work_dist.step;
                has_next = true;
                pthread_mutex_unlock(&state->mutex);
                
                // Periodically check for imbalance (every 16 items)
                if ((*index % 16) == 0) {
                    goo_work_distribution_detect_imbalance(thread_id);
                }
            } else {
                // Current chunk is done
                state->has_work = false;
                pthread_mutex_unlock(&state->mutex);
                
                // First try to get a new chunk (using guided scheduling as base)
                uint64_t start, end;
                has_next = get_next_guided_chunk(thread_id, &start, &end);
                
                if (has_next) {
                    *index = start;
                    
                    // Update state with the new position
                    pthread_mutex_lock(&state->mutex);
                    state->next_index = start + work_dist.step;
                    pthread_mutex_unlock(&state->mutex);
                } else {
                    // No new chunks available, try stealing with advanced strategy
                    has_next = steal_work_from_others(thread_id, &start, &end);
                    if (has_next) {
                        *index = start;
                        
                        // Update state with the new position
                        pthread_mutex_lock(&state->mutex);
                        state->next_index = start + work_dist.step;
                        pthread_mutex_unlock(&state->mutex);
                    } else {
                        // Check for any imbalance and try to address it
                        // This is our last resort before giving up
                        if (goo_work_distribution_detect_imbalance(thread_id)) {
                            // If imbalance detection triggered work stealing, try again
                            has_next = steal_work_from_others(thread_id, &start, &end);
                            if (has_next) {
                                *index = start;
                                
                                // Update state with the new position
                                pthread_mutex_lock(&state->mutex);
                                state->next_index = start + work_dist.step;
                                pthread_mutex_unlock(&state->mutex);
                            }
                        }
                    }
                }
            }
            break;
        }
    }
    
    return has_next;
}

// Estimate the best scheduling strategy based on the workload
GooScheduleType goo_work_distribution_auto_strategy(uint64_t start, uint64_t end, uint64_t step) {
    uint64_t total_work = (end - start + step - 1) / step; // Ceiling division
    int num_threads = sysconf(_SC_NPROCESSORS_ONLN); // Get available CPU cores
    
    // For very small workloads, use static scheduling to minimize overhead
    if (total_work <= num_threads * 2) {
        return GOO_SCHEDULE_STATIC;
    }
    
    // For small workloads, use dynamic scheduling with small chunks
    if (total_work < 100) {
        return GOO_SCHEDULE_DYNAMIC;
    }
    
    // For medium workloads, decide based on number of threads and work size
    if (total_work < 1000) {
        // For fewer threads or smaller workloads, dynamic is usually better
        if (num_threads <= 4) {
            return GOO_SCHEDULE_DYNAMIC;
        } else {
            // With more threads, guided can be better to reduce overhead
            return GOO_SCHEDULE_GUIDED;
        }
    }
    
    // For large workloads, guided scheduling usually performs best
    if (total_work < 100000) {
        return GOO_SCHEDULE_GUIDED;
    }
    
    // For massive workloads, hybrid approach works best
    // We'll implement this via the custom AUTO scheduling in goo_work_distribution_next
    // which combines guided scheduling with work stealing
    return GOO_SCHEDULE_AUTO;
}

// Choose the optimal chunk size based on workload and thread count
int goo_work_distribution_optimal_chunk_size(uint64_t start, uint64_t end, uint64_t step, 
                                          GooScheduleType schedule, int num_threads) {
    uint64_t total_work = (end - start + step - 1) / step; // Ceiling division
    
    switch (schedule) {
        case GOO_SCHEDULE_STATIC:
            // For static scheduling, divide work evenly
            return (total_work + num_threads - 1) / num_threads;
            
        case GOO_SCHEDULE_DYNAMIC: {
            // For dynamic scheduling, use a smarter approach based on workload size
            if (total_work < num_threads * 4) {
                // For very small workloads, use smallest possible chunks
                return 1;
            } else if (total_work < 100) {
                // For small workloads, relatively small chunks
                return (total_work / (num_threads * 8)) > 1 ? (total_work / (num_threads * 8)) : 1;
            } else if (total_work < 1000) {
                // For medium workloads
                return total_work / (num_threads * 6);
            } else if (total_work < 10000) {
                // For large workloads
                return total_work / (num_threads * 4);
            } else {
                // For very large workloads, larger chunks to reduce overhead
                return total_work / (num_threads * 2);
            }
        }
            
        case GOO_SCHEDULE_GUIDED: {
            // For guided scheduling, set the initial chunk size based on workload
            if (total_work < 100) {
                // Small workloads
                return total_work / 4;
            } else if (total_work < 1000) {
                // Medium workloads
                return total_work / 3;
            } else if (total_work < 10000) {
                // Large workloads
                return total_work / 2;
            } else {
                // Very large workloads - can start with larger chunks
                // since guided scheduling will reduce them over time
                return total_work / num_threads; 
            }
        }
            
        case GOO_SCHEDULE_AUTO: {
            // For auto scheduling, base decision on workload size and thread count
            if (total_work < num_threads * 4) {
                // Very small workloads - use 1
                return 1;
            } else if (total_work < 100) {
                // Small workloads - use small chunks
                return 2;
            } else if (total_work < 1000) {
                // Medium workloads - start with medium chunks,
                // work stealing will balance things
                return total_work / (num_threads * 4);
            } else {
                // Large workloads - start with larger chunks to reduce overhead
                return total_work / (num_threads * 2);
            }
        }
            
        default:
            // Default failsafe
            return (total_work / num_threads) > 1 ? (total_work / num_threads) : 1;
    }
}

// Get statistics about the current work distribution
void goo_work_distribution_stats(int *completed_items, int *total_items) {
    if (!completed_items || !total_items) return;
    
    pthread_mutex_lock(&work_dist.mutex);
    
    *total_items = work_dist.total_work_items;
    *completed_items = (work_dist.current_index - work_dist.start) / work_dist.step;
    
    // Ensure completed_items doesn't exceed total_items
    if (*completed_items > *total_items) {
        *completed_items = *total_items;
    }
    
    pthread_mutex_unlock(&work_dist.mutex);
}

// Detect workload imbalance and adjust scheduling strategy if needed
bool goo_work_distribution_detect_imbalance(int thread_id) {
    if (thread_id < 0 || thread_id >= work_dist.num_threads) {
        return false;
    }
    
    // This function should be called periodically by threads
    // We'll check for signs of imbalance
    
    // Acquire global mutex to check overall progress
    pthread_mutex_lock(&work_dist.mutex);
    
    // Calculate how much work is done and how much remains
    uint64_t total_work = work_dist.total_work_items;
    uint64_t completed = (work_dist.current_index - work_dist.start) / work_dist.step;
    
    if (completed >= total_work) {
        // All work is already assigned, nothing to adjust
        pthread_mutex_unlock(&work_dist.mutex);
        return false;
    }
    
    // Check imbalance by counting idle threads vs busy threads
    int idle_threads = 0;
    int busy_threads = 0;
    uint64_t max_remaining_work = 0;
    int thread_with_most_work = -1;
    
    // First scan: get counts of busy/idle threads and find the thread with most work
    for (int i = 0; i < work_dist.num_threads; i++) {
        ThreadWorkState *state = &work_dist.thread_states[i];
        
        if (pthread_mutex_trylock(&state->mutex) == 0) {
            if (state->has_work && state->next_index < state->end_index) {
                busy_threads++;
                
                uint64_t thread_remaining = (state->end_index - state->next_index) / work_dist.step;
                if (thread_remaining > max_remaining_work) {
                    max_remaining_work = thread_remaining;
                    thread_with_most_work = i;
                }
            } else {
                idle_threads++;
            }
            pthread_mutex_unlock(&state->mutex);
        }
    }
    
    // If we have idle threads and there's significant work remaining,
    // adjust the schedule or recommend work stealing
    bool made_adjustment = false;
    
    if (idle_threads > 0 && busy_threads > 0 && thread_with_most_work >= 0) {
        // Significant imbalance detected
        
        // Scenario 1: We're using static scheduling and things are imbalanced
        if (work_dist.schedule == GOO_SCHEDULE_STATIC) {
            // Can't change static scheduling mid-run, but we can flag
            // that the calling thread should try stealing
            pthread_mutex_unlock(&work_dist.mutex);
            
            // Steal from the thread with most work
            uint64_t start, end;
            if (steal_work_from_others(thread_id, &start, &end)) {
                return true;
            }
            return false;
        }
        
        // Scenario 2: We're using dynamic scheduling but chunks too large
        else if (work_dist.schedule == GOO_SCHEDULE_DYNAMIC && 
                work_dist.initial_chunk_size > 1) {
            // Reduce chunk size for better distribution
            work_dist.initial_chunk_size = work_dist.initial_chunk_size / 2;
            if (work_dist.initial_chunk_size < 1) {
                work_dist.initial_chunk_size = 1;
            }
            made_adjustment = true;
        }
        
        // Scenario 3: We're using guided scheduling but the ratio is wrong
        else if (work_dist.schedule == GOO_SCHEDULE_GUIDED) {
            // No need to adjust parameters here, guided already adapts
            made_adjustment = false;
        }
    }
    
    pthread_mutex_unlock(&work_dist.mutex);
    
    // If we're the idle thread, try to steal work regardless of adjustments
    if (idle_threads > 0) {
        ThreadWorkState *my_state = &work_dist.thread_states[thread_id];
        bool am_i_idle = false;
        
        pthread_mutex_lock(&my_state->mutex);
        am_i_idle = !(my_state->has_work && my_state->next_index < my_state->end_index);
        pthread_mutex_unlock(&my_state->mutex);
        
        if (am_i_idle) {
            uint64_t start, end;
            if (steal_work_from_others(thread_id, &start, &end)) {
                return true;
            }
        }
    }
    
    return made_adjustment;
} 
