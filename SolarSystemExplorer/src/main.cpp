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
                 "  --quality LEVEL     low (1 sample) or high (up to 4 samples), main scene\n"
                 "  --compare-bloom     Show Bloom off/on with one camera\n"
                 "  --pipeline STAGE    final, scene, bright, or bloom\n"
                 "  --no-asteroids      Hide the instanced asteroid belt\n"
                 "  --compare-planets   Open the real-size planet comparison window\n"
                 "  --team-card         Show the project and team information\n"
                 "  --demo-tour         Run the 60-second guided tour at fixed 30 FPS steps\n"
                 "  --record-frames DIR Save each rendered frame as JPEG (30 FPS timeline)\n"
                 "  --fallback-textures  Exercise procedural missing-texture recovery\n"
                 "  --assets PATH       Resource root containing shaders/ and assets/\n"
                 "  --lab MODE          explore, orbit, eclipse, or workshop\n"
                 "  --kind KIND         rocky or gas (opens Planet Workshop)\n"
                 "  --seed N            Workshop terrain seed (0 .. 999999)\n"
                 "  --render-mode MODE  shaded, wireframe, or normals\n"
                 "  --shading MODEL     blinn-phong, phong, or gouraud\n"
                 "  --orthographic      Start with an orthographic projection\n"
                 "  --low-poly          Draw planets with a coarse 16x8 sphere\n"
                 "  --bezier            Show the comet's Bezier control polygon\n"
                 "  --no-comet          Hide the Bezier comet\n"
                 "  --experiment FILE   Load a saved .sselab experiment\n"
                 "  --eclipse KIND      solar or lunar (opens Eclipse Lab)\n"
                 "  --eclipse-phase N   Alignment phase offset (-180 .. 180 degrees)\n"
                 "  --eclipse-tilt N    Orbit inclination (-30 .. 30 degrees)\n"
                 "  --observer          Use the eclipse observer viewpoint\n"
