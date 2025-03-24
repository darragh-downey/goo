const std = @import("std");
const Allocator = std.mem.Allocator;
const Thread = std.Thread;
const Atomic = std.atomic.Atomic;
const Mutex = std.Thread.Mutex;
const Condition = std.Thread.Condition;

// Maximum number of threads in the pool
const MAX_THREADS = 64;

// Global allocator for the parallel module
var gpa = std.heap.GeneralPurposeAllocator(.{}){};
var global_allocator: *Allocator = undefined;

// Thread pool for parallel execution
pub const ThreadPool = struct {
    allocator: *Allocator,
    threads: []Thread,
    queue: std.atomic.Queue(Task),
    shutdown: std.atomic.Bool,
    mutex: Mutex,
    condition: Condition,
    active_tasks: std.atomic.Value(usize),

    const Self = @This();

    // Initialize the thread pool
    pub fn create(num_threads: usize) !*Self {
        const actual_threads = std.math.min(num_threads, MAX_THREADS);
        const pool = try global_allocator.create(Self);
        pool.* = Self{
            .allocator = global_allocator,
            .threads = try global_allocator.alloc(Thread, actual_threads),
            .queue = std.atomic.Queue(Task).init(),
            .shutdown = std.atomic.Bool.init(false),
            .mutex = Mutex{},
            .condition = Condition{},
            .active_tasks = std.atomic.Value(usize).init(0),
        };

        // Start worker threads
        for (0..actual_threads) |i| {
            pool.threads[i] = try Thread.spawn(.{}, workerFn, .{pool});
        }

        return pool;
    }

    // Clean up the thread pool
    pub fn destroy(self: *Self) void {
        // Signal threads to stop
        self.shutdown.store(true, .SeqCst);

        // Wake up all threads
        self.mutex.lock();
        self.condition.broadcast();
        self.mutex.unlock();

        // Wait for threads to complete
        for (self.threads) |thread| {
            thread.join();
        }

        self.allocator.free(self.threads);

        // Clean up remaining tasks in the queue
        while (self.queue.get()) |node| {
            const task = node.data;
            task.destroy();
            self.allocator.destroy(node);
        }

        self.allocator.destroy(self);
    }

    // Submit a task to the thread pool
    pub fn submit(self: *Self, task: *Task) bool {
        if (self.shutdown.load(.SeqCst)) {
            return false;
        }

        // Create a queue node
        const node = self.allocator.create(std.atomic.Queue(Task).Node) catch {
            return false;
        };

        // Initialize the node with the task
        node.* = .{ .data = task.* };

        // Add to active tasks count
        _ = self.active_tasks.fetchAdd(1, .SeqCst);

        // Add to queue and signal a thread
        self.queue.put(node);

        self.mutex.lock();
        self.condition.signal();
        self.mutex.unlock();

        return true;
    }

    // Wait for all tasks to complete
    pub fn waitAll(self: *Self) void {
        // Simple spin-wait until all tasks are completed
        // More sophisticated implementations could use a separate condition variable
        while (self.active_tasks.load(.SeqCst) > 0) {
            std.time.sleep(1 * std.time.ns_per_ms);
        }
    }

    // Worker thread function
    fn workerFn(pool: *Self) void {
        while (true) {
            // Check for shutdown
            if (pool.shutdown.load(.SeqCst)) {
                break;
            }

            // Try to get a task
            if (pool.queue.get()) |node| {
                const task = node.data;

                // Execute the task
                task.execute();

                // Cleanup
                pool.allocator.destroy(node);

                // Decrement active tasks
                _ = pool.active_tasks.fetchSub(1, .SeqCst);
            } else {
                // No tasks, wait for signal
                pool.mutex.lock();
                if (!pool.shutdown.load(.SeqCst)) {
                    pool.condition.wait(&pool.mutex);
                }
                pool.mutex.unlock();
            }
        }
    }
};

