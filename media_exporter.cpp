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

void writeAudio(std::string filePath, std::string title, std::string artist)
{
    std::ofstream file(filePath);
    if (file.is_open())
    {
        file << "Playing: " << title << " - " << artist << "\n";
        file.close();
        std::cout << "Playing: " << title << " - " << artist << "\n";
    }
}

void monitorAudios(std::string filePath)
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
                    writeAudio(filePath, title, artist);
                    previous = current;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
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

    init_apartment();

    SetConsoleOutputCP(CP_UTF8);

    try
    {
        monitorAudios(filePath);
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