#ifdef SSE_WITH_TESTS
                 "  --smoke-test        Run the built-in integration scenario\n"
                 "  --lab-smoke-test    Exercise orbit, graphics, persistence, and eclipse labs\n"
                 "  --workshop-smoke-test  Exercise workshop generation, controls, and challenges\n"
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
            else if (flag == "--record-frames")
                options.recordFrames = std::filesystem::u8path(value());
            else if (flag == "--quality") {
                const auto level = value();
                if (level != "low" && level != "high") throw std::invalid_argument("--quality must be low or high");
                options.quality = level == "high" ? 1 : 0;
            } else if (flag == "--pipeline") {
                const auto stage = value();
                const std::vector<std::string> stages{"final", "scene", "bright", "bloom"};
                const auto found = std::find(stages.begin(), stages.end(), stage);
                if (found == stages.end()) throw std::invalid_argument("--pipeline must be final, scene, bright, or bloom");
                options.pipelineStage = static_cast<int>(found - stages.begin());
            }
            else if (flag == "--assets")
                options.root = std::filesystem::u8path(value());
            else if (flag == "--experiment")
                options.experiment = std::filesystem::u8path(value());
            else if (flag == "--lab") {
                const auto mode = value();
                if (mode == "explore")
                    options.lab = sse::LabView::Explore;
                else if (mode == "orbit")
                    options.lab = sse::LabView::Orbit;
                else if (mode == "eclipse")
                    options.lab = sse::LabView::Eclipse;
                else if (mode == "workshop")
                    options.lab = sse::LabView::Workshop;
                else
                    throw std::invalid_argument("--lab must be explore, orbit, eclipse, or workshop");
            } else if (flag == "--kind") {
                const auto kind = value();
                if (kind != "rocky" && kind != "gas")
                    throw std::invalid_argument("--kind must be rocky or gas");
                options.workshopKind = kind == "gas" ? sse::WorkshopKind::Gas : sse::WorkshopKind::Rocky;
                options.lab = sse::LabView::Workshop;
            } else if (flag == "--seed") {
                options.seed = static_cast<uint32_t>(integer(value(), 0, 999999));
            } else if (flag == "--render-mode") {
                const auto mode = value();
                if (mode == "shaded")
                    options.surfaceMode = sse::SurfaceMode::Shaded;
                else if (mode == "wireframe")
                    options.surfaceMode = sse::SurfaceMode::Wireframe;
                else if (mode == "normals")
                    options.surfaceMode = sse::SurfaceMode::Normals;
                else
                    throw std::invalid_argument("--render-mode must be shaded, wireframe, or normals");
            } else if (flag == "--shading") {
                const auto model = value();
                if (model == "blinn-phong")
                    options.shading = sse::ShadingModel::BlinnPhong;
                else if (model == "phong")
                    options.shading = sse::ShadingModel::Phong;
                else if (model == "gouraud")
                    options.shading = sse::ShadingModel::Gouraud;
                else
                    throw std::invalid_argument("--shading must be blinn-phong, phong, or gouraud");
            } else if (flag == "--orthographic")
                options.orthographic = true;
            else if (flag == "--low-poly")
                options.lowPoly = true;
            else if (flag == "--bezier")
                options.bezierControl = true;
            else if (flag == "--no-comet")
                options.noComet = true;
            else if (flag == "--eclipse") {
                const auto kind = value();
                if (kind != "solar" && kind != "lunar")
                    throw std::invalid_argument("--eclipse must be solar or lunar");
                options.lab = sse::LabView::Eclipse;
                options.lunar = kind == "lunar";
            } else if (flag == "--eclipse-phase") {
                const auto phase = number(value());
                if (phase < -180 || phase > 180)
                    throw std::invalid_argument("--eclipse-phase outside [-180, 180]");
                options.eclipsePhase = static_cast<float>(phase);
            } else if (flag == "--eclipse-tilt") {
                const auto tilt = number(value());
                if (tilt < -30 || tilt > 30)
                    throw std::invalid_argument("--eclipse-tilt outside [-30, 30]");
                options.eclipseInclination = static_cast<float>(tilt);
            } else if (flag == "--observer")
                options.observer = true;
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
            else if (flag == "--compare-bloom") options.compareBloom = true;
            else if (flag == "--no-asteroids") options.noAsteroids = true;
            else if (flag == "--compare-planets") options.comparePlanets = true;
            else if (flag == "--team-card") options.teamCard = true;
            else if (flag == "--demo-tour") options.demoTour = true;
#ifdef SSE_WITH_TESTS
            else if (flag == "--smoke-test")
                options.smoke = true;
            else if (flag == "--lab-smoke-test")
                options.labSmoke = true;
            else if (flag == "--workshop-smoke-test")
                options.workshopSmoke = true;
#endif
            else
                throw std::invalid_argument("Unknown option: " + flag + " (use --help)");
        }
        if (options.smoke)
            options.frames = 360;
        if (options.labSmoke)
            options.frames = 240;
        if (options.workshopSmoke)
            options.frames = 240;
        if (int(options.smoke) + int(options.labSmoke) + int(options.workshopSmoke) > 1)
            throw std::invalid_argument("Choose only one smoke scenario");
        if (options.frames == 0 && options.demoTour) options.frames = 1800;
        if (options.frames == 0 && !options.capture.empty())
            options.frames = 180;
        if (options.frames == 0 && options.benchmark)
            options.frames = 600;
        if (options.demoTour && (options.smoke || options.labSmoke || options.workshopSmoke || options.benchmark))
            throw std::invalid_argument("--demo-tour cannot be combined with smoke tests or benchmark");
        if (!options.recordFrames.empty() && options.frames == 0)
            throw std::invalid_argument("--record-frames requires --frames or --demo-tour");
        if (!options.recordFrames.empty() && options.benchmark)
            throw std::invalid_argument("Frame recording would invalidate benchmark timing");
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