// Task queue for the thread pool
const TaskQueue = struct {
    allocator: Allocator,
    mutex: Mutex,
    condition: Condition,
    tasks: std.ArrayList(*Task),

    const Self = @This();

    // Initialize the task queue
    fn init(allocator: Allocator) Self {
        return Self{
            .allocator = allocator,
            .mutex = Mutex{},
            .condition = Condition{},
            .tasks = std.ArrayList(*Task).init(allocator),
        };
    }

    // Clean up the task queue
    fn deinit(self: *Self) void {
        self.tasks.deinit();
    }

    // Push a task to the queue
    fn push(self: *Self, task: *Task) !void {
        self.mutex.lock();
        defer self.mutex.unlock();

        try self.tasks.append(task);

        // Signal that a task is available
        self.condition.signal();
    }

    // Pop a task from the queue
    fn pop(self: *Self) ?*Task {
        self.mutex.lock();
        defer self.mutex.unlock();

        if (self.tasks.items.len == 0) {
            return null;
        }

        return self.tasks.orderedRemove(0);
    }

    // Signal all waiting threads
    fn signal(self: *Self) void {
        self.condition.broadcast();
    }
};

// Base task for execution in the thread pool
pub const Task = struct {
    execute_fn: *const fn (*anyopaque) void,
    data: *anyopaque,

    const Self = @This();

    // Initialize a new task
    pub fn create(execute_fn: *const fn (*anyopaque) void, data: *anyopaque) !*Self {
        const task = try global_allocator.create(Self);
        task.* = Self{
            .execute_fn = execute_fn,
            .data = data,
        };
        return task;
    }

    pub fn destroy(self: *Self) void {
        global_allocator.destroy(self);
    }

    // Execute the task
    pub fn execute(self: *Self) void {
        self.executeImpl();
    }

    fn executeImpl(self: Self) void {
        self.execute_fn(self.data);
    }
};

// Parallel for loop implementation
pub const ParallelFor = struct {
    allocator: Allocator,
    pool: *ThreadPool,
    start: usize,
    end: usize,
    step: usize,
    chunk_size: usize,
    fn_ptr: *const fn (usize, *anyopaque) void,
    data: *anyopaque,
    tasks: std.ArrayList(ForTask),

    const Self = @This();

    // For loop task
    const ForTask = struct {
        task: Task,
        range_start: usize,
        range_end: usize,
        fn_ptr: *const fn (usize, *anyopaque) void,
        data: *anyopaque,

        fn init(
            range_start: usize,
            range_end: usize,
            fn_ptr: *const fn (usize, *anyopaque) void,
            data: *anyopaque,
        ) ForTask {
            const self = ForTask{
                .task = undefined,
                .range_start = range_start,
                .range_end = range_end,
                .fn_ptr = fn_ptr,
                .data = data,
            };

            return self;
        }

        fn executeFn(data: ?*anyopaque) void {
            const self = @as(*ForTask, @ptrCast(@alignCast(data.?)));

            // Execute the user function for each index in the range
            var i = self.range_start;
            while (i < self.range_end) : (i += 1) {
                self.fn_ptr(i, self.data);
            }
        }
    };

    // Initialize a parallel for loop
    pub fn create(
        allocator: Allocator,
        pool: *ThreadPool,
        start: usize,
        end: usize,
        step: usize,
        fn_ptr: *const fn (usize, *anyopaque) void,
        data: *anyopaque,
    ) !*Self {
        const total_items = if (end > start) (end - start) else 0;
        const chunk_size = calculateChunkSize(total_items, pool.threads.len);

        const parallel_for = try global_allocator.create(Self);
        parallel_for.* = Self{
            .allocator = allocator,
            .pool = pool,
            .start = start,
            .end = end,
            .step = step,
            .chunk_size = chunk_size,
            .fn_ptr = fn_ptr,
            .data = data,
            .tasks = std.ArrayList(ForTask).init(allocator),
        };

        return parallel_for;
    }

    // Clean up resources
    pub fn destroy(self: *Self) void {
        self.tasks.deinit();
        global_allocator.destroy(self);
    }

    // Execute the parallel for loop
    pub fn execute(self: *Self) bool {
        if (self.start >= self.end) {
            return true; // Nothing to do
        }

        // Prepare the tasks
        var current_start = self.start;
        const total_items = self.end - self.start;
        var remaining_items = total_items;

        // Clear any existing tasks
        self.tasks.clearRetainingCapacity();

        // Create tasks for each chunk
        while (remaining_items > 0) {
            const items_in_chunk = @min(remaining_items, self.chunk_size);
            const chunk_end = current_start + items_in_chunk;

            // Create a task for this chunk
            try self.tasks.append(ForTask.init(
                current_start,
                chunk_end,
                self.fn_ptr,
                self.data,
            ));

            // Get a pointer to the last task
            const task_ptr = &self.tasks.items[self.tasks.items.len - 1];
            task_ptr.task = Task.create(ForTask.executeFn, task_ptr) catch {
                // Clean up
                self.tasks.deinit();
                return false;
            };

            // Submit the task to the thread pool
            if (!self.pool.submit(task_ptr.task)) {
                // Clean up
                task_ptr.task.destroy();
                return false;
            }

            // Move to the next range
            current_start = chunk_end;
            remaining_items -= items_in_chunk;
        }

        // Wait for all tasks to complete
        self.pool.waitAll();

        return true;
    }

    // Calculate optimal chunk size for a parallel for loop
    fn calculateChunkSize(total_items: usize, num_threads: usize) usize {
        if (num_threads <= 1 or total_items <= 1) {
            return total_items;
        }

        // Aim for each thread to get approximately equal work
        const base_chunk = total_items / num_threads;

        // Ensure at least 1 item per chunk
        return std.math.max(base_chunk, 1);
    }
};

