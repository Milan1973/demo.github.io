#pragma once

#include <sstream>

constexpr  unsigned int MAX_BUFFERS_SIZE = 4000000;

constexpr  unsigned int MAX_ONE_THREAD_BUFFER_SIZE = 40000;

constexpr  unsigned int MAX_ONE_FILE_BUFFER_SIZE = 100000;

constexpr  uint64_t PREFIX_LENGTH = 3;

constexpr  uint64_t SUFFIX_LENGTH = 3;

using Buffer = std::vector<char>;

//Output from threads not interleaved
template<typename ...Args>
void log(Args && ...args)
{
    std::stringstream stream;
    (stream << ... << args);
    std::cout << stream.str();
}

template<typename ...Args>
void log_err(Args && ...args)
{
    std::stringstream stream;
    (stream << ... << args);
    std::cerr << stream.str();
}

class Scheduler
{
protected:

        const uint64_t _length;
        const std::string & _path; 
        const std::string & _pattern;
        ThreadPool &_pool;
        std::ifstream _ifs;

public:
        Scheduler(const uint64_t length, const std::string &path, const std::string &pattern, ThreadPool &pool) :
            _length(length), _path(path), _pattern(pattern), _pool(pool){};

        Scheduler() = delete;

        virtual void process() = 0;

        virtual uint64_t allocated_size( ) { return _length; };

protected :

    static void replace_with_escape(const char c, std::string &str, const char r)
    {
        size_t pos = 0;
        std::string repl = "\\";
        repl += r;

        while ( (pos = str.find(c, pos)) != str.npos)
        {
            str.replace(pos, 1, repl);
            pos += 2;
        }
    }

    //nebo zkusit prepsat operator <<
    static void remove_commands(std::string &str)
    {
        replace_with_escape('\t', str, 't');
        replace_with_escape('\n', str, 'n');
    }

    //Output: Print the output to stdout lines in format “<file>(<position>): <prefix>…<suffix>”.
    void output(const Buffer &buffer, Buffer::const_iterator &it, const uint64_t offset) const
    {
        uint64_t pattern_pos = (it - buffer.begin());
        uint64_t suffix_pos = pattern_pos + _pattern.length();
        std::string prefix( it - std::min(pattern_pos, PREFIX_LENGTH), it);
        std::string suffix( it + _pattern.length(), it + _pattern.length() + std::min(buffer.size() - suffix_pos, SUFFIX_LENGTH));

        remove_commands(prefix), remove_commands(suffix);

        log("<" , _path , ">(<" , (pattern_pos + offset) , ">): <" , prefix , ">...<" , suffix , ">" , '\n'); 
    }

    // search buffer
    void search_buffer(const Buffer &buffer, const uint64_t start_pos, const uint64_t end_pos, const uint64_t offset) const
    {
        log(  "search_buffer path" , _path , " start_pos " ,  start_pos ,  " end_pos " , end_pos , " offset "  , offset , '\n');

        auto it = buffer.cbegin() + start_pos;
        while ( (it = std::search(it, buffer.cbegin() + end_pos, _pattern.cbegin(), _pattern.cend())) != (buffer.cbegin() + end_pos))
        {
            output(buffer, it, offset);
            it++;
        }
    }

    virtual void search_whole_buffer(const Buffer &buffer) = 0;

    bool open_file()
    {
        _ifs.open(_path, std::ifstream::in);

        //checking failbit
        if (!_ifs)
        {
            log_err("Error opening " , '\n');
            return false;
        }

        return true;
    }

    bool read_file(Buffer &bytes, uint64_t size, uint64_t buf_begin)
    {
        log( "read_file size " , size , " buf_begin  " , buf_begin ,  '\n');

        _ifs.read(bytes.data() + buf_begin, size - buf_begin);

        if (_ifs.bad() || (_ifs.fail() && !_ifs.eof()) )
        {
            log_err( "Error reading bad fail eof " , _ifs.bad() , _ifs.fail() , _ifs.eof() , '\n');
            close_file();
            return false;
        }

        return true;
    }

    void close_file()
    {
        _ifs.close();
    }

    void process_file()
    {
        log("process_file path ", _path , " length  ",  _length, " pattern  ", _pattern,  '\n');

        if (!open_file())
            return;

        Buffer buffer;
        buffer.resize(_length);

        if (!read_file(buffer, _length, 0))
            return;

        close_file();

        search_whole_buffer(buffer);

        log("process_whole_file  END path" ,_path , '\n');
    }
};