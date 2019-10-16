
#include "threadpool.h"
#include "dirreader.h"
#include "scheduler.h"
#include "scheduler_small.h"
#include "scheduler_middle.h"
#include "scheduler_large.h"

class FileList
{
private:

    const std::string &_pattern;
    DirectoryReader _dir_reader;
    ThreadPool _pool;
    std::vector<std::unique_ptr<Scheduler>> schedulers;
 
    void process_file(std::string &path, uint64_t size)
    {
        //TODO vsichni dohromady nemuzou zahltit pamet.
        if (size < MAX_ONE_THREAD_BUFFER_SIZE)
        {
        //opening, reading and search will be done in one thread
            schedulers.emplace_back(std::make_unique<SchedulerSmallFile>(size, path, _pattern, _pool))->process();
        }
        else
        {
            if (size < MAX_ONE_FILE_BUFFER_SIZE)
            {
                schedulers.emplace_back(std::make_unique<SchedulerMiddleFile>(size, path, _pattern, _pool))->process();
            }
            else
            {
                schedulers.emplace_back(std::make_unique<SchedulerLargeFile>(size, path, _pattern, _pool))->process();
            }
        }
    }


public:
    FileList(const std::string & pattern) :
    _pattern(pattern) {};

    FileList() = delete;

    bool init(std::string &path)
    {
        if (!_dir_reader.init(path, _pattern.length()))
            return false;

        _pool.init();

        return true;
    }

    void process_files()
    {
        std::vector<FileAttribs>  files = _dir_reader.get_filelist();

        if (files.empty())
        {
            log_err( "No relevant files found" , '\n');
            return;
        }

        for (auto file : files)
            process_file(file.path, file.size);

         _pool.waitUntilCompleted();
    }
};