// Parallel reduce operation
pub const ParallelReduce = struct {
    allocator: Allocator,
    pool: *ThreadPool,
    start: usize,
    end: usize,
    chunk_size: usize,
    identity_value: *anyopaque,
    mapper_fn: *const fn (usize, *anyopaque) *anyopaque,
    reducer_fn: *const fn (*anyopaque, *anyopaque) *anyopaque,
    data: *anyopaque,
    tasks: std.ArrayList(ReduceTask),
    results: []?*anyopaque,

    const Self = @This();

    // Reduce task
    const ReduceTask = struct {
        task: Task,
        range_start: usize,
        range_end: usize,
        result_index: usize,
        mapper_fn: *const fn (usize, *anyopaque) *anyopaque,
        reducer_fn: *const fn (*anyopaque, *anyopaque) *anyopaque,
        identity_value: *anyopaque,
        data: *anyopaque,
        results: *[]?*anyopaque,

        fn init(
            range_start: usize,
            range_end: usize,
            result_index: usize,
            mapper_fn: *const fn (usize, *anyopaque) *anyopaque,
            reducer_fn: *const fn (*anyopaque, *anyopaque) *anyopaque,
            identity_value: *anyopaque,
            data: *anyopaque,
            results: *[]?*anyopaque,
        ) ReduceTask {
            const self = ReduceTask{
                .task = undefined,
                .range_start = range_start,
                .range_end = range_end,
                .result_index = result_index,
                .mapper_fn = mapper_fn,
                .reducer_fn = reducer_fn,
                .identity_value = identity_value,
                .data = data,
                .results = results,
            };

            return self;
        }

        fn executeFn(data: ?*anyopaque) void {
            const self = @as(*ReduceTask, @ptrCast(@alignCast(data.?)));

            // Start with the identity value
            var result = self.identity_value;

            // Process each item in the range
            var i = self.range_start;
            while (i < self.range_end) : (i += 1) {
                // Map the current index
                const mapped = self.mapper_fn(i, self.data);

                // Reduce with the accumulator
                result = self.reducer_fn(result, mapped);
            }

            // Store the result
            self.results[self.result_index] = result;
        }
    };

    // Initialize a parallel reduce operation
    pub fn create(
        allocator: Allocator,
        pool: *ThreadPool,
        start: usize,
        end: usize,
        identity_value: *anyopaque,
        mapper_fn: *const fn (usize, *anyopaque) *anyopaque,
        reducer_fn: *const fn (*anyopaque, *anyopaque) *anyopaque,
        data: *anyopaque,
    ) !*Self {
        const total_items = if (end > start) (end - start) else 0;
        const chunk_size = calculateChunkSize(total_items, pool.threads.len);
        var results = try global_allocator.alloc(?*anyopaque, total_items);

        const parallel_reduce = try global_allocator.create(Self);
        parallel_reduce.* = Self{
            .allocator = allocator,
            .pool = pool,
            .start = start,
            .end = end,
            .chunk_size = chunk_size,
            .identity_value = identity_value,
            .mapper_fn = mapper_fn,
            .reducer_fn = reducer_fn,
            .data = data,
            .tasks = std.ArrayList(ReduceTask).init(allocator),
            .results = results,
        };

        return parallel_reduce;
    }

    // Clean up resources
    pub fn destroy(self: *Self) void {
        self.tasks.deinit();
        global_allocator.free(self.results);
        global_allocator.destroy(self);
    }

    // Execute the parallel reduce operation
    pub fn execute(self: *Self, result: **anyopaque) bool {
        if (self.start >= self.end) {
            result.* = self.identity_value; // Nothing to do
            return true;
        }

        // Prepare the tasks
        var current_start = self.start;
        const total_items = self.end - self.start;
        var remaining_items = total_items;

        // Clear any existing tasks
        self.tasks.clearRetainingCapacity();

        // Create tasks for each chunk
        while (remaining_items > 0) {
            const items_in_chunk = @min(remaining_items, self.chunk_size);
            const chunk_end = current_start + items_in_chunk;

            // Create a task for this chunk
            try self.tasks.append(ReduceTask.init(
                current_start,
                chunk_end,
                self.tasks.items.len,
                self.mapper_fn,
                self.reducer_fn,
                self.identity_value,
                self.data,
                &self.results,
            ));

            // Get a pointer to the last task
            const task_ptr = &self.tasks.items[self.tasks.items.len - 1];
            task_ptr.task = Task.create(ReduceTask.executeFn, task_ptr) catch {
                // Clean up
                self.tasks.deinit();
                return false;
            };

            // Submit the task to the thread pool
            if (!self.pool.submit(task_ptr.task)) {
                // Clean up
                task_ptr.task.destroy();
                return false;
            }

            // Move to the next range
            current_start = chunk_end;
            remaining_items -= items_in_chunk;
        }

        // Wait for all tasks to complete
        self.pool.waitAll();

        // Combine the results
        var final_result = self.identity_value;
        for (self.results) |maybe_result| {
            if (maybe_result) |chunk_result| {
                final_result = self.reducer_fn(final_result, chunk_result);
            }
        }

        result.* = final_result;
        return true;
    }

    // Calculate optimal chunk size for a parallel reduce operation
    fn calculateChunkSize(total_items: usize, num_threads: usize) usize {
        if (num_threads <= 1 or total_items <= 1) {
            return total_items;
        }

        // Aim for each thread to get approximately equal work
        const base_chunk = total_items / num_threads;

        // Ensure at least 1 item per chunk
        return std.math.max(base_chunk, 1);
    }
};

