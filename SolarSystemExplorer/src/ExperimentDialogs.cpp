#include "ExperimentDialogs.h"
#include <stdexcept>
#include <string>
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#endif

namespace sse {
std::filesystem::path chooseExperimentFile(GLFWwindow* window, bool save) {
#ifdef _WIN32
    wchar_t buffer[32768] = L"";
    if (save)
        lstrcpyW(buffer, L"Solar-System-Experiment.sselab");
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = glfwGetWin32Window(window);
    dialog.lpstrFilter = L"Solar System experiments (*.sselab)\0*.sselab\0All files (*.*)\0*.*\0\0";
    dialog.lpstrFile = buffer;
    dialog.nMaxFile = static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0]));
    dialog.lpstrTitle = save ? L"Save Solar System experiment" : L"Load Solar System experiment";
    dialog.lpstrDefExt = L"sselab";
    dialog.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST |
                   (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    const BOOL chosen = save ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog);
    if (chosen)
        return std::filesystem::path(buffer);
    const DWORD error = CommDlgExtendedError();
    if (error)
        throw std::runtime_error("Could not open the experiment file dialog (Windows error " +
                                 std::to_string(error) + ").");
    return {};
#else
    (void)window;
    (void)save;
    throw std::runtime_error("Native experiment dialogs are available on Windows. Use --experiment PATH to load.");
#endif
}
} // namespace sse
