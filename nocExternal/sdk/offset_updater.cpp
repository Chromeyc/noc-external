#include "offset_updater.h"
#include <fstream>
#include <windows.h>

bool OffsetUpdater::DownloadOffsets(const std::string& url, std::string& content)
{
    HINTERNET hInternet = InternetOpenA("Mozilla/5.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return false;

    HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect)
    {
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[4096];
    DWORD bytesRead;
    content.clear();

    while (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        content += buffer;
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    return !content.empty();
}

bool OffsetUpdater::ParseAndUpdateOffsets(const std::string& content, const std::string& outputPath)
{
    std::ofstream file(outputPath);
    if (!file.is_open()) return false;

    file << content;
    file.close();

    return true;
}

bool OffsetUpdater::UpdateOffsets(const std::string& url)
{
    std::string content;
    if (!DownloadOffsets(url, content))
        return false;

    
    char modulePath[MAX_PATH];
    if (GetModuleFileNameA(GetModuleHandle(NULL), modulePath, MAX_PATH) == 0)
        return false;

    std::string exePath = modulePath;
    size_t lastSlash = exePath.find_last_of("\\/");
    if (lastSlash == std::string::npos)
        return false;

    std::string projectRoot = exePath.substr(0, lastSlash);
    
   
    if (projectRoot.find("x64") != std::string::npos)
    {
        size_t x64Pos = projectRoot.find("x64");
        projectRoot = projectRoot.substr(0, x64Pos);
    }
    
   
    if (!projectRoot.empty() && (projectRoot.back() == '\\' || projectRoot.back() == '/'))
        projectRoot.pop_back();

    std::string path1 = projectRoot + "\\sdk\\offsets.h";
    std::string path2 = projectRoot + "\\nocExternal\\sdk\\offsets.h";
    
    std::string outputPath;
    if (GetFileAttributesA(path1.c_str()) != INVALID_FILE_ATTRIBUTES)
    {
        outputPath = path1;
    }
    else if (GetFileAttributesA(path2.c_str()) != INVALID_FILE_ATTRIBUTES)
    {
        outputPath = path2;
    }
    else
    {
        std::string parentPath = projectRoot + "\\..\\nocExternal\\sdk\\offsets.h";
        if (GetFileAttributesA(parentPath.c_str()) != INVALID_FILE_ATTRIBUTES)
        {
             outputPath = parentPath;
        }
        else
        {
             outputPath = path2;
        }
    }
    
    char resolvedPath[MAX_PATH];
    if (GetFullPathNameA(outputPath.c_str(), MAX_PATH, resolvedPath, NULL) != 0)
    {
        outputPath = resolvedPath;
    }
    
    return ParseAndUpdateOffsets(content, outputPath);
}

