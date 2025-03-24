/**
 * work_distribution_demo.c
 * 
 * Demonstrates the performance benefits of different work distribution strategies
 * in Goo's parallel processing system.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "parallel/goo_parallel.h"

// Number of work items
#define WORK_ITEMS 10000

// Global array to track completion time of each work item
double* completion_times;

// Random work function with varying workloads
void do_work(uint64_t index, void* context) {
    // Simulate varying workloads
    // Items at the beginning take longer than items at the end
    int workload = 10000 + (WORK_ITEMS - index) * 1000;
    
    // Busy wait to simulate CPU-bound work
    for (int i = 0; i < workload; i++) {
        // Prevent optimization
        if (i % 10000 == 0) {
            usleep(1);
        }
    }
    
    // Record completion time
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    completion_times[index] = ts.tv_sec + (ts.tv_nsec / 1.0e9);
}

// Calculate statistics from completion times
void calculate_statistics(double start_time, double* completion_times, int count, 
                         double* total_time, double* average_time, double* max_time) {
    *total_time = 0;
    *max_time = 0;
    
    for (int i = 0; i < count; i++) {
        double item_time = completion_times[i] - start_time;
        if (item_time > *max_time) {
            *max_time = item_time;
        }
    }
    
    *total_time = *max_time;  // Total time is the time when the last item finished
    *average_time = *total_time / 2;  // Approximation of average completion time
}

// Print visual representation of work distribution
void print_distribution_visualization(double start_time, double* completion_times, int count, const char* strategy) {
    printf("\nWork Distribution Visualization for %s:\n", strategy);
    printf("Each '.' represents a completed work item. Time flows from left to right.\n");
    
    // Find max completion time
    double max_time = 0;
    for (int i = 0; i < count; i++) {
        double item_time = completion_times[i] - start_time;
        if (item_time > max_time) {
            max_time = item_time;
        }
    }
    
    // Create time buckets (60 columns)
    int buckets[60] = {0};
    const int BUCKETS = 60;
    
    for (int i = 0; i < count; i++) {
        double item_time = completion_times[i] - start_time;
        int bucket = (int)((item_time / max_time) * (BUCKETS - 1));
        buckets[bucket]++;
    }
    
    // Print visualization
    printf("\n");
    int max_bucket = 0;
    for (int i = 0; i < BUCKETS; i++) {
        if (buckets[i] > max_bucket) max_bucket = buckets[i];
    }
    
    const int MAX_HEIGHT = 20;
    for (int row = MAX_HEIGHT; row > 0; row--) {
        printf("|");
        for (int i = 0; i < BUCKETS; i++) {
            int height = (buckets[i] * MAX_HEIGHT) / max_bucket;
            printf("%c", (height >= row) ? '#' : ' ');
        }
        printf("|\n");
    }
    
    printf("+");
    for (int i = 0; i < BUCKETS; i++) {
        printf("-");
    }
    printf("+\n");
    
    printf(" 0%");
    for (int i = 0; i < BUCKETS-6; i++) {
        printf(" ");
    }
    printf("100%%\n");
    
    printf("Time: 0.0s");
    for (int i = 0; i < BUCKETS-14; i++) {
        printf(" ");
    }
    printf("%.2fs\n", max_time);
}

// Run benchmark with given scheduling strategy
void run_benchmark(GooScheduleType schedule, const char* strategy_name) {
    // Initialize completion times array
    completion_times = (double*)malloc(WORK_ITEMS * sizeof(double));
    
    // Record start time
    struct timespec ts_start;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);
    double start_time = ts_start.tv_sec + (ts_start.tv_nsec / 1.0e9);
    
    // Run parallel computation
    goo_parallel_for(0, WORK_ITEMS, 1, do_work, NULL, schedule, 0, 0);
    
    // Calculate statistics
    double total_time, average_time, max_time;
    calculate_statistics(start_time, completion_times, WORK_ITEMS, 
                        &total_time, &average_time, &max_time);
    
    // Print results
    printf("\n=== %s Scheduling ===\n", strategy_name);
    printf("Total time: %.4f seconds\n", total_time);
    printf("Max completion time: %.4f seconds\n", max_time);
    
    // Print visualization
    print_distribution_visualization(start_time, completion_times, WORK_ITEMS, strategy_name);
    
    // Free memory
    free(completion_times);
}

int main() {
    printf("Goo Work Distribution Demo\n");
    printf("==========================\n");
    printf("This demo shows the performance characteristics of different\n");
    printf("work distribution strategies for imbalanced workloads.\n");
    printf("The workload decreases linearly from start to end.\n");
    
    // Initialize parallel subsystem
    goo_parallel_init(4);  // Use 4 threads
    
    // Run benchmarks for different scheduling strategies
    run_benchmark(GOO_SCHEDULE_STATIC, "Static");
    run_benchmark(GOO_SCHEDULE_DYNAMIC, "Dynamic");
    run_benchmark(GOO_SCHEDULE_GUIDED, "Guided");
    run_benchmark(GOO_SCHEDULE_AUTO, "Auto");
    
    // Clean up
    goo_parallel_cleanup();
    
    return 0;
} 