// Exported C API functions

export fn memoryInit() bool {
    global_allocator = &gpa.allocator;
    return true;
}

export fn memoryCleanup() void {
    _ = gpa.deinit();
}

export fn threadPoolCreate(num_threads: usize) ?*ThreadPool {
    return ThreadPool.create(num_threads) catch null;
}

export fn threadPoolDestroy(pool: *ThreadPool) void {
    pool.destroy();
}

export fn taskCreate(execute_fn: *const fn (*anyopaque) void, data: *anyopaque) ?*Task {
    return Task.create(execute_fn, data) catch null;
}

export fn taskDestroy(task: *Task) void {
    task.destroy();
}

export fn taskExecute(task: *Task) void {
    task.execute();
}

export fn threadPoolSubmit(pool: *ThreadPool, task: *Task) bool {
    return pool.submit(task);
}

export fn threadPoolWaitAll(pool: *ThreadPool) void {
    pool.waitAll();
}

export fn parallelForCreate(pool: *ThreadPool, start: usize, end: usize, step: usize, fn_ptr: *const fn (usize, *anyopaque) void, data: *anyopaque) ?*ParallelFor {
    return ParallelFor.create(pool, start, end, step, fn_ptr, data) catch null;
}

export fn parallelForDestroy(parallel_for: *ParallelFor) void {
    parallel_for.destroy();
}

export fn parallelForExecute(parallel_for: *ParallelFor) bool {
    return parallel_for.execute();
}

export fn parallelReduceCreate(pool: *ThreadPool, start: usize, end: usize, identity_value: *anyopaque, mapper_fn: *const fn (usize, *anyopaque) *anyopaque, reducer_fn: *const fn (*anyopaque, *anyopaque) *anyopaque, data: *anyopaque) ?*ParallelReduce {
    return ParallelReduce.create(pool, start, end, identity_value, mapper_fn, reducer_fn, data) catch null;
}

export fn parallelReduceDestroy(parallel_reduce: *ParallelReduce) void {
    parallel_reduce.destroy();
}

export fn parallelReduceExecute(parallel_reduce: *ParallelReduce, result: **anyopaque) bool {
    return parallel_reduce.execute(result);
}
