#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

namespace {

bool waitForFile(const std::filesystem::path &path)
{
    for (int attempt = 0; attempt < 100; ++attempt) {
        if (std::filesystem::exists(path)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

} // namespace

int main(int argc, char *argv[])
{
    std::string arguments;
    for (int index = 1; index < argc; ++index) {
        arguments += argv[index];
        arguments += '\n';
    }

    const char *coordinationValue = std::getenv("AMTS_FAKE_ADB_COORD_DIR");
    const std::filesystem::path coordination = coordinationValue == nullptr
        ? std::filesystem::path()
        : std::filesystem::path(coordinationValue);

    if (arguments.find("__AMTS_PROPERTIES__") != std::string::npos) {
        const std::filesystem::path own = coordination / "snapshot.ready";
        const std::filesystem::path peer = coordination / "apps.ready";
        std::ofstream(own.string()).put('1');
        if (!waitForFile(peer)) {
            return 2;
        }
        std::cout << "__AMTS_PROPERTIES__\n"
                     "[ro.product.model]: [Test Phone]\n"
                     "[ro.product.manufacturer]: [AMTS]\n"
                     "[ro.build.version.release]: [16]\n"
                     "[ro.build.version.sdk]: [36]\n"
                     "[ro.product.cpu.abi]: [x86_64]\n"
                     "__AMTS_SIZE__\nPhysical size: 1080x2400\n"
                     "__AMTS_DENSITY__\nPhysical density: 420\n"
                     "__AMTS_BATTERY__\nlevel: 80\nhealth: 2\n"
                     "__AMTS_MEMORY__\nMemTotal: 8000000 kB\nMemAvailable: 4000000 kB\n"
                     "__AMTS_UPTIME__\n123.5 10.0\n";
        return 0;
    }

    if (arguments.find("pm list packages") != std::string::npos) {
        const std::filesystem::path own = coordination / "apps.ready";
        const std::filesystem::path peer = coordination / "snapshot.ready";
        std::ofstream(own.string()).put('1');
        if (!waitForFile(peer)) {
            return 3;
        }
        std::cout << "package:/data/app/one.apk=com.example.one\n"
                     "package:/data/app/two.apk=com.example.two\n";
        return 0;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::cout << "action complete\n";
    return 0;
}
