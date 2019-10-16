#include <string>

#include "filelist.h"

constexpr unsigned int MAX_PATTERN_LENGTH = 128;

int main(int argc, char* argv[])
{    
    //read and check inputs
    if (argc < 3) 
    {
        log_err( "Usage: " , argv[0] , " Path Pattern" , '\n');
        return 1;
    }

    std::string path(argv[1]), pattern(argv[2]);

    if (pattern.length() > MAX_PATTERN_LENGTH)
    {
        log_err( "Pattern too long " , pattern.length() , " Max length " , MAX_PATTERN_LENGTH << '\n');
        return 1;
    }

    FileList filelist(pattern);

    filelist.init(path);

    filelist.process_files();

    //std::this_thread::sleep_for (std::chrono::seconds(30));

    std::cout << "main  FINISHING !!!!!!!!!!!!!!!!!!! " << '\n';

    return 0;
}