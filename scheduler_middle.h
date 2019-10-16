
class SchedulerMiddleFile :  public Scheduler
{
private:
    uint64_t _final_size;
    std::atomic_ullong _searched_size;

    std::mutex _mtx;
    std::condition_variable _cv;

public:

    SchedulerMiddleFile(uint64_t length, const std::string &path, const std::string &pattern, ThreadPool &pool) : 
    _final_size(0), _searched_size(0), Scheduler(length, path, pattern, pool) {};

    SchedulerMiddleFile() = delete;

    // search buffer
    void search_buffer_synced(const Buffer &buffer, const uint64_t start_pos, 
    const uint64_t end_pos, const uint64_t thread_searched, const uint64_t offset)
    {
        search_buffer(buffer, start_pos, end_pos, offset);

        uint64_t size_before = _searched_size.fetch_add(thread_searched);
        
        log( "search_buffer_synced ending size " , thread_searched , " all " ,  (size_before + thread_searched) , " final size " , _final_size , '\n');

        if ( (size_before + thread_searched) >= _final_size)
        {
            log("search_buffer_synced NOTIFY " , '\n');
            _cv.notify_one();
        }
    }

    unsigned int get_task_count(const uint64_t total_size)
    {
        //TODO tasku by melo byt minimalne tolik co procesoru
        unsigned int task_count = total_size/MAX_ONE_THREAD_BUFFER_SIZE;
        if (total_size%MAX_ONE_THREAD_BUFFER_SIZE)
            task_count++;

        return task_count;
    }
            
    void schedule_search(const Buffer &buffer, const uint64_t offset, const uint64_t total_size)
    {
        log(  "schedule_search path" , _path , " offset  " , offset , '\n');

        unsigned int task_count = get_task_count(total_size);

        uint64_t begin_pos(0), end_pos(MAX_ONE_THREAD_BUFFER_SIZE + SUFFIX_LENGTH);
        _searched_size.store(0);
        _final_size = total_size;
        uint64_t thread_searched(MAX_ONE_THREAD_BUFFER_SIZE);
        for (auto i=0; i<task_count; 
            i++, begin_pos += MAX_ONE_THREAD_BUFFER_SIZE, end_pos += MAX_ONE_THREAD_BUFFER_SIZE)
        {
            if (i == 1)
                begin_pos -= (_pattern.length() + PREFIX_LENGTH);
            if (i == (task_count - 1))
            {
                end_pos = (total_size - (_pattern.length() + SUFFIX_LENGTH));
                thread_searched = total_size - (begin_pos + PREFIX_LENGTH + _pattern.length());
            }
            //search_buffer_synced(buffer, begin_pos, end_pos, offset);
            //C++20 when_all + using future
            _pool.add_task(std::bind(&SchedulerMiddleFile::search_buffer_synced, this, buffer, 
                            begin_pos, end_pos, thread_searched, offset));
        }

        log( "schedule_search WAITING" , '\n');

        std::unique_lock<std::mutex> lck(_mtx);
        _cv.wait(lck);

        log( "schedule_search FINISHING" , '\n');
    }

    virtual void search_whole_buffer(const Buffer &buffer) override
    {
        schedule_search(buffer,  0, _length);
    }

    virtual void process() override
    {   
        process_file();
    }
};