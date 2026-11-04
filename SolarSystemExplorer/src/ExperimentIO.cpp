#include "ExperimentIO.h"
#include <atomic>
#include <charconv>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <utility>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace sse {
namespace {
constexpr size_t maximumFileBytes = 65536;

void require(bool condition, const std::string& message) {
    if (!condition)
        throw std::runtime_error(message);
}
template <typename T>
bool inRange(T value, T lower, T upper) {
    return std::isfinite(value) && value >= lower && value <= upper;
}
void validateSettings(const SolarSystem& system, const RenderSettings& render, const LabState& lab, int selected) {
    require(inRange(system.days(), 0.0, 1.0e10), "Simulation day must be between 0 and 10000000000.");
    require(inRange(system.speed(), 0.05f, 100.0f), "Simulation speed must be between 0.05 and 100.");
    require(selected >= -1 && selected < 8, "Selected planet must be -1 (overview) or 0 through 7.");
    require(inRange(render.exposure, 0.2f, 3.0f), "Exposure must be between 0.2 and 3.");
    require(render.surfaceMode == SurfaceMode::Shaded || render.surfaceMode == SurfaceMode::Wireframe ||
                render.surfaceMode == SurfaceMode::Normals, "Unknown surface visualization mode.");
    require(inRange(render.ambient, 0.0f, 3.0f) && inRange(render.diffuse, 0.0f, 3.0f) &&
                inRange(render.specular, 0.0f, 3.0f), "Light multipliers must be between 0 and 3.");
    require(lab.view == LabView::Explore || lab.view == LabView::Orbit || lab.view == LabView::Eclipse,
            "Unknown laboratory view.");
    require(inRange(lab.eclipse.phaseDegrees, -3600.0f, 3600.0f), "Eclipse phase must be between -3600 and 3600 degrees.");
    require(inRange(lab.eclipse.inclinationDegrees, -30.0f, 30.0f), "Eclipse inclination must be between -30 and 30 degrees.");
    require(inRange(lab.eclipse.angularSpeed, 0.0f, 90.0f), "Eclipse animation speed must be between 0 and 90 degrees/second.");
}

// Ordered, labelled records make omitted/duplicated fields fail instead of silently inheriting defaults.
class Reader {
  public:
    explicit Reader(const std::string& contents) : input_(contents) { input_.imbue(std::locale::classic()); }
    void expect(const char* expected) {
        require(token(expected) == expected, std::string("Expected experiment record '") + expected + "'.");
    }
    template <typename T>
    T number(const char* field) {
        const auto text = token(field);
        T value{};
        const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
        require(result.ec == std::errc() && result.ptr == text.data() + text.size(),
                std::string("Invalid numeric value for ") + field + ".");
        return value;
    }
    bool boolean(const char* field) {
        const auto value = token(field);
        require(value == "0" || value == "1", std::string("Expected 0 or 1 for ") + field + ".");
        return value == "1";
    }
    void finish() {
        std::string extra;
        require(!(input_ >> extra), "Unexpected data after the end of the experiment.");
    }
  private:
    std::string token(const char* field) {
        std::string value;
        require(static_cast<bool>(input_ >> value), std::string("Experiment is truncated at ") + field + ".");
        return value;
    }
    std::istringstream input_;
};

struct TemporaryFile {
    std::filesystem::path path;
    ~TemporaryFile() {
        if (!path.empty()) {
            std::error_code ignored;
            std::filesystem::remove(path, ignored);
        }
    }
};

void writeAtomic(const std::filesystem::path& destination, const std::string& contents) {
    require(!destination.empty() && !destination.filename().empty(), "Choose an experiment file name.");
    TemporaryFile temporary;
    static std::atomic<unsigned long long> sequence{0};
#ifdef _WIN32
    HANDLE handle = INVALID_HANDLE_VALUE;
    for (int attempt = 0; attempt < 32; ++attempt) {
        auto candidate = destination;
        candidate += ".tmp-" + std::to_string(GetCurrentProcessId()) + "-" +
                     std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" +
                     std::to_string(sequence.fetch_add(1));
        handle = CreateFileW(candidate.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                             FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle != INVALID_HANDLE_VALUE) {
            temporary.path = std::move(candidate);
            break;
        }
        const auto reason = GetLastError();
        if (reason != ERROR_FILE_EXISTS && reason != ERROR_ALREADY_EXISTS)
            throw std::system_error(static_cast<int>(reason), std::system_category(), "Cannot create temporary experiment file");
    }
    require(handle != INVALID_HANDLE_VALUE, "Cannot allocate a unique temporary experiment file.");
    DWORD written = 0;
    bool success = WriteFile(handle, contents.data(), static_cast<DWORD>(contents.size()), &written, nullptr) != FALSE;
    DWORD failure = success ? ERROR_SUCCESS : GetLastError();
    if (success && written != contents.size()) {
        success = false;
        failure = ERROR_WRITE_FAULT;
    }
    if (success && !FlushFileBuffers(handle)) {
        success = false;
        failure = GetLastError();
    }
    if (!CloseHandle(handle) && success) {
        success = false;
        failure = GetLastError();
    }
    if (!success)
        throw std::system_error(static_cast<int>(failure), std::system_category(), "Cannot write complete experiment file");
    // Same-directory replacement preserves the previous file if writing or replacement fails.
    if (!MoveFileExW(temporary.path.c_str(), destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "Cannot replace experiment file");
#else
    int descriptor = -1;
    for (int attempt = 0; attempt < 32; ++attempt) {
        auto candidate = destination;
        candidate += ".tmp-" + std::to_string(getpid()) + "-" +
                     std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" +
                     std::to_string(sequence.fetch_add(1));
        descriptor = open(candidate.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
        if (descriptor >= 0) {
            temporary.path = std::move(candidate);
            break;
        }
        if (errno != EEXIST)
            throw std::system_error(errno, std::generic_category(), "Cannot create temporary experiment file");
    }
    require(descriptor >= 0, "Cannot allocate a unique temporary experiment file.");
    size_t offset = 0;
    int failure = 0;
    while (offset < contents.size()) {
        const auto written = write(descriptor, contents.data() + offset, contents.size() - offset);
        if (written < 0 && errno == EINTR)
            continue;
        if (written <= 0) {
            failure = written == 0 ? EIO : errno;
            break;
        }
        offset += static_cast<size_t>(written);
    }
    if (!failure && fsync(descriptor) != 0)
        failure = errno;
    if (close(descriptor) != 0 && !failure)
        failure = errno;
    if (failure)
        throw std::system_error(failure, std::generic_category(), "Cannot write complete experiment file");
    std::filesystem::rename(temporary.path, destination);
#endif
    temporary.path.clear();
}
} // namespace

bool saveExperiment(const std::filesystem::path& path, const SolarSystem& system,
                    const RenderSettings& render, const LabState& lab, int selected, std::string& error) {
    try {
        validateSettings(system, render, lab, selected);
        SolarSystem validation;
        require(system.planets().size() == 8, "Experiments must contain exactly eight planets.");
        for (size_t i = 0; i < system.planets().size(); ++i) {
            std::string reason;
            const bool valid = validation.setOrbit(i, system.planets()[i].orbitParameters(), reason);
            require(valid, "Invalid orbit for " + system.planets()[i].data().name + ": " + reason);
        }
        std::ostringstream output;
        output.imbue(std::locale::classic());
        output << std::setprecision(std::numeric_limits<double>::max_digits10);
        output << "SSELAB 1\nsimulation " << system.days() << ' ' << system.speed() << ' ' << system.paused() << ' ' << selected;
        output << "\nrender " << render.orbits << ' ' << render.labels << ' ' << render.bloom << ' ' << render.exposure << ' '
               << static_cast<int>(render.surfaceMode) << ' ' << render.ambient << ' ' << render.diffuse << ' '
               << render.specular << ' ' << render.trails << ' ' << render.velocityVectors << ' ' << render.apsides;
        output << "\nlab " << static_cast<int>(lab.view);
        const auto& eclipse = lab.eclipse;
        output << "\neclipse " << eclipse.lunar << ' ' << eclipse.animate << ' ' << eclipse.showCones << ' '
               << eclipse.observerView << ' ' << eclipse.phaseDegrees << ' ' << eclipse.inclinationDegrees << ' ' << eclipse.angularSpeed;
        output << "\norbits 8\n";
        for (size_t i = 0; i < system.planets().size(); ++i) {
            const auto& orbit = system.planets()[i].orbitParameters();
            output << "orbit " << i << ' ' << orbit.semiMajorAxis << ' ' << orbit.eccentricity << ' '
                   << orbit.inclinationDegrees << ' ' << orbit.periodDays << ' ' << orbit.phaseDegrees << '\n';
        }
        output << "end\n";
        require(static_cast<bool>(output), "Cannot serialize the experiment.");
        writeAtomic(path, output.str());
        error.clear();
        return true;
    } catch (const std::exception& exception) {
        error = exception.what();
        return false;
    }
}

bool loadExperiment(const std::filesystem::path& path, SolarSystem& system,
                    RenderSettings& render, LabState& lab, int& selected, std::string& error) {
    try {
        const auto length = std::filesystem::file_size(path);
        require(length > 0 && length <= maximumFileBytes, "Experiment file must contain between 1 and 65536 bytes.");
        std::ifstream input(path, std::ios::binary);
        require(static_cast<bool>(input), "Cannot open experiment file for reading.");
        std::string contents(static_cast<size_t>(length), '\0');
        require(static_cast<bool>(input.read(contents.data(), static_cast<std::streamsize>(length))),
                "Experiment file could not be read completely.");
        require(input.peek() == std::char_traits<char>::eof(), "Experiment file changed while it was being read.");
        Reader reader(contents);
        reader.expect("SSELAB");
        require(reader.number<int>("version") == 1, "Unsupported experiment version; expected SSELAB 1.");
        SolarSystem stagedSystem;
        RenderSettings stagedRender;
        LabState stagedLab;
        reader.expect("simulation");
        const auto days = reader.number<double>("simulation day");
        const auto speed = reader.number<float>("simulation speed");
        const auto paused = reader.boolean("pause");
        const auto stagedSelected = reader.number<int>("selected planet");
        // Validate before the setters, which deliberately clamp normal UI input.
        require(inRange(days, 0.0, 1.0e10), "Simulation day must be between 0 and 10000000000.");
        require(inRange(speed, 0.05f, 100.0f), "Simulation speed must be between 0.05 and 100.");
        stagedSystem.setDays(days);
        stagedSystem.setSpeed(speed);
        stagedSystem.setPaused(paused);
        reader.expect("render");
        stagedRender.orbits = reader.boolean("orbits");
        stagedRender.labels = reader.boolean("labels");
        stagedRender.bloom = reader.boolean("bloom");
        stagedRender.exposure = reader.number<float>("exposure");
        stagedRender.surfaceMode = static_cast<SurfaceMode>(reader.number<int>("surface mode"));
        stagedRender.ambient = reader.number<float>("ambient light");
        stagedRender.diffuse = reader.number<float>("diffuse light");
        stagedRender.specular = reader.number<float>("specular light");
        stagedRender.trails = reader.boolean("trails");
        stagedRender.velocityVectors = reader.boolean("velocity vectors");
        stagedRender.apsides = reader.boolean("apsides");
        reader.expect("lab");
        stagedLab.view = static_cast<LabView>(reader.number<int>("laboratory view"));
        reader.expect("eclipse");
        auto& eclipse = stagedLab.eclipse;
        eclipse.lunar = reader.boolean("lunar eclipse");
        eclipse.animate = reader.boolean("eclipse animation");
        eclipse.showCones = reader.boolean("shadow cones");
        eclipse.observerView = reader.boolean("observer view");
        eclipse.phaseDegrees = reader.number<float>("eclipse phase");
        eclipse.inclinationDegrees = reader.number<float>("eclipse inclination");
        eclipse.angularSpeed = reader.number<float>("eclipse angular speed");
        validateSettings(stagedSystem, stagedRender, stagedLab, stagedSelected);
        reader.expect("orbits");
        require(reader.number<int>("planet count") == 8, "Experiments must contain exactly eight planets.");
        for (size_t i = 0; i < 8; ++i) {
            reader.expect("orbit");
            require(reader.number<size_t>("planet index") == i, "Orbit records must list planets 0 through 7 exactly once, in order.");
            const OrbitParameters orbit{reader.number<double>("semi-major axis"), reader.number<double>("eccentricity"),
                reader.number<double>("inclination"), reader.number<double>("period"), reader.number<double>("phase")};
            std::string reason;
            const bool valid = stagedSystem.setOrbit(i, orbit, reason);
            require(valid, "Invalid orbit for " + stagedSystem.planets()[i].data().name + ": " + reason);
        }
        reader.expect("end");
        reader.finish();
        system = std::move(stagedSystem);
        render = stagedRender;
        lab = stagedLab;
        selected = stagedSelected;
        error.clear();
        return true;
    } catch (const std::exception& exception) {
        error = exception.what();
        return false;
    }
}
} // namespace sse
