#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <string>
#include <shlobj.h>

using namespace winrt;
using namespace Windows::Media::Control;

std::string formatTemplate(const std::string& templateStr, const std::unordered_map<std::string, std::string>& values) {
    std::string result = templateStr;
    for (const auto& [key, value] : values) {
        std::string placeholder = "{" + key + "}";
        size_t pos = result.find(placeholder);
        while (pos != std::string::npos) {
            result.replace(pos, placeholder.length(), value);
            pos = result.find(placeholder, pos + value.length());
        }
    }
    return result;
}

std::string readTemplateFromConfig(const std::string& configFilePath) {
    if (GetFileAttributesA(configFilePath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        std::cout << "File does not exist!" << std::endl;
        throw std::runtime_error("File does not exist");
    }

    std::ifstream configFile(configFilePath);
    if (configFile.is_open()) {
        std::string templateStr;
        std::getline(configFile, templateStr);
        configFile.close();
        return templateStr;
    }
    else {
        DWORD error = GetLastError();
        std::cout << "System error code: " << error << std::endl;
        throw std::runtime_error("Cannot open config file");
    }
}

std::string loadTemplate(const std::string& fileName)
{
    std::string templateStr = "Playing: {title} - {artist}";

    try {
        templateStr = readTemplateFromConfig(fileName);
    }
    catch (const std::exception& ex) {
        std::cout << "Could not read config file, using default template. Error: " << ex.what() << "\n";
    }
    
    return templateStr;
}

std::string desktopPath()
{
    char desktopPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_DESKTOP, NULL, 0, desktopPath))) {
        return std::string(desktopPath);
    }
    else {
        throw std::runtime_error("Cannot extract desktop path");
    }
}

void writeAudio(const std::string& filePath, const std::string& output)
{
    std::ofstream file(filePath);
    if (file.is_open())
    {
        file << output << "\n";
        file.close();
        std::cout << output << "\n";
    }
}

void monitorAudios(const std::string& filePath, const std::string& templateStr)
{
    auto manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
    std::string previous = "";

    while (true)
    {
        auto session = manager.GetCurrentSession();

        if (session)
        {
            auto info = session.TryGetMediaPropertiesAsync().get();

            if (info)
            {
                std::string title = winrt::to_string(info.Title());
                std::string artist = winrt::to_string(info.Artist());
                std::string current = title + artist;

                if (current != previous) {
                    std::unordered_map<std::string, std::string> values = {
                            {"title", title},
                            {"artist", artist}
                    };
                    std::string output = formatTemplate(templateStr, values);
                    writeAudio(filePath, output);
                    previous = current;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

std::string getExePath() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    return std::string(buffer).substr(0, pos);
}

int main()
{
    std::cout << "Press CTRL+C to exit program" << std::endl << std::endl;

    std::string filePath = desktopPath() + "\\now_playing.txt";
    std::string inputPath;

    std::cout << "Enter the path to the output file or left it blank to use default location " << filePath << ": ";
    std::getline(std::cin, inputPath);

    if (!inputPath.empty()) {
        filePath = inputPath;
    }
    std::string templateStr = loadTemplate(getExePath() + "\\template.txt");

    init_apartment();

    SetConsoleOutputCP(CP_UTF8);

    try
    {
        monitorAudios(filePath, templateStr);
    }
    catch (const winrt::hresult_error& ex)
    {
        std::cout << "Error: " << winrt::to_string(ex.message()) << "\n";
    }
    catch (const std::exception& ex)
    {
        std::cout << "Standard error: " << ex.what() << "\n";
    }

    return 0;
}
