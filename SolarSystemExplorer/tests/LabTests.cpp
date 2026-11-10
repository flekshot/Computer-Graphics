#include "ExperimentIO.h"
#include <glm/gtc/constants.hpp>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace {
int checks = 0;
void check(bool condition, const char* message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
bool closeEnough(double a, double b, double tolerance = 0.0001) {
    return std::abs(a - b) < tolerance;
}
bool sameOrbit(const sse::OrbitParameters& a, const sse::OrbitParameters& b) {
    return a.semiMajorAxis == b.semiMajorAxis && a.eccentricity == b.eccentricity &&
           a.inclinationDegrees == b.inclinationDegrees && a.periodDays == b.periodDays &&
           a.phaseDegrees == b.phaseDegrees;
}
bool sameSystem(const sse::SolarSystem& a, const sse::SolarSystem& b) {
    if (a.days() != b.days() || a.speed() != b.speed() || a.paused() != b.paused())
        return false;
    for (size_t i = 0; i < a.planets().size(); ++i)
        if (!sameOrbit(a.planets()[i].orbitParameters(), b.planets()[i].orbitParameters()))
            return false;
    return true;
}
bool sameRender(const sse::RenderSettings& a, const sse::RenderSettings& b) {
    return a.orbits == b.orbits && a.labels == b.labels && a.bloom == b.bloom &&
           a.exposure == b.exposure && a.surfaceMode == b.surfaceMode && a.ambient == b.ambient &&
           a.diffuse == b.diffuse && a.specular == b.specular && a.trails == b.trails &&
           a.velocityVectors == b.velocityVectors && a.apsides == b.apsides;
}
bool sameWorkshop(const sse::WorkshopSettings& a, const sse::WorkshopSettings& b) {
    return a.kind == b.kind && a.seed == b.seed && a.radius == b.radius && a.relief == b.relief &&
           a.featureScale == b.featureScale && a.seaLevel == b.seaLevel && a.iceCap == b.iceCap &&
           a.clouds == b.clouds && a.atmosphere == b.atmosphere && a.rotationDegrees == b.rotationDegrees &&
           a.rotationSpeed == b.rotationSpeed && a.animate == b.animate && a.gasBands == b.gasBands &&
           a.gasTurbulence == b.gasTurbulence && a.landColor == b.landColor && a.waterColor == b.waterColor &&
           a.atmosphereColor == b.atmosphereColor && a.gasColorA == b.gasColorA && a.gasColorB == b.gasColorB;
}
bool sameLab(const sse::LabState& a, const sse::LabState& b) {
    const auto& x = a.eclipse;
    const auto& y = b.eclipse;
    return a.view == b.view && x.lunar == y.lunar && x.animate == y.animate &&
           x.showCones == y.showCones && x.observerView == y.observerView &&
           x.phaseDegrees == y.phaseDegrees && x.inclinationDegrees == y.inclinationDegrees &&
           x.angularSpeed == y.angularSpeed && sameWorkshop(a.workshop, b.workshop) &&
           a.missions.active == b.missions.active && a.missions.completed == b.missions.completed;
}
struct TemporaryDirectory {
    std::filesystem::path path;
    TemporaryDirectory() {
        path = std::filesystem::temp_directory_path() /
               ("sselab-tests-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directory(path);
    }
    ~TemporaryDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }
};
std::string read(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
void write(const std::filesystem::path& path, const std::string& contents) {
    std::ofstream stream(path, std::ios::binary);
    stream << contents;
    check(static_cast<bool>(stream), "Test fixture must be written");
}
std::string replaceOnce(std::string text, const std::string& from, const std::string& to) {
    const auto at = text.find(from);
    check(at != std::string::npos, "Corruption fixture must identify a real saved field");
    text.replace(at, from.size(), to);
    return text;
}
std::string changeField(std::string text, const std::string& record, size_t field, const std::string& value) {
    const auto start = text.find("\n" + record + " ");
    check(start != std::string::npos, "Corruption fixture must find its record");
    const auto end = text.find('\n', start + 1);
    std::istringstream input(text.substr(start + 1, end - start - 1));
    std::ostringstream output;
    std::string token;
    size_t index = 0;
    while (input >> token) {
        if (index)
            output << ' ';
        output << (index == field ? value : token);
        ++index;
    }
    check(field < index, "Corruption fixture must target an existing field");
    text.replace(start + 1, end - start - 1, output.str());
    return text;
}

void testOrbits() {
    sse::SolarSystem system;
    const auto original = system;
    std::string error;
    check(!system.isModified(), "Default system must not report edited orbits");
    // Hand-derived original circular convention: inclination around Z, motion toward -Z.
    for (const auto& planet : system.planets()) {
        const auto& d = planet.data();
        const double phase = d.phaseDegrees * glm::pi<double>() / 180.0;
        const double inc = d.inclinationDegrees * glm::pi<double>() / 180.0;
        const glm::vec3 expected(d.orbitRadius * std::cos(phase) * std::cos(inc),
                                 d.orbitRadius * std::cos(phase) * std::sin(inc),
                                 -d.orbitRadius * std::sin(phase));
        check(glm::length(planet.position(0) - expected) < 0.00003f,
              "Default circular positions must preserve the original coordinate convention");
    }
    const sse::OrbitParameters ellipse{20.0, 0.5, 30.0, 100.0, 0.0};
    check(system.setOrbit(2, ellipse, error), "A valid elliptical orbit must be editable");
    check(error.empty() && system.isModified(), "Valid edits clear errors and mark modified");
    const auto& earth = system.planets()[2];
    check(closeEnough(earth.periapsis(), 10) && closeEnough(earth.apoapsis(), 30), "Apsides must use a(1 +/- e)");
    check(closeEnough(glm::length(earth.position(0)), 10), "Phase zero must start at periapsis");
    check(closeEnough(glm::length(earth.position(50)), 30), "Half period must reach apoapsis");
    check(closeEnough(earth.position(0).y, 5), "Inclination must rotate the orbital plane");
    for (double day : {0.0, 0.001, 12.5, 99.999, 1.0e8})
        check(glm::length(earth.position(day) - earth.position(day + 100)) < 0.0001f,
              "Ellipse must close after exactly one period");
    check(closeEnough(glm::length(earth.velocity(0)) / glm::length(earth.velocity(50)), 3.0),
          "Kepler motion must be three times faster at periapsis for e=0.5");
    check(closeEnough(earth.velocity(0).z, -2.0 * glm::pi<double>() / 100.0 * 20.0 * std::sqrt(3.0)),
          "Velocity must use scene units per day and the original orbit direction");
    const double day = 13.0, delta = 0.002;
    const auto finiteDifference = (earth.position(day + delta) - earth.position(day - delta)) /
                                  static_cast<float>(2 * delta);
    check(glm::length(finiteDifference - earth.velocity(day)) < 0.001f,
          "Analytical velocity must agree with position change away from the apsides");
    auto shifted = ellipse;
    shifted.phaseDegrees = 180;
    check(system.setOrbit(2, shifted, error) && closeEnough(glm::length(earth.position(0)), 30),
          "Phase must represent mean anomaly at time zero");
    const auto beforeInvalid = system;
    for (auto invalid : {sse::OrbitParameters{5.9, 0.5, 30, 100, 0},
                         sse::OrbitParameters{180.1, 0.5, 30, 100, 0},
                         sse::OrbitParameters{20, -0.01, 30, 100, 0},
                         sse::OrbitParameters{20, 0.851, 30, 100, 0},
                         sse::OrbitParameters{20, 0.5, -60.1, 100, 0},
                         sse::OrbitParameters{20, 0.5, 60.1, 100, 0},
                         sse::OrbitParameters{20, 0.5, 30, 4.9, 0},
                         sse::OrbitParameters{20, 0.5, 30, 200001, 0},
                         sse::OrbitParameters{20, 0.5, 30, 100, -3601},
                         sse::OrbitParameters{20, 0.5, 30, 100, 3601},
                         sse::OrbitParameters{6, 0.5, 30, 100, 0}}) {
        check(!system.setOrbit(2, invalid, error) && !error.empty(), "Invalid or Sun-crossing orbit must be rejected");
        check(sameSystem(system, beforeInvalid), "Rejected edits must leave every live orbit unchanged");
    }
    for (double invalid : {std::numeric_limits<double>::infinity(),
                           -std::numeric_limits<double>::infinity(),
                           std::numeric_limits<double>::quiet_NaN()}) {
        for (auto field : {&sse::OrbitParameters::semiMajorAxis, &sse::OrbitParameters::eccentricity,
                           &sse::OrbitParameters::inclinationDegrees, &sse::OrbitParameters::periodDays,
                           &sse::OrbitParameters::phaseDegrees}) {
            auto orbit = ellipse;
            orbit.*field = invalid;
            check(!system.setOrbit(2, orbit, error) && sameSystem(system, beforeInvalid),
                  "Every orbital field must reject all non-finite values atomically");
        }
    }
    check(!system.setOrbit(8, ellipse, error), "Invalid planet indices must be rejected");
    auto tangent = ellipse;
    tangent.semiMajorAxis = 10;
    tangent.eccentricity = 1.0 - (sse::SolarSystem::sunRadius + earth.data().radius + 0.4) / 10.0;
    check(!system.setOrbit(2, tangent, error), "The exact Sun clearance boundary must be excluded");
    check(system.setOrbit(0, {180, 0.85, -60, 5, -3600}, error), "Valid extreme settings must be accepted");
    for (double time : {0.0, 0.01, 2.5, 1.0e10}) {
        const auto p = system.planets()[0].position(time);
        const auto v = system.planets()[0].velocity(time);
        check(std::isfinite(p.x + p.y + p.z + v.x + v.y + v.z), "Extreme valid orbits must remain finite");
    }
    system.setDays(123);
    system.setSpeed(4);
    system.setPaused(true);
    system.resetOrbits();
    check(!system.isModified(), "Reset must clear the modified state");
    check(system.days() == 123 && system.speed() == 4 && system.paused(), "Orbit reset must preserve clock controls");
    for (size_t i = 0; i < 8; ++i)
        check(sameOrbit(system.planets()[i].orbitParameters(), original.planets()[i].orbitParameters()),
              "Reset must restore every original orbit exactly");
}

void testPersistence() {
    TemporaryDirectory temporary;
    const auto file = temporary.path / std::filesystem::u8path(u8"СЌРєСЃРїРµСЂРёРјРµРЅС‚-иЅЁйЃ“.sselab");
    sse::SolarSystem source;
    std::string error;
    for (size_t i = 0; i < 8; ++i)
        check(source.setOrbit(i, {30.25 + i, 0.21 + i * 0.01, -31.125 + i, 85.125 + i, 359.25 - i}, error),
              "Roundtrip setup must create all eight distinct edited orbits");
    source.setDays(987654321.1234567);
    source.setSpeed(0.123456789f);
    source.setPaused(true);
    sse::RenderSettings render;
    render.orbits = false; render.labels = false; render.bloom = false;
    render.exposure = 2.1234567f; render.surfaceMode = sse::SurfaceMode::Normals;
    render.ambient = 0.1234567f; render.diffuse = 2.2345678f; render.specular = 0.3456789f;
    render.trails = true; render.velocityVectors = true; render.apsides = true;
    sse::LabState lab;
    lab.view = sse::LabView::Workshop;
    lab.eclipse = {true, true, false, true, -123.4567f, -12.34567f, 24.56789f};
    auto& world = lab.workshop;
    world.kind = sse::WorkshopKind::Gas;
    world.seed = 999999; world.radius = 2.1234567f; world.relief = 0.2345678f; world.featureScale = 10.12345f;
    world.seaLevel = 0.654321f; world.iceCap = 0.345678f; world.clouds = 0.765432f; world.atmosphere = 0.876543f;
    world.rotationDegrees = -3456.789f; world.rotationSpeed = -12.34567f; world.animate = false;
    world.gasBands = 23; world.gasTurbulence = 0.789123f;
    world.landColor = {0.1234567f, 0.2345678f, 0.3456789f};
    world.waterColor = {0.4567891f, 0.5678912f, 0.6789123f};
    world.atmosphereColor = {0.7891234f, 0.8912345f, 0.9123456f};
    world.gasColorA = {0.1122334f, 0.2233445f, 0.3344556f};
    world.gasColorB = {0.4455667f, 0.5566778f, 0.6677889f};
    lab.missions.active = 4; lab.missions.completed = {true, false, true, false, true};
    check(sse::saveExperiment(file, source, render, lab, 6, error), "Unicode experiment path must save");
    check(error.empty(), "Successful saving must clear any previous error");
    const auto valid = read(file);
    check(valid.rfind("SSELAB 2\n", 0) == 0, "Saves must advertise the version 2 format");
    sse::SolarSystem loaded;
    sse::RenderSettings loadedRender;
    sse::LabState loadedLab;
    int selected = -1;
    check(sse::loadExperiment(file, loaded, loadedRender, loadedLab, selected, error), "Saved experiment must load");
    check(sameSystem(source, loaded) && sameRender(render, loadedRender) && sameLab(lab, loadedLab) && selected == 6,
          "All eight orbits, clock, selection and every graphics/lab field must roundtrip exactly");
    for (const auto view : {sse::LabView::Explore, sse::LabView::Orbit, sse::LabView::Eclipse}) {
        auto otherViewLab = lab;
        otherViewLab.view = view;
        check(sse::saveExperiment(file, source, render, otherViewLab, 6, error),
              "Workshop settings and mission progress must save from every other view");
        sse::SolarSystem otherSystem;
        sse::RenderSettings otherRender;
        sse::LabState otherLab;
        int otherSelected = -1;
        check(sse::loadExperiment(file, otherSystem, otherRender, otherLab, otherSelected, error),
              "Experiments saved from another view must load");
        check(sameSystem(source, otherSystem) && sameRender(render, otherRender) &&
                  sameLab(otherViewLab, otherLab) && otherSelected == 6,
              "Changing views must preserve the entire saved workshop and mission progress");
    }
    // A late invalid record must not apply the valid earlier records either.
    const auto unchanged = loaded;
    const auto unchangedRender = loadedRender;
    const auto unchangedLab = loadedLab;
    const auto bad = temporary.path / "broken.sselab";
    auto rejected = [&](const std::string& contents) {
        write(bad, contents);
        check(!sse::loadExperiment(bad, loaded, loadedRender, loadedLab, selected, error) && !error.empty(),
              "Malformed experiment must fail with a useful error");
        check(sameSystem(loaded, unchanged) && sameRender(loadedRender, unchangedRender) &&
                  sameLab(loadedLab, unchangedLab) && selected == 6,
              "A failed load must not partially mutate any live experiment state");
    };
    rejected("");
    rejected(replaceOnce(valid, "SSELAB 2", "SSELAB 99"));
    rejected(valid.substr(0, valid.size() / 2));
    rejected(valid + "unexpected trailing data\n");
    rejected(replaceOnce(valid, "orbits 8", "orbits 9"));
    rejected(replaceOnce(valid, "orbit 7 ", "orbit 6 "));
    rejected(replaceOnce(valid, "lab 3", "lab 4"));
    rejected(replaceOnce(valid, "render 0 0 0 ", "render 2 0 0 "));
    rejected(replaceOnce(valid, "eclipse 1 1 0 1 ", "eclipse 1 1 -1 1 "));
    rejected(replaceOnce(valid, "orbit 7 37.25", "orbit 7 nan"));
    rejected(replaceOnce(valid, "orbit 7 37.25", "orbit 7 181"));
    rejected(std::string(65537, 'X'));
    for (const auto& value : {"nan", "inf", "-1", "10000000001", "1x"})
        rejected(changeField(valid, "simulation", 1, value));
    for (const auto& value : {"nan", "0", "101"})
        rejected(changeField(valid, "simulation", 2, value));
    rejected(changeField(valid, "simulation", 3, "true"));
    rejected(changeField(valid, "simulation", 4, "8"));
    rejected(changeField(valid, "simulation", 4, "-2"));
    for (const auto& value : {"nan", "0.19", "3.1"})
        rejected(changeField(valid, "render", 4, value));
    rejected(changeField(valid, "render", 5, "-1"));
    rejected(changeField(valid, "render", 5, "3"));
    for (size_t field : {6u, 7u, 8u})
        for (const auto& value : {"nan", "-0.01", "3.1"})
            rejected(changeField(valid, "render", field, value));
    for (const auto& value : {"nan", "-3601", "3601"})
        rejected(changeField(valid, "eclipse", 5, value));
    for (const auto& value : {"nan", "-31", "31"})
        rejected(changeField(valid, "eclipse", 6, value));
    for (const auto& value : {"nan", "-0.01", "91"})
        rejected(changeField(valid, "eclipse", 7, value));
    for (size_t field : {2u, 3u, 4u, 5u, 6u})
        rejected(changeField(valid, "orbit 7", field, "nan"));
    rejected(changeField(valid, "workshop", 1, "2"));
    for (const auto& value : {"-1", "1000000", "4294967296"})
        rejected(changeField(valid, "workshop", 2, value));
    const std::array<std::pair<const char*, const char*>, 11> invalidRanges{{
        {"0.49", "3.01"}, {"-0.01", "0.251"}, {"0.99", "12.01"}, {"-0.01", "1.01"},
        {"-0.01", "0.601"}, {"-0.01", "1.01"}, {"-0.01", "1.01"},
        {"-3601", "3601"}, {"-45.01", "45.01"}, {"2", "25"}, {"-0.01", "1.01"}}};
    for (size_t i = 0; i < invalidRanges.size(); ++i) {
        const size_t field = i < 9 ? i + 3 : i + 4;
        for (const auto& value : {invalidRanges[i].first, invalidRanges[i].second, "nan", "inf"})
            rejected(changeField(valid, "workshop", field, value));
    }
    rejected(changeField(valid, "workshop", 12, "true"));
    for (const auto& record : {"land_color", "water_color", "atmosphere_color", "gas_color_a", "gas_color_b"})
        for (size_t field : {1u, 2u, 3u})
            for (const auto& value : {"-0.01", "1.01", "nan"})
                rejected(changeField(valid, record, field, value));
    rejected(changeField(valid, "missions", 1, "-2"));
    rejected(changeField(valid, "missions", 1, "5"));
    for (size_t field : {2u, 3u, 4u, 5u, 6u})
        rejected(changeField(valid, "missions", field, "2"));
    rejected(replaceOnce(valid, "\nwater_color ", "\nland_color "));
    auto legacy = replaceOnce(valid, "SSELAB 2", "SSELAB 1");
    const auto workshopStart = legacy.find("\nworkshop ");
    legacy.erase(workshopStart, legacy.find("\norbits ", workshopStart) - workshopStart);
    rejected(legacy); // A version 1 file cannot select the newly added fourth view.
    check(!sse::loadExperiment(temporary.path / "absent.sselab", loaded, loadedRender, loadedLab, selected, error),
          "Missing files must fail cleanly");
    check(sameSystem(loaded, unchanged) && sameRender(loadedRender, unchangedRender) &&
              sameLab(loadedLab, unchangedLab) && selected == 6,
          "A missing file must preserve every live field");
    legacy = replaceOnce(legacy, "lab 3", "lab 2");
    write(bad, legacy);
    check(sse::loadExperiment(bad, loaded, loadedRender, loadedLab, selected, error), "Version 1 experiments must still load");
    auto legacyLab = lab;
    legacyLab.view = sse::LabView::Eclipse;
    legacyLab.workshop = {}; legacyLab.missions = {};
    check(sameSystem(source, loaded) && sameRender(render, loadedRender) && sameLab(legacyLab, loadedLab) && selected == 6,
          "Version 1 loading must restore existing state and reset workshop/progress to defaults");
    // Defaults, overview selection and false booleans must also roundtrip.
    sse::SolarSystem defaults;
    sse::RenderSettings defaultRender;
    sse::LabState defaultLab;
    check(sse::saveExperiment(file, defaults, defaultRender, defaultLab, -1, error), "Existing save must be replaceable");
    check(sse::loadExperiment(file, loaded, loadedRender, loadedLab, selected, error), "Default experiment must load");
    check(sameSystem(defaults, loaded) && sameRender(defaultRender, loadedRender) &&
              sameLab(defaultLab, loadedLab) && selected == -1 && !loaded.isModified(),
          "Defaults and overview selection must survive replacing a save");
    const auto originalBytes = read(file);
    auto invalidRender = defaultRender;
    invalidRender.exposure = std::numeric_limits<float>::infinity();
    check(!sse::saveExperiment(file, defaults, invalidRender, defaultLab, -1, error), "Invalid settings must not save");
    check(read(file) == originalBytes, "Failed save validation must preserve the prior file");
    invalidRender = defaultRender;
    invalidRender.surfaceMode = static_cast<sse::SurfaceMode>(4);
    check(!sse::saveExperiment(file, defaults, invalidRender, defaultLab, -1, error), "Unknown surface modes must not save");
    check(!sse::saveExperiment(file, defaults, defaultRender, defaultLab, 8, error), "Out-of-range selection must not save");
    auto invalidLab = defaultLab;
    invalidLab.eclipse.angularSpeed = -1;
    check(!sse::saveExperiment(file, defaults, defaultRender, invalidLab, -1, error), "Invalid eclipse values must not save");
    invalidLab = defaultLab;
    invalidLab.workshop.landColor[2] = std::numeric_limits<float>::quiet_NaN();
    check(!sse::saveExperiment(file, defaults, defaultRender, invalidLab, -1, error), "Non-finite workshop colors must not save");
    invalidLab = defaultLab; invalidLab.missions.active = 5;
    check(!sse::saveExperiment(file, defaults, defaultRender, invalidLab, -1, error), "Invalid active mission must not save");
    check(read(file) == originalBytes, "Every rejected save must preserve the prior file");
    check(!sse::saveExperiment(temporary.path / "missing" / "save.sselab", defaults, defaultRender, defaultLab, -1, error),
          "A missing destination directory must report a saving error");
    check(!sse::saveExperiment(temporary.path, defaults, defaultRender, defaultLab, -1, error),
          "Saving over a directory must report an error");
#ifdef _WIN32
    // Disallow deletion/replacement while still allowing reads of the known-good file.
    const HANDLE lock = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    check(lock != INVALID_HANDLE_VALUE, "Overwrite-failure fixture must lock the original file");
    const bool savedWhileLocked = sse::saveExperiment(file, source, render, lab, 6, error);
    CloseHandle(lock);
    check(!savedWhileLocked && !error.empty(), "A denied replacement must report failure");
    check(read(file) == originalBytes, "Denied replacement must preserve the original file byte-for-byte");
#endif
    size_t fileCount = 0;
    for (const auto& entry : std::filesystem::directory_iterator(temporary.path)) {
        (void)entry;
        ++fileCount;
    }
    check(fileCount == 2, "Failed writes and replacements must clean up temporary files");
}
} // namespace

int main() {
    try {
        testOrbits();
        testPersistence();
        std::cout << "PASS: " << checks << " lab assertions (Kepler orbits, validation, experiment persistence)\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL after " << checks << " assertions: " << e.what() << '\n';
        return 1;
    }
}

