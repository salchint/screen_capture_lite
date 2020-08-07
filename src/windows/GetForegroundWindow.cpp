#include "GDIHelpers.h"
#include "ScreenCapture.h"
#include "internal/SCCommon.h"
#include <algorithm>

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#include <Windows.h>

namespace SL {
namespace Screen_Capture {

    Window GetForegroundWindow()
    {
        auto hwnd = ::GetForegroundWindow();
        Window w = {};

        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);
        w.Name[0] = '\n';
        if (pid != GetCurrentProcessId()) {
            GetWindowTextA(hwnd, w.Name, sizeof(w.Name));
        }

        w.Handle = reinterpret_cast<size_t>(hwnd);
        auto windowrect = SL::Screen_Capture::GetWindowRect(hwnd);
        w.Position.x = windowrect.ClientRect.left;
        w.Position.y = windowrect.ClientRect.top;
        w.Size.x = windowrect.ClientRect.right - windowrect.ClientRect.left;
        w.Size.y = windowrect.ClientRect.bottom - windowrect.ClientRect.top;
        std::transform(std::begin(w.Name), std::end(w.Name), std::begin(w.Name), ::tolower);
        return w;
    }

} // namespace Screen_Capture
} // namespace SL
