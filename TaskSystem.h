#include <condition_variable>
#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <set>
#include <map>

using namespace std;

typedef int TaskID;

class IRunnable {
    public:
        virtual ~IRunnable() noexcept;

        /*
          Executes an instance of the task as part of a bulk task launch.
          
           - task_id: the current task identifier. This value will be
                between 0 and num_total_tasks-1.
              
           - num_total_tasks: the total number of tasks in the bulk
                task launch.
         */
        virtual void runTask(int task_id, int num_total_tasks) = 0;
};

struct ReadyTaskData {
    TaskID id;
    IRunnable* runnable;
    int index;
    int num_total_tasks;
};

struct WaitingTaskBatchData {
    TaskID id;
    int num_deps;
    IRunnable* runnable;
    int num_total_tasks;
};


class TaskSystem {
    public:
        
        // num_threads: the maximum number of threads that the task system can use. 
        TaskSystem(int num_threads);
        ~TaskSystem();

        /*
          Executes an asynchronous bulk task launch of
          num_total_tasks, but with a dependency on prior launched
          tasks.

          The task runtime must complete execution of the tasks
          associated with all bulk task launches referenced in the
          array `deps` before beginning execution of *any* task in
          this bulk task launch.

          The caller must invoke sync() to guarantee completion of the
          tasks in this bulk task launch.
 
          Returns an identifer that can be used in subsequent calls to
          runAsyncWithDeps() to specify a dependency of some future
          bulk task launch on this bulk task launch.
         */
        TaskID run(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);

        /*
          Blocks until all tasks created as a result of **any prior**
          run() calls are complete.
         */
        void sync();

    private:
        int num_threads;
        vector<thread> threads;
        
        // maps each task to tasks depending on it
        std::map<TaskID, vector<TaskID>> tasks_depending_on;
        
        queue<ReadyTaskData> ready_queue;
        
        std::map<TaskID, WaitingTaskBatchData> waiting_list; 
        
        std::map<TaskID, bool> is_task_done;
        
        // num of currently executing tasks 
        std::atomic<int> active_tasks;    
        
        // This flag indicates that the destructor of TaskSystem has been called.
        // This signals all thread loops to break.
        std::atomic<bool> shutdown;      
        
        std::mutex mutex;
        std::condition_variable completion_cv;
        std::condition_variable task_available_cv;
        std::condition_variable wait_for_deps_cv;

        int last_task_id; 


        void markTaskFinished(ReadyTaskData task);
        void workerThread();
};