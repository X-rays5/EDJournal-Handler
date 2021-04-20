//
// Created by X-ray on 4/19/2021.
//
#pragma once
#include <iostream>
#include <filesystem>
#include <thread>
#include <Windows.h>
#include <tlhelp32.h>
#include <functional>
#include <fstream>
#include <rapidjson/document.h>


namespace EDJournalLogger {
    class Logger {
    private:
        using EventHandler_t = std::function<void(std::string, std::string)>;
    public:
        explicit Logger(int checkforneweventinterval) {
            timebetweenchecks = checkforneweventinterval;
        }

        void SetEventHandler(EventHandler_t handler) {
            eventhandler_ = std::move(handler);
        }

        void StartListening() {
            if (eventhandler_ == nullptr) {
                std::cout << "Event handler was not set" << "\n";
                return;
            }

            // get path to journals
            std::string journal_directory (std::getenv("USERPROFILE"));
            journal_directory.append(R"(\Saved Games\Frontier Developments\Elite Dangerous)");
            std::cout << journal_directory << "\n";

            // checking if the game is running
            while (!GameRunning()) {
                std::cout << "Game not running\n";
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(10s);
            }

            std::vector<std::string> journal_names;
            if (std::filesystem::is_directory(journal_directory)) {
                std::filesystem::directory_iterator dirIt{ journal_directory };
                for (auto&& dirEntry : dirIt)
                {
                    const auto& path = dirEntry.path();
                    if (path.has_filename())
                    {
                        if (path.extension().string() == ".log") {
                            if (path.filename().string().find("Journal.") != std::string::npos) {
                                journal_names.emplace_back(path.filename().string()); // last entry here is the most recent journal file
                            }
                        }
                    }
                }
            }

            std::string journal = journal_directory + "\\" + journal_names.back();
            journal_names.clear(); // no longer needed so cleaning up

            ListenToEvents(std::move(journal));
        };

    private:

        int timebetweenchecks = 10000; // time between event updates in ms

        EventHandler_t eventhandler_ = nullptr; // default nullptr so there can be a check if the eventhandler was set

        void ListenToEvents(std::string path) {
            listen:

            auto events = CheckForNewEvent(path);

            if (!events.empty()) {
                for (auto&& event : events) {
                    rapidjson::Document json;

                    if (!json.Parse(event.c_str()).HasParseError()) {
                        if (json["event"].IsString()) {
                            if (json["event"].GetString() == "Shutdown") {
                                return; // returning on game close
                            }
                            std::invoke(eventhandler_, json["event"].GetString(), std::move(event));
                        }
                    } else {
                        std::cout << "Failed to parse json for event: " << event << "\n";
                        std::cout << "Rapidjson error: " << json.GetParseError() << std::endl;
                    }
                }

                events.clear();
            }

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(std::chrono::milliseconds(timebetweenchecks));

            goto listen;
        }

        int lasteventline_ = 0;
        std::vector<std::string> CheckForNewEvent(std::string& path) {
            std::ifstream reader(path, std::ios::binary);

            if (!reader.is_open()) {
                return {};
            }

            std::vector<std::string> events;
            std::string buffer;
            int curline = 0;
            while (std::getline(reader, buffer)) {
                curline++;

                // if there is a new event put it in the vector so it can be returned
                if (curline > lasteventline_) {
                   events.emplace_back(buffer);
                }
            }
            reader.close();

            lasteventline_ = curline;

            return events;
        }

        inline bool GameRunning() {
            HANDLE hProcessSnap;
            PROCESSENTRY32 pe32;

            // Take a snapshot of all processes in the system.
            hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
            if( hProcessSnap == INVALID_HANDLE_VALUE )
            {
                return false;
            }

            // Set the size of the structure before using it.
            pe32.dwSize = sizeof( PROCESSENTRY32 );

            // Now walk the snapshot of processes, and
            // display information about each process in turn
            do
            {
                std::string pName = pe32.szExeFile;
                if (pName.find("EliteDangerous") != std::string::npos)
                    return true;
            } while( Process32Next( hProcessSnap, &pe32 ) );

            return false;
        }
    };
}
