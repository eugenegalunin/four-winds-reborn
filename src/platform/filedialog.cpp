/***************************************************************************
 *   Four Winds Reborn native file selection                              *
 ***************************************************************************/

#include "filedialog.h"

#if defined(_WIN32)
#include <algorithm>
#include <array>

#ifdef ERROR
#undef ERROR
#endif
#include <windows.h>
#include <commdlg.h>
#ifdef ERROR
#undef ERROR
#endif

namespace
{
std::wstring utf8ToWide(const std::string & value)
{
    if(value.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                         value.data(), static_cast<int>(value.size()),
                                         nullptr, 0);
    if(size <= 0) return {};
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                        static_cast<int>(value.size()), result.data(), size);
    return result;
}

std::string wideToUtf8(const std::wstring & value)
{
    if(value.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                         value.data(), static_cast<int>(value.size()),
                                         nullptr, 0, nullptr, nullptr);
    if(size <= 0) return {};
    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
                        static_cast<int>(value.size()), result.data(), size,
                        nullptr, nullptr);
    return result;
}

FileDialog::Result finishDialog(bool selected, const wchar_t* buffer,
                                std::string & path, std::string* error)
{
    if(selected)
    {
        path = wideToUtf8(buffer);
        if(path.empty())
        {
            if(error) *error = "the selected path could not be converted to UTF-8";
            return FileDialog::Result::Failed;
        }
        if(error) error->clear();
        return FileDialog::Result::Selected;
    }

    const DWORD code = CommDlgExtendedError();
    if(code == 0)
    {
        if(error) error->clear();
        return FileDialog::Result::Cancelled;
    }
    if(error) *error = "native file dialog failed with code " + std::to_string(code);
    return FileDialog::Result::Failed;
}
}
#endif

FileDialog::Result FileDialog::openReplay(std::string & path, std::string* error)
{
#if defined(_WIN32)
    std::array<wchar_t, 32768> buffer{};
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = GetActiveWindow();
    dialog.lpstrFilter = L"Four Winds Replay (*.fwr)\0*.fwr\0All Files (*.*)\0*.*\0\0";
    dialog.lpstrFile = buffer.data();
    dialog.nMaxFile = static_cast<DWORD>(buffer.size());
    dialog.lpstrDefExt = L"fwr";
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR |
                   OFN_DONTADDTORECENT;
    return finishDialog(GetOpenFileNameW(&dialog) != FALSE, buffer.data(), path, error);
#else
    (void) path;
    if(error) error->clear();
    return Result::Unavailable;
#endif
}

FileDialog::Result FileDialog::saveReplay(const std::string & defaultName,
                                          std::string & path, std::string* error)
{
#if defined(_WIN32)
    std::array<wchar_t, 32768> buffer{};
    const std::wstring initialName = utf8ToWide(defaultName);
    const std::size_t count = std::min(initialName.size(), buffer.size() - 1);
    std::copy_n(initialName.data(), count, buffer.data());

    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = GetActiveWindow();
    dialog.lpstrFilter = L"Four Winds Replay (*.fwr)\0*.fwr\0All Files (*.*)\0*.*\0\0";
    dialog.lpstrFile = buffer.data();
    dialog.nMaxFile = static_cast<DWORD>(buffer.size());
    dialog.lpstrDefExt = L"fwr";
    dialog.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT |
                   OFN_DONTADDTORECENT;
    return finishDialog(GetSaveFileNameW(&dialog) != FALSE, buffer.data(), path, error);
#else
    (void) defaultName;
    (void) path;
    if(error) error->clear();
    return Result::Unavailable;
#endif
}
