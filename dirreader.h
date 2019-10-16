// namespace std have been included for this problem.
#include <vector>

#include <fstream> 

//POSIX for traversing directories
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

//POSIX for traversing directories
//#include <boost/filesystem.hpp>
//                "-IC:D:\\boost_1_70_0\\include\\boost-1_70",
//                "-I D:\\boost_1_70_0\\include\\boost-1_70",
//                "-lstdc++fs",
#include <iostream>

constexpr  char PATH_SEPARATOR = '/'; 

struct FileAttribs
{
    std::string path;
    uint64_t size;
};

class DirectoryReader
{
private:

    std::vector<FileAttribs>  _files;
    uint64_t _min_file_size;

    static uint64_t get_file_length(std::string path)
    {//C++17 there is a file_size function in filesystem
        std::ifstream ifs (path, std::ifstream::in);

        ifs.seekg (0, ifs.end);
        uint64_t length = ifs.tellg();
        ifs.seekg (0, ifs.beg);

        ifs.close();
        
        return length;
    }

    bool read_directory(std::string path)
    {
        std::cout << "read_directory path " << path << std::endl;

        DIR* dir = opendir(path.c_str());

        struct dirent * dp;

        while ((dp = readdir(dir)) != nullptr)
        {
            const std::string entry(dp->d_name);
            if (entry == "." || entry == "..") 
            {   // Skip the dot entries.
                continue;
            }

            if (!add_file(path + PATH_SEPARATOR + entry))
            {
                closedir(dir);
                std::cout << "read_directory FAILED path " << path << std::endl;
                return false;
            }
        }

        closedir(dir);

        std::cout << "read_directory END path " << path << std::endl;

        return true;
    }

    bool add_file(std::string &&path)
    {
        std::cout << "add_file path " << path << std::endl;

        struct stat st;
        if (stat(path.c_str(), &st) != 0)
        {
            //errno Errors: ENOENT means the file named by filename doesn't exist.
            //TODO check errno
            std::cerr << path <<  " probably doesn't exist " << std::endl;
            std::cout << "add_file FAILED 1 path " << path << std::endl;
            return false;
        }

    //using MinGW-W64-builds-4.3.5 including gcc-8.1.0 which seems to have a problem with c++17 std::filesystem
    //boost not supporting MinGW windows
    //so do it with POSIX 

        if ( S_ISREG(st.st_mode) && (st.st_size >= _min_file_size) )
            _files.push_back(FileAttribs{path, get_file_length(path)});
        else if (S_ISDIR(st.st_mode))
                return read_directory(path);
    //for simplicity omit all other types..

    std::cout << "add_file path END " << path << std::endl;

    return true;
    }

public:
    DirectoryReader() : _min_file_size(0) {};
    
    bool init(std::string &path, uint64_t min_file_size)
    {
        _min_file_size = min_file_size;

        if (!add_file(std::move(path)))
            return false;

        return true;
    }

    std::vector<FileAttribs> &get_filelist()
    {
        return _files;
    }
};