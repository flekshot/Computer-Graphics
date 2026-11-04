#include "Application.h"
#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <string>
#include <vector>
namespace {
double number(const std::string& text) {
    size_t used = 0;
    double value = std::stod(text, &used);
    if (used != text.size() || !std::isfinite(value))
        throw std::invalid_argument("Invalid numeric argument: " + text);
    return value;
}
int integer(const std::string& text, int lo, int hi) {
    double n = number(text);
    if (n < lo || n > hi || std::floor(n) != n)
        throw std::invalid_argument("Integer outside supported range: " + text);
    return static_cast<int>(n);
}
void help() {
    std::cout << "Solar System Explorer\n"
                 "Usage: SolarSystemExplorer [options]\n"
                 "  --planet NAME       mercury, venus, earth, mars, jupiter, saturn, uranus, neptune\n"
                 "  --paused            Start with simulation paused\n"
                 "  --days N            Initial simulated days (0 .. 1e10)\n"
                 "  --speed N           Time multiplier (0.05 .. 100); 1x is 6 days/second\n"
                 "  --width N --height N  Window size (minimum 1000 x 640)\n"
                 "  --frames N          Exit after N frames; use deterministic 1/60s steps\n"
                 "  --capture FILE.png  Save final framebuffer (defaults to 180 frames)\n"
                 "  --no-ui --no-bloom   Disable UI or bloom\n"
                 "  --hidden            Create a hidden window for automated checks\n"
                 "  --benchmark         Disable VSync, default 600 frames\n"
                 "  --fallback-textures  Exercise procedural missing-texture recovery\n"
                 "  --assets PATH       Resource root containing shaders/ and assets/\n"
#ifdef SSE_WITH_TESTS
                 "  --smoke-test        Run the built-in integration scenario\n"
#endif
                 "  --help              Show this help\n";
}
} // namespace
int explorerMain(int argc, char** argv) {
    try {
        sse::Options options;
        for (int i = 1; i < argc; ++i) {
            const std::string flag = argv[i];
            auto value = [&]() -> std::string {
                if (i + 1 >= argc)
                    throw std::invalid_argument("Missing value for " + flag);
                return argv[++i];
            };
            if (flag == "--help" || flag == "-h") {
                help();
                return 0;
            } else if (flag == "--frames")
                options.frames = integer(value(), 1, 1000000);
            else if (flag == "--width")
                options.width = integer(value(), 1000, 7680);
            else if (flag == "--height")
                options.height = integer(value(), 640, 4320);
            else if (flag == "--capture")
                options.capture = std::filesystem::u8path(value());
            else if (flag == "--assets")
                options.root = std::filesystem::u8path(value());
            else if (flag == "--days") {
                options.days = number(value());
                if (options.days < 0 || options.days > 1e10)
                    throw std::invalid_argument("--days outside [0, 1e10]");
            } else if (flag == "--speed") {
                auto speed = number(value());
                if (speed < 0.05 || speed > 100)
                    throw std::invalid_argument("--speed outside [0.05, 100]");
                options.speed = static_cast<float>(speed);
            } else if (flag == "--planet") {
                const auto name = value();
                const std::vector<std::string> names = {"mercury", "venus",  "earth",  "mars",
                                                        "jupiter", "saturn", "uranus", "neptune"};
                auto it = std::find(names.begin(), names.end(), name);
                if (it == names.end())
                    throw std::invalid_argument("Unknown planet: " + name);
                options.planet = static_cast<int>(it - names.begin());
            } else if (flag == "--paused")
                options.paused = true;
            else if (flag == "--no-ui")
                options.noUI = true;
            else if (flag == "--no-bloom")
                options.noBloom = true;
            else if (flag == "--hidden")
                options.hidden = true;
            else if (flag == "--fallback-textures")
                options.fallback = true;
            else if (flag == "--benchmark")
                options.benchmark = true;
#ifdef SSE_WITH_TESTS
            else if (flag == "--smoke-test")
                options.smoke = true;
#endif
            else
                throw std::invalid_argument("Unknown option: " + flag + " (use --help)");
        }
        if (options.smoke)
            options.frames = 360;
        if (options.frames == 0 && !options.capture.empty())
            options.frames = 180;
        if (options.frames == 0 && options.benchmark)
            options.frames = 600;
        if (options.root.empty())
            options.root = sse::assetRoot(argv[0]);
        sse::Application app(std::move(options));
        return app.run();
    } catch (const std::exception& error) {
        std::cerr << "Solar System Explorer: " << error.what() << '\n';
        return 1;
    }
}
#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
    // MSVC's narrow argv cannot represent all Windows paths. Normalize to UTF-8.
    std::vector<std::string> encoded;
    std::vector<char*> arguments;
    encoded.reserve(argc);
    for (int i = 0; i < argc; ++i)
        encoded.push_back(std::filesystem::path(argv[i]).u8string());
    for (auto& argument : encoded)
        arguments.push_back(argument.data());
    return explorerMain(argc, arguments.data());
}
#else
int main(int argc, char** argv) {
    return explorerMain(argc, argv);
}
#endif
