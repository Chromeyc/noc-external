#pragma once
#include <string>
#include <windows.h>
#include <wininet.h>

#pragma comment(lib, "wininet.lib")

class OffsetUpdater
{
public:
    static bool DownloadOffsets(const std::string& url, std::string& content);
    static bool ParseAndUpdateOffsets(const std::string& content, const std::string& outputPath);
    static bool UpdateOffsets(const std::string& url = "https://imtheo.lol/Offsets/Offsets.hpp");
};

