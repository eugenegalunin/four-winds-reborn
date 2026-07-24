/***************************************************************************
 *   Four Winds Reborn native file selection                              *
 ***************************************************************************/

#ifndef _FWR_FILEDIALOG_
#define _FWR_FILEDIALOG_

#include <string>

namespace FileDialog
{
    enum class Result
    {
        Selected,
        Cancelled,
        Unavailable,
        Failed
    };

    Result openReplay(std::string & path, std::string* error = nullptr);
    Result saveReplay(const std::string & defaultName, std::string & path,
                      std::string* error = nullptr);
}

#endif
