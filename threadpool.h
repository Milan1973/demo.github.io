
#include <queue>

//#include <thread>
//#include <condition_variable>
#include "mingw.thread.h"
#include "mingw.condition_variable.h"

class ThreadPool
{
private:

    std::mutex _queue_mutex;

    std::queue<std::function<void()>> _tasks;

    std::vector<std::thread> _threads;

    std::condition_variable _condition;

    bool _stop;


    std::atomic<int> njobs_pending;
    std::mutex main_mutex;
    std::condition_variable main_condition;

    static unsigned int get_number_of_threads()
    {
        unsigned int n = std::thread::hardware_concurrency();

        if (n == 0)
        {
        #ifdef WIN32
            SYSTEM_INFO sysinfo;
            GetSystemInfo(&sysinfo);
            n = sysinfo.dwNumberOfProcessors;
        #endif
        }

        return (n > 1 ? --n : 1);
    }

public:

    void waitUntilCompleted() 
    {
    std::unique_lock<std::mutex> lock(main_mutex);
    main_condition.wait(lock);
    }

    void worker_function()
    {
        //ItemQueue item;
        std::function<void()> task;
        while(true)
        {
            {   // acquire lock
                std::unique_lock<std::mutex> 
                    lock(_queue_mutex);
                
                // look for a work item
                while(!_stop && _tasks.empty())
                { // if there are none wait for notification
                    _condition.wait(lock);
                }
    
                if(_stop) // exit if the pool is stopped
                    return;
    
                // get the task from the queue
                task = _tasks.front();
                _tasks.pop();
    
            }   // release lock
    
            task();

            if ( --njobs_pending == 0 ) {
            main_condition.notify_one();
            }
        }
    }

public:
    ThreadPool() : _stop(false) {};

    // the destructor joins all threads
    ~ThreadPool()
    {
        // stop all threads
        _stop = true;
        _condition.notify_all();
     
        // join them
        for(auto i = 0; i<_threads.size(); i++)
            _threads[i].join();

        _threads.empty();
    }

    bool init()
    {
        unsigned thread_count = get_number_of_threads();

        for(auto i = 0; i < thread_count; i++)
        {  
           _threads.push_back(std::thread(worker_function, this));
        }

        return true;
    }

 	template<class F, class... Args>
	void add_task(F&& f, Args&&... args)
	{

        { // acquire lock
            std::unique_lock<std::mutex> lock(_queue_mutex);
            
            //std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
            //std::function<void()> wrapped = [func](){ (func)(); };
            //_tasks.push(wrapped);

            auto func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);

            // add the task
            _tasks.push( [func](){ (func)(); });

            njobs_pending++;
         } // release lock

        // wake up one thread
        _condition.notify_one();
    }

    unsigned int thread_count()
    {
        return _threads.size();
    }

};