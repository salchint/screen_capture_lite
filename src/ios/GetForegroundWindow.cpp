#include "ScreenCapture.h"
#include "internal/SCCommon.h"
#include <algorithm>
#include <string>
#include <array>
#include <iostream>
#include "TargetConditionals.h"
#include <ApplicationServices/ApplicationServices.h>

extern "C" pid_t GetForegroundProcessId();

namespace SL
{
namespace Screen_Capture
{

    Window GetForegroundWindow()
    {
        CGDisplayCount count=0;
        CGGetActiveDisplayList(0, 0, &count);
        std::vector<CGDirectDisplayID> displays;
        displays.resize(count);
        CGGetActiveDisplayList(count, displays.data(), &count);
        auto xscale=1.0f;
        auto yscale = 1.0f;

        for(auto  i = 0; i < count; i++) {
            //only include non-mirrored displays
            if(CGDisplayMirrorsDisplay(displays[i]) == kCGNullDirectDisplay){

                auto dismode =CGDisplayCopyDisplayMode(displays[i]);
                auto scaledsize = CGDisplayBounds(displays[i]);

                auto pixelwidth = CGDisplayModeGetPixelWidth(dismode);
                auto pixelheight = CGDisplayModeGetPixelHeight(dismode);

                CGDisplayModeRelease(dismode);

                if(scaledsize.size.width !=pixelwidth){//scaling going on!
                    xscale = static_cast<float>(pixelwidth)/static_cast<float>(scaledsize.size.width);
                }
                if(scaledsize.size.height !=pixelheight){//scaling going on!
                    yscale = static_cast<float>(pixelheight)/static_cast<float>(scaledsize.size.height);
                }
                break;
            }
        }

        Window w = {};
        auto windowList = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements, kCGNullWindowID);
        CFIndex numWindows = CFArrayGetCount(windowList);

        for( int i = 0; i < (int)numWindows; i++ ) {
            auto windowpid=0u;
            auto foregroundPid = GetForegroundProcessId();

            auto dict = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(windowList, i));
            auto cfwindowPid = static_cast<CFNumberRef>(CFDictionaryGetValue(dict, kCGWindowOwnerPID));
            CFNumberGetValue(cfwindowPid, kCFNumberIntType, &windowpid);

            if (windowpid == foregroundPid) {
                auto cfwindowname = static_cast<CFStringRef>(CFDictionaryGetValue(dict, kCGWindowName));
                CFStringGetCString(cfwindowname, w.Name, sizeof(w.Name), kCFStringEncodingUTF8);
                w.Name[sizeof(w.Name)-1] = '\n';

                auto windowid=0u;
                CFNumberGetValue(static_cast<CFNumberRef>(CFDictionaryGetValue(dict, kCGWindowNumber)), kCFNumberIntType, &windowid);
                w.Handle = static_cast<size_t>(windowid);

                std::array<char, 256> ownerName;
                auto cfownername = static_cast<CFStringRef>(CFDictionaryGetValue(dict, kCGWindowOwnerName));
                CFStringGetCString(cfownername, ownerName.data(), ownerName.size(), kCFStringEncodingUTF8);
                std::cout << "Active Window [" << windowpid << "/" << foregroundPid << "] @" << w.Handle << " " << ownerName.data() << "/" << w.Name << std::endl;

                auto dims =static_cast<CFDictionaryRef>(CFDictionaryGetValue(dict, kCGWindowBounds));
                CGRect rect;
                CGRectMakeWithDictionaryRepresentation(dims, &rect);
                w.Position.x = static_cast<int>(rect.origin.x);
                w.Position.y = static_cast<int>(rect.origin.y);

                w.Size.x = static_cast<int>(rect.size.width * xscale);
                w.Size.y = static_cast<int>(rect.size.height* yscale);
                std::transform(std::begin(w.Name), std::end(w.Name), std::begin(w.Name), ::tolower);

                break;
            }

            //std::array<char, 256> ownerName;
            //auto cfownername = static_cast<CFStringRef>(CFDictionaryGetValue(dict, kCGWindowOwnerName));
            //CFStringGetCString(cfownername, ownerName.data(), ownerName.size(), kCFStringEncodingUTF8);
            //std::cout << "Active Window [" << windowpid << "/" << foregroundPid << "] @" << w.Handle << " " << ownerName.data() << "/" << w.Name << std::endl;
        }

        return w;
    }
}
}
