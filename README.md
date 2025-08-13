# batchrunner-cpp

batchrunner is a thread pool-based task execution library with support for dependency management. It supports bulk task launches where multiple instances of the same task can run concurrently, and provides a mechanism to specify dependencies between different task batches.

## Features

- Configurable number of worker threads
- Launch multiple instances of the same task as a batch in parallel
- Specify dependencies between task batches
- Wait for all tasks to complete with `sync()`


## Quick Start

### Basic Usage

```cpp
#include "TaskSystem.h"
#include <iostream>
#include <vector>

class MyTask : public IRunnable {
public:
    MyTask(int size) : data(size) {}
    
    void runTask(int task_id, int num_total_tasks) override {
        // Process a portion of the data based on task_id
        int chunk_size = data.size() / num_total_tasks;
        int start = task_id * chunk_size;
        int end = start + chunk_size;
        
        for (int i = start; i < end; i++) {
            data[i] = i * 2; // Example computation
        }
    }
    
    std::vector<int> data;
};

int main() {
    // Create task system with 4 worker threads
    TaskSystem taskSystem(4);
    
    // Create a task
    MyTask* task = new MyTask(1000);
    
    // Run the task with 4 parallel instances
    std::vector<TaskID> noDeps;
    TaskID taskId = taskSystem.run(task, 4, noDeps);
    
    // Wait for completion
    taskSystem.sync();
    
    // Use results...
    delete task;
    return 0;
}
```

### Task Dependencies

```cpp
TaskSystem taskSystem(8);

// First task batch
MyTask* task1 = new MyTask(100);
std::vector<TaskID> noDeps;
TaskID batch1 = taskSystem.run(task1, 4, noDeps);

// Second task batch depends on first
MyTask* task2 = new MyTask(200);
TaskID batch2 = taskSystem.run(task2, 4, {batch1});

// Wait for all tasks to complete
taskSystem.sync();
```

## API Reference

### TaskSystem Class

#### Constructor
```cpp
TaskSystem(int num_threads)
```
Creates a task system with the specified number of worker threads.

#### Methods

##### run()
```cpp
TaskID run(IRunnable* runnable, int num_total_tasks, const std::vector<TaskID>& deps)
```
Launches a bulk task execution.

**Parameters:**
- `runnable`: Pointer to the task object implementing `IRunnable`
- `num_total_tasks`: Number of parallel instances to run
- `deps`: Vector of TaskIDs that must complete before this batch starts

**Returns:** TaskID that can be used as a dependency for future task batches

##### sync()
```cpp
void sync()
```
Blocks until all previously launched tasks are complete.

### IRunnable Interface

All tasks must inherit from `IRunnable` and implement:

```cpp
virtual void runTask(int task_id, int num_total_tasks) = 0
```

**Parameters:**
- `task_id`: Current task instance ID (0 to num_total_tasks-1)
- `num_total_tasks`: Total number of parallel instances in this batch


## Example: Parallel Array Processing

The included example demonstrates computing squares of numbers in parallel:

```cpp
class SquaresCalculator : public IRunnable {
public:
    SquaresCalculator(int arr_size) {
        nums.resize(arr_size);
    }
    
    void runTask(int task_id, int num_total_tasks) override {
        int step = nums.size() / num_total_tasks;
        int start = task_id * step;
        int end = start + step;
        
        for (int i = start; i < end; i++) {
            nums[i] = i * i;
        }
    }
    
    std::vector<int> nums;
};
```

## Requirements

- C++11 or later
- pthread support
- Compiler with thread support (GCC, Clang, MSVC)

## Credits
I didn't start this project from scratch, but I built it on my solutions to Stanford CS149's assignments. available here https://gfxcourses.stanford.edu/cs149/fall23/.

