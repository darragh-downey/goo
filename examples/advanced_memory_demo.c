/**
 * Goo Advanced Memory Management Demo
 * 
 * This example demonstrates the Zig implementation of specialized allocators:
 * - Arena Allocator: Fast allocations with bulk freeing
 * - Pool Allocator: Efficient for fixed-size objects
 * - Region Allocator: Contiguous memory regions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>

// Base allocator type
typedef struct GooAllocator GooAllocator;

// Memory initialization
extern int goo_memory_init(void);
extern void goo_memory_cleanup(void);

// Basic allocator functions
extern void* goo_alloc_with_allocator(GooAllocator* allocator, size_t size);
extern void* goo_alloc_zero_with_allocator(GooAllocator* allocator, size_t size);
extern void goo_free_with_allocator(GooAllocator* allocator, void* ptr, size_t size);

// Arena allocator
extern GooAllocator* goo_arena_allocator_create(size_t initial_size);
extern void goo_arena_allocator_reset(GooAllocator* allocator);
extern void goo_arena_allocator_destroy(GooAllocator* allocator);

// Pool allocator
extern GooAllocator* goo_pool_allocator_create(size_t chunk_size, size_t alignment, size_t chunks_per_block);
extern void goo_pool_allocator_destroy(GooAllocator* allocator);

// Region allocator
extern GooAllocator* goo_region_allocator_create(size_t region_size, bool allow_large_allocations);
extern void goo_region_allocator_reset(GooAllocator* allocator);
extern void goo_region_allocator_destroy(GooAllocator* allocator);

// Allocator stats
typedef struct GooAllocStats {
    size_t bytes_allocated;
    size_t bytes_reserved;
    size_t max_bytes_allocated;
    size_t allocation_count;
    size_t total_allocations;
    size_t total_frees;
    size_t failed_allocations;
} GooAllocStats;

extern GooAllocStats goo_get_allocator_stats(GooAllocator* allocator);

// Time measurement helpers
typedef struct {
    clock_t start;
    clock_t end;
} TimingInfo;

static void timing_start(TimingInfo* timing) {
    timing->start = clock();
}

static void timing_end(TimingInfo* timing) {
    timing->end = clock();
}

static double timing_seconds(TimingInfo* timing) {
    return ((double)(timing->end - timing->start)) / CLOCKS_PER_SEC;
}

// =========================== Demo Scenarios ===========================

// --------------------- Arena Allocator Demo ---------------------
// Scenario: Parsing a large text file with temporary allocations

// Simple token structure
typedef struct {
    char* text;
    size_t length;
} Token;

// Simple document structure
typedef struct {
    Token* tokens;
    size_t token_count;
    size_t capacity;
} Document;

// Tokenize text using arena allocator
void tokenize_with_arena(const char* text, size_t length) {
    printf("\n----- Arena Allocator: Text Processing Demo -----\n");
    
    // Create arena allocator
    GooAllocator* arena = goo_arena_allocator_create(64 * 1024); // 64KB arena
    
    // Create document
    Document* doc = (Document*)goo_alloc_with_allocator(arena, sizeof(Document));
    doc->capacity = 1000;
    doc->token_count = 0;
    doc->tokens = (Token*)goo_alloc_with_allocator(arena, doc->capacity * sizeof(Token));
    
    printf("Tokenizing text of %zu bytes...\n", length);
    
    // Simple tokenizer (splits on whitespace)
    size_t start = 0;
    size_t i;
    
    TimingInfo timing;
    timing_start(&timing);
    
    for (i = 0; i < length; i++) {
        // If whitespace, end token
        if (text[i] == ' ' || text[i] == '\n' || text[i] == '\t' || text[i] == '\r') {
            if (i > start) { // Non-empty token
                // Resize token array if needed
                if (doc->token_count >= doc->capacity) {
                    size_t new_capacity = doc->capacity * 2;
                    Token* new_tokens = (Token*)goo_alloc_with_allocator(
                        arena, new_capacity * sizeof(Token));
                    memcpy(new_tokens, doc->tokens, doc->token_count * sizeof(Token));
                    doc->tokens = new_tokens;
                    doc->capacity = new_capacity;
                }
                
                // Add token
                size_t token_len = i - start;
                char* token_text = (char*)goo_alloc_with_allocator(arena, token_len + 1);
                memcpy(token_text, &text[start], token_len);
                token_text[token_len] = '\0';
                
                doc->tokens[doc->token_count].text = token_text;
                doc->tokens[doc->token_count].length = token_len;
                doc->token_count++;
            }
            start = i + 1;
        }
    }
    
    // Handle last token
    if (i > start) {
        size_t token_len = i - start;
        char* token_text = (char*)goo_alloc_with_allocator(arena, token_len + 1);
        memcpy(token_text, &text[start], token_len);
        token_text[token_len] = '\0';
        
        doc->tokens[doc->token_count].text = token_text;
        doc->tokens[doc->token_count].length = token_len;
        doc->token_count++;
    }
    
    timing_end(&timing);
    
    // Show results
    printf("Found %zu tokens in %.6f seconds\n", doc->token_count, timing_seconds(&timing));
    
    // Print first few tokens
    printf("First 10 tokens:\n");
    for (i = 0; i < (doc->token_count < 10 ? doc->token_count : 10); i++) {
        printf("  %zu: '%s'\n", i, doc->tokens[i].text);
    }
    
    // Get allocator stats
    GooAllocStats stats = goo_get_allocator_stats(arena);
    printf("\nArena allocator stats:\n");
    printf("  Bytes allocated: %zu\n", stats.bytes_allocated);
    printf("  Bytes reserved: %zu\n", stats.bytes_reserved);
    printf("  Allocation count: %zu\n", stats.allocation_count);
    
    // Reset and free everything at once
    goo_arena_allocator_reset(arena);
    
    // Stats after reset
    stats = goo_get_allocator_stats(arena);
    printf("\nAfter reset:\n");
    printf("  Bytes allocated: %zu\n", stats.bytes_allocated);
    printf("  Allocation count: %zu\n", stats.allocation_count);
    
    // Cleanup
    goo_arena_allocator_destroy(arena);
    
    printf("\nArena demonstration complete\n");
}

// --------------------- Pool Allocator Demo ---------------------
// Scenario: Object pool for particle system

typedef struct {
    float x, y, z;
    float velocity_x, velocity_y, velocity_z;
    float lifetime;
    float size;
    unsigned int color;
    bool active;
} Particle;

#define MAX_PARTICLES 10000
#define ACTIVE_PARTICLES 1000

void particle_system_with_pool(void) {
    printf("\n----- Pool Allocator: Particle System Demo -----\n");
    
    // Create pool allocator sized for particles
    GooAllocator* pool = goo_pool_allocator_create(sizeof(Particle), 8, 64);
    
    // Particle pointers
    Particle* particles[MAX_PARTICLES];
    size_t active_count = 0;
    
    // Timing
    TimingInfo timing;
    
    // Initialize random seed
    srand(time(NULL));
    
    // Simulation steps
    printf("Running particle system simulation...\n");
    
    // Create initial particles
    printf("Creating %d initial particles...\n", ACTIVE_PARTICLES);
    timing_start(&timing);
    
    for (int i = 0; i < ACTIVE_PARTICLES; i++) {
        particles[i] = (Particle*)goo_alloc_with_allocator(pool, sizeof(Particle));
        
        // Initialize particle
        particles[i]->x = (float)(rand() % 1000) / 10.0f;
        particles[i]->y = (float)(rand() % 1000) / 10.0f;
        particles[i]->z = (float)(rand() % 1000) / 10.0f;
        particles[i]->velocity_x = (float)(rand() % 100 - 50) / 10.0f;
        particles[i]->velocity_y = (float)(rand() % 100) / 10.0f;
        particles[i]->velocity_z = (float)(rand() % 100 - 50) / 10.0f;
        particles[i]->lifetime = (float)(rand() % 1000) / 100.0f;
        particles[i]->size = (float)(rand() % 50) / 10.0f;
        particles[i]->color = rand() % 0xFFFFFF;
        particles[i]->active = true;
        
        active_count++;
    }
    
    timing_end(&timing);
    printf("Created initial particles in %.6f seconds\n", timing_seconds(&timing));
    
    // Run simulation steps
    printf("\nRunning 60 simulation steps (frames)...\n");
    timing_start(&timing);
    
    for (int frame = 0; frame < 60; frame++) {
        // Update existing particles
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (i < active_count && particles[i]->active) {
                // Update position
                particles[i]->x += particles[i]->velocity_x;
                particles[i]->y += particles[i]->velocity_y;
                particles[i]->z += particles[i]->velocity_z;
                
                // Gravity effect
                particles[i]->velocity_y -= 0.1f;
                
                // Update lifetime
                particles[i]->lifetime -= 0.016f; // ~60fps
                
                // Check if particle died
                if (particles[i]->lifetime <= 0 || particles[i]->y < 0) {
                    particles[i]->active = false;
                    
                    // Free the particle
                    goo_free_with_allocator(pool, particles[i], sizeof(Particle));
                    
                    // Move the last active particle to this slot
                    if (i < active_count - 1) {
                        particles[i] = particles[active_count - 1];
                        particles[active_count - 1] = NULL;
                    }
                    
                    active_count--;
                    i--; // Reprocess this slot
                }
            }
        }
        
        // Create new particles to maintain count
        int to_create = ACTIVE_PARTICLES - active_count;
        for (int i = 0; i < to_create && active_count < MAX_PARTICLES; i++) {
            particles[active_count] = (Particle*)goo_alloc_with_allocator(pool, sizeof(Particle));
            
            // Initialize a new particle (emitter at origin)
            particles[active_count]->x = 0.0f;
            particles[active_count]->y = 0.0f;
            particles[active_count]->z = 0.0f;
            particles[active_count]->velocity_x = (float)(rand() % 100 - 50) / 10.0f;
            particles[active_count]->velocity_y = (float)(rand() % 100) / 5.0f;
            particles[active_count]->velocity_z = (float)(rand() % 100 - 50) / 10.0f;
            particles[active_count]->lifetime = (float)(rand() % 1000) / 100.0f;
            particles[active_count]->size = (float)(rand() % 50) / 10.0f;
            particles[active_count]->color = rand() % 0xFFFFFF;
            particles[active_count]->active = true;
            
            active_count++;
        }
        
        // For demonstration, print stats every 10 frames
        if (frame % 10 == 0) {
            printf("Frame %d: %zu active particles\n", frame, active_count);
        }
    }
    
    timing_end(&timing);
    printf("Simulation complete in %.6f seconds (avg %.6f ms per frame)\n", 
           timing_seconds(&timing), timing_seconds(&timing) * 1000.0 / 60.0);
    
    // Print allocator stats
    GooAllocStats stats = goo_get_allocator_stats(pool);
    printf("\nPool allocator stats:\n");
    printf("  Bytes allocated: %zu\n", stats.bytes_allocated);
    printf("  Bytes reserved: %zu\n", stats.bytes_reserved);
    printf("  Active allocations: %zu\n", stats.allocation_count);
    printf("  Total allocations: %zu\n", stats.total_allocations);
    printf("  Total frees: %zu\n", stats.total_frees);
    
    // Clean up remaining particles
    for (size_t i = 0; i < active_count; i++) {
        goo_free_with_allocator(pool, particles[i], sizeof(Particle));
    }
    
    // Destroy the pool
    goo_pool_allocator_destroy(pool);
    
    printf("\nParticle system demonstration complete\n");
}

// --------------------- Region Allocator Demo ---------------------
// Scenario: Image processing with memory regions

typedef struct {
    int width;
    int height;
    unsigned char* data;  // RGB data (3 bytes per pixel)
} Image;

#define IMAGE_WIDTH 1024
#define IMAGE_HEIGHT 1024

static Image* create_image(GooAllocator* allocator, int width, int height) {
    Image* img = (Image*)goo_alloc_with_allocator(allocator, sizeof(Image));
    if (img == NULL) return NULL;
    
    size_t data_size = width * height * 3; // RGB
    img->data = (unsigned char*)goo_alloc_with_allocator(allocator, data_size);
    if (img->data == NULL) {
        goo_free_with_allocator(allocator, img, sizeof(Image));
        return NULL;
    }
    
    img->width = width;
    img->height = height;
    
    return img;
}

static void fill_image_gradient(Image* img) {
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            int index = (y * img->width + x) * 3;
            img->data[index] = (unsigned char)((float)x / img->width * 255); // R
            img->data[index+1] = (unsigned char)((float)y / img->height * 255); // G
            img->data[index+2] = (unsigned char)(255 - (float)x / img->width * 255); // B
        }
    }
}

static Image* blur_image(GooAllocator* allocator, const Image* input) {
    Image* output = create_image(allocator, input->width, input->height);
    if (output == NULL) return NULL;
    
    // Simple box blur with 3x3 kernel
    for (int y = 0; y < input->height; y++) {
        for (int x = 0; x < input->width; x++) {
            int r_sum = 0, g_sum = 0, b_sum = 0;
            int count = 0;
            
            // 3x3 kernel
            for (int ky = -1; ky <= 1; ky++) {
                int py = y + ky;
                if (py < 0 || py >= input->height) continue;
                
                for (int kx = -1; kx <= 1; kx++) {
                    int px = x + kx;
                    if (px < 0 || px >= input->width) continue;
                    
                    int index = (py * input->width + px) * 3;
                    r_sum += input->data[index];
                    g_sum += input->data[index+1];
                    b_sum += input->data[index+2];
                    count++;
                }
            }
            
            // Write averaged pixel
            int out_index = (y * output->width + x) * 3;
            output->data[out_index] = r_sum / count;
            output->data[out_index+1] = g_sum / count;
            output->data[out_index+2] = b_sum / count;
        }
    }
    
    return output;
}

static Image* sharpen_image(GooAllocator* allocator, const Image* input) {
    Image* output = create_image(allocator, input->width, input->height);
    if (output == NULL) return NULL;
    
    // Simple sharpen filter
    // Kernel: [-1, -1, -1; -1, 9, -1; -1, -1, -1]
    for (int y = 0; y < input->height; y++) {
        for (int x = 0; x < input->width; x++) {
            int r_sum = 0, g_sum = 0, b_sum = 0;
            
            // Apply kernel
            for (int ky = -1; ky <= 1; ky++) {
                int py = y + ky;
                if (py < 0 || py >= input->height) continue;
                
                for (int kx = -1; kx <= 1; kx++) {
                    int px = x + kx;
                    if (px < 0 || px >= input->width) continue;
                    
                    int weight = (kx == 0 && ky == 0) ? 9 : -1;
                    int index = (py * input->width + px) * 3;
                    r_sum += input->data[index] * weight;
                    g_sum += input->data[index+1] * weight;
                    b_sum += input->data[index+2] * weight;
                }
            }
            
            // Clamp and write pixel
            int out_index = (y * output->width + x) * 3;
            output->data[out_index] = (r_sum < 0) ? 0 : (r_sum > 255) ? 255 : r_sum;
            output->data[out_index+1] = (g_sum < 0) ? 0 : (g_sum > 255) ? 255 : g_sum;
            output->data[out_index+2] = (b_sum < 0) ? 0 : (b_sum > 255) ? 255 : b_sum;
        }
    }
    
    return output;
}

void image_processing_with_region(void) {
    printf("\n----- Region Allocator: Image Processing Demo -----\n");
    
    // Create a region allocator (large enough for multiple images)
    size_t region_size = IMAGE_WIDTH * IMAGE_HEIGHT * 3 * 4; // space for ~4 images
    GooAllocator* region = goo_region_allocator_create(region_size, true);
    
    TimingInfo timing;
    
    // Create an image
    printf("Creating %dx%d RGB image...\n", IMAGE_WIDTH, IMAGE_HEIGHT);
    timing_start(&timing);
    
    Image* original = create_image(region, IMAGE_WIDTH, IMAGE_HEIGHT);
    if (original == NULL) {
        printf("Failed to create image\n");
        goo_region_allocator_destroy(region);
        return;
    }
    
    // Fill with a gradient
    fill_image_gradient(original);
    
    timing_end(&timing);
    printf("Image created in %.6f seconds\n", timing_seconds(&timing));
    
    // Apply a blur filter
    printf("\nApplying blur filter...\n");
    timing_start(&timing);
    
    Image* blurred = blur_image(region, original);
    if (blurred == NULL) {
        printf("Failed to blur image\n");
        goo_region_allocator_destroy(region);
        return;
    }
    
    timing_end(&timing);
    printf("Blur applied in %.6f seconds\n", timing_seconds(&timing));
    
    // Apply a sharpen filter to the blurred image
    printf("\nApplying sharpen filter...\n");
    timing_start(&timing);
    
    Image* sharpened = sharpen_image(region, blurred);
    if (sharpened == NULL) {
        printf("Failed to sharpen image\n");
        goo_region_allocator_destroy(region);
        return;
    }
    
    timing_end(&timing);
    printf("Sharpen applied in %.6f seconds\n", timing_seconds(&timing));
    
    // Get allocator stats
    GooAllocStats stats = goo_get_allocator_stats(region);
    printf("\nRegion allocator stats after processing:\n");
    printf("  Bytes allocated: %zu\n", stats.bytes_allocated);
    printf("  Bytes reserved: %zu\n", stats.bytes_reserved);
    printf("  Allocation count: %zu\n", stats.allocation_count);
    
    // Calculate expected allocation
    size_t expected_size = (sizeof(Image) + IMAGE_WIDTH * IMAGE_HEIGHT * 3) * 3;
    printf("  Expected allocation size: %zu\n", expected_size);
    
    // Reset the region for the next processing batch
    printf("\nResetting region for next batch...\n");
    goo_region_allocator_reset(region);
    
    // Stats after reset
    stats = goo_get_allocator_stats(region);
    printf("Region allocator stats after reset:\n");
    printf("  Bytes allocated: %zu\n", stats.bytes_allocated);
    printf("  Allocation count: %zu\n", stats.allocation_count);
    
    // Clean up
    goo_region_allocator_destroy(region);
    
    printf("\nImage processing demonstration complete\n");
}

// Helper function to generate sample text
static char* generate_sample_text(size_t size) {
    char* text = (char*)malloc(size + 1);
    if (text == NULL) return NULL;
    
    // Array of sample words
    const char* words[] = {
        "the", "quick", "brown", "fox", "jumps", "over", "lazy", "dog",
        "hello", "world", "computer", "algorithm", "memory", "allocation",
        "performance", "optimization", "software", "engineering", "code",
        "programming", "language", "design", "pattern", "structure", "data"
    };
    const int num_words = sizeof(words) / sizeof(words[0]);
    
    size_t pos = 0;
    while (pos < size) {
        // Select a random word
        const char* word = words[rand() % num_words];
        size_t word_len = strlen(word);
        
        // Ensure we don't overflow the buffer
        if (pos + word_len + 1 > size) {
            break;
        }
        
        // Copy the word
        strcpy(&text[pos], word);
        pos += word_len;
        
        // Add a space or newline
        text[pos++] = (rand() % 20 == 0) ? '\n' : ' ';
    }
    
    text[pos] = '\0';
    return text;
}

int main(void) {
    printf("***** GOO ADVANCED MEMORY MANAGEMENT DEMONSTRATION *****\n");
    
    // Initialize random seed
    srand(time(NULL));
    
    // Initialize the memory system
    if (!goo_memory_init()) {
        printf("FATAL: Failed to initialize memory system!\n");
        return EXIT_FAILURE;
    }
    
    // Generate sample text for the arena demo
    size_t text_size = 100 * 1024; // 100KB of sample text
    char* sample_text = generate_sample_text(text_size);
    if (sample_text == NULL) {
        printf("Failed to generate sample text\n");
        goo_memory_cleanup();
        return EXIT_FAILURE;
    }
    
    // Run the demos
    tokenize_with_arena(sample_text, strlen(sample_text));
    particle_system_with_pool();
    image_processing_with_region();
    
    // Clean up
    free(sample_text);
    goo_memory_cleanup();
    
    printf("\n***** ADVANCED MEMORY MANAGEMENT DEMO COMPLETED SUCCESSFULLY *****\n");
    return EXIT_SUCCESS;
} 