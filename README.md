# EDJournal-Handler
This library is meant to be a simple way to interact with the elite dangerous journals.
This library can be used asynchronous.

<h2>example on how to use</h2>

```c++
#include "journal_logger.hpp"

int main() {
     // there are multiple ways to create a instance of the library

    // init with default settings 10 seconds between event checks and non async
    EDJournalLogger::Logger journal;

    // init with custom time between new event checks in ms and non async
    EDJournalLogger::Logger journal(5000);

    // init async with standard time between new event checks
    EDJournalLogger::Logger journal(true);

    // init with custom time between new event checks and async
    EDJournalLogger::Logger journal(5000, true);

    // this will be triggered on a new event
    journal.SetEventHandler([](std::string event, std::string event_info) {
       std::cout << event << "\n" << event_info << "\n";
    });

    // This starts the library and will trigger the event handler on a new event
    journal.StartListening();

    // if we get to here the game has closed
    std::cout << "Game closed shutting down\n";
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // You can do journal.StartListening() again or shutdown
    // if you do journal.StartListening() again the library will wait for the game to launch
    // and will then start logging again

    return 0;
}

```
