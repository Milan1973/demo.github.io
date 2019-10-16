

class SchedulerLargeFile :  public SchedulerMiddleFile
{

private:
    uint64_t calculate_alloc_size() { return MAX_ONE_FILE_BUFFER_SIZE + _pattern.length() + PREFIX_LENGTH; };

public:

    SchedulerLargeFile(uint64_t length, const std::string &path, const std::string &pattern, ThreadPool &pool) : 
    SchedulerMiddleFile(length, path, pattern, pool) {};

    SchedulerLargeFile() = delete;

    virtual void process() override
    {
        log( "process_file_with_cache path " , _path , " length  " , _length , " pattern  " , _pattern ,  '\n');

        Buffer bytes;

        if (!open_file())
            return;

        uint64_t offset(0) ;
        uint64_t read_cnt = _length/MAX_ONE_FILE_BUFFER_SIZE;
        uint64_t last_read = _length%MAX_ONE_FILE_BUFFER_SIZE;

        uint64_t overlay(_pattern.length() + PREFIX_LENGTH);

        bytes.resize(calculate_alloc_size());

        for (auto i=0; i<=read_cnt; i++)
        {
            uint64_t read_size = (i==read_cnt ? last_read : MAX_ONE_FILE_BUFFER_SIZE);
            uint64_t buf_begin_pos =  i==0 ? 0 : overlay;
            if (!read_file(bytes, read_size, buf_begin_pos))
                return;

            schedule_search(bytes, offset, read_size);

            offset += (read_size - overlay);

            if (i < read_cnt)
            {//overlay
                if (overlay < read_size)
                    move(bytes.end() - buf_begin_pos, bytes.end(), bytes.begin());
            }
        }

        close_file();

        log( "process_file_with_cache  END path" , _path , '\n');
    }

    virtual uint64_t allocated_size() override { return calculate_alloc_size(); };
};
