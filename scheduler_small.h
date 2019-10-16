
class SchedulerSmallFile :  public Scheduler
{
public:

    SchedulerSmallFile(uint64_t length, const std::string &path, const std::string &pattern, ThreadPool &pool) : 
    Scheduler(length, path, pattern, pool) {};

    SchedulerSmallFile() = delete;

    //do whole file in a thread
    virtual void process() override
    {   
        _pool.add_task(std::bind(&SchedulerSmallFile::process_file, this));
    }

private :

    virtual void search_whole_buffer(const Buffer &buffer) override
    {
        search_buffer(buffer, 0, _length, 0);
    }

};