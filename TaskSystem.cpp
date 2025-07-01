#include "TaskSystem.h"
#include <condition_variable>
#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <set>
#include <map>
#include <iostream>
using namespace std;


IRunnable::~IRunnable() noexcept {}

TaskSystem::TaskSystem(int num_threads)
    : num_threads(num_threads), active_tasks(0), shutdown(false), last_task_id(0) {

    threads.reserve(num_threads);
    for (int i = 0; i < num_threads; i++) {     
        threads[i] = thread(&TaskSystem::workerThread, this);
    } 
}

TaskSystem::~TaskSystem() {
    shutdown.store(true);
    {
        unique_lock<std::mutex> lock(mutex);
        task_available_cv.notify_all();
    }
    for (int i = 0; i < num_threads; i++)
    {
        if(threads[i].joinable())
            threads[i].join();
    }
}

void TaskSystem::workerThread() {
    
    while (true) {
        ReadyTaskData next_task; 
        bool has_task = false;  

        {
            unique_lock<std::mutex> lock(mutex);
            if (!ready_queue.empty()) {
                next_task = ready_queue.front();
                ready_queue.pop();
                has_task = true;
                active_tasks ++;
            }
        }

        if (has_task) {
            next_task.runnable->runTask(next_task.index, next_task.num_total_tasks); 
            {

                unique_lock<std::mutex> lock(mutex); 
                markTaskFinished(next_task);
            }
        }   
        else {
            if (shutdown.load()) 
                break;
            {
                unique_lock<std::mutex> lock(mutex);

                // if the queue is empty, and the system isn't terminated, stay asleep
                task_available_cv.wait(lock, [this](){
                    return !ready_queue.empty() or shutdown.load(); 
                });
            }
        }
    }
}

void TaskSystem::markTaskFinished(ReadyTaskData finished_task) {

    // notify all tasks depending on finished_task that it has finished
    for (TaskID t : tasks_depending_on[finished_task.id]) {
        waiting_list[t].num_deps --;

        // if finished_task is the last dependency of batch t, 
        // push all tasks of batch t into ready queue
        if (waiting_list[t].num_deps == 0) {
            WaitingTaskBatchData newly_ready_batch = waiting_list[t];
            waiting_list.erase(t);
            for (int i = 0; i < newly_ready_batch.num_total_tasks; i++) {
                ready_queue.push({newly_ready_batch.id, newly_ready_batch.runnable, i, newly_ready_batch.num_total_tasks});
            }
        }
    }

    is_task_done[finished_task.id] = true;   
    active_tasks --;
    if (active_tasks == 0 and ready_queue.empty()) {
        completion_cv.notify_all();
    }
    task_available_cv.notify_one();
}


TaskID TaskSystem::run(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {

    int this_task_id;

    {
        unique_lock<std::mutex> lock(mutex);

        this_task_id = ++last_task_id;

        int num_deps = 0;

        // appending new task to depndency graph 
        for (TaskID t : deps) {
            if (!is_task_done[t]) {
                tasks_depending_on[t].push_back(this_task_id);
                num_deps ++;
            }
        }

        // push each task from the batch to the ready queue
        // if has no dependencies or all deps are executed 
        if (num_deps == 0) { 
            for (int i = 0; i < num_total_tasks; i++) {
                ready_queue.push({this_task_id, runnable, i, num_total_tasks});
            }
        }
        // put the whole batch on a waiting list if it has dependencies
        else { 
            waiting_list[this_task_id] = {this_task_id, num_deps, runnable, num_total_tasks};
        }

        task_available_cv.notify_all();
    }

    return this_task_id;
}

void TaskSystem::sync() {
    {
        unique_lock<std::mutex> lock(mutex);
        completion_cv.wait(lock, [this]() {
            return active_tasks == 0 && ready_queue.empty();
        });
    }    
    return;
}
