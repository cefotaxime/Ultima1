// Original Ultima II Windows-port bootstrap: no copyrighted game code/assets.
#include <windows.h>
#include <shellapi.h>
#include <filesystem>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cwctype>

namespace fs = std::filesystem;
struct Entry {
    const wchar_t* filename;
    std::uintmax_t referenceSize;
};
static constexpr Entry kReference[] = {
    {L"ULTIMAII.EXE", 37344},
    {L"PLAYER", 384},
    {L"MAPX00", 4224},
    {L"MONSTERS", 2176},
    {L"PICOUT", 16384},
    {L"PICDNG", 16384},
    {L"PICTWN", 16384},
    {L"PICSPA", 16384}
};

static fs::path g_gameDir;
static std::wstring g_report;
static std::wstring lower(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
    return s;
}
static fs::path findCaseInsensitive(const fs::path& dir, const wchar_t* target) {
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) return {};
    for (fs::directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec), end;
         it != end && !ec; it.increment(ec)) {
        if (!it->is_regular_file(ec)) continue;
        if (lower(it->path().filename().wstring()) == lower(target)) return it->path();
    }
    return {};
}
static std::wstring inspect(const fs::path& dir) {
    std::wostringstream out;
    std::error_code ec;
    out << L"Ultima II original DOS data inspector (not yet playable)\n\n";
    out << L"Selected folder: " << dir.wstring() << L"\n\n";
    if (!fs::is_directory(dir, ec)) {
        out << L"Folder not found. Pass the original DOS game directory as an argument.\n";
        return out.str();
    }
    unsigned found = 0;
    for (const auto& entry : kReference) {
        const auto path = findCaseInsensitive(dir, entry.filename);
        if (path.empty()) {
            out << L"[MISSING] " << entry.filename << L"\n";
            continue;
        }
        const auto size = fs::file_size(path, ec);
        if (ec) {
            out << L"[ERROR] " << entry.filename << L" (size unreadable)\n";
            ec.clear();
            continue;
        }
        ++found;
        out << (size == entry.referenceSize ? L"[MATCH] " : L"[DIFFERENT SIZE] ")
            << entry.filename << L" - " << size << L" bytes";
        if (size != entry.referenceSize) out << L" (reference " << entry.referenceSize << L")";
        out << L"\n";
    }
    out << L"\nFiles found: " << found << L"/" << (sizeof(kReference) / sizeof(kReference[0]))
        << L"\nAsset sizes are diagnostic, not game-version proof.\n";
    return out.str();
}
static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC dc = BeginPaint(hwnd, &ps);
        RECT client{};
        GetClientRect(hwnd, &client);
        FillRect(dc, &client, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(24, 24, 24));
        RECT body{20, 20, client.right - 20, client.bottom - 20};
        DrawTextW(dc, g_report.c_str(), -1, &body, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}
int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int show) {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    bool probe = argc >= 2 && std::wstring(argv[1]) == L"--probe";
    int pathIndex = probe ? 2 : 1;
    if (argc > pathIndex) g_gameDir = fs::path(argv[pathIndex]);
    else g_gameDir = fs::current_path();
    if (argv) LocalFree(argv);
    g_report = inspect(g_gameDir);
    if (probe) {
        // A GUI-subsystem application needs an explicitly attached console for CLI use.
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
            if (out != INVALID_HANDLE_VALUE && out != nullptr) {
                DWORD written = 0;
                const std::wstring message = g_report + L"\n";
                WriteConsoleW(out, message.c_str(), static_cast<DWORD>(message.size()), &written, nullptr);
            }
        }
        return 0;
    }
    const wchar_t* klass = L"Ultima2OriginalWinBootstrap";
    WNDCLASSW wc{};
    wc.lpfnWndProc = wndProc;
    wc.hInstance = inst;
    wc.lpszClassName = klass;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    if (!RegisterClassW(&wc)) return 1;
    HWND hwnd = CreateWindowExW(0, klass, L"Ultima II - Original DOS Windows Port (bootstrap)",
                                WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                780, 480, nullptr, nullptr, inst, nullptr);
    if (!hwnd) return 2;
    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}
