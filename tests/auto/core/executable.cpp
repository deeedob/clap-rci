#include <thread>
#include <string>
#include <iostream>

int main(int argc, char *argv[])
{
    if (argc == 2) {
        std::cout << "Timed" << std::endl;
        const auto sleepTime = std::stoull(argv[1]);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
        return 254;
    }

    if (argc == 3) {
        std::cout << "Endless" << std::endl;
        while (true) {}
    }
    std::cout << "Immediate" << std::endl;
    return 126;
}