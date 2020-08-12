#include "ScreenCapture.h"
#include "internal/SCCommon.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <algorithm>
#include <string>
#include <vector>
#include <iostream>

#include <cstdlib>
#include <cstdio>
#include <locale>

#include <X11/Xlib.h>           // `apt-get install libx11-dev`
#include <X11/Xmu/WinUtil.h>    // `apt-get install libxmu-dev`


namespace
{
    Bool xerror = False;

    Display* OpenDisplay()
    {
        //printf("connecting X server ... ");
        Display* d = XOpenDisplay(NULL);
        if(d == NULL)
        {
            //printf("fail\n");
            exit(1);
        }
        else
        {
            //printf("success\n");
        }
        return d;
    }

    int HandleError(Display* display, XErrorEvent* error){
        printf("ERROR: X11 error\n");
        xerror = True;
        return 1;
    }

    ::Window GetFocusWindow(Display* d)
    {
        ::Window w;
        int revert_to;
        //printf("getting input focus window ... ");
        XGetInputFocus(d, &w, &revert_to); // see man
        if(xerror)
        {
            //printf("fail\n");
            exit(1);
        }
        else if(w == None)
        {
            //printf("no focus window\n");
            exit(1);
        }
        else
        {
            //printf("success (window: %d)\n", (int)w);
        }

        return w;
    }

    // get the top window.
    // a top window have the following specifications.
    //  * the start window is contained the descendent windows.
    //  * the parent window is the root window.
    ::Window GetTopWindow(Display* d, ::Window start)
    {
        ::Window w = start;
        ::Window parent = start;
        ::Window root = None;
        ::Window *children;
        unsigned int nchildren;
        Status s;

        //printf("getting top window ... \n");
        while (parent != root)
        {
            w = parent;
            s = XQueryTree(d, w, &root, &parent, &children, &nchildren); // see man

            if (s)
            {
                XFree(children);
            }

            if(xerror)
            {
                //printf("fail\n");
                exit(1);
            }

            //printf("  get parent (window: %d)\n", (int)w);
        }

        //printf("success (window: %d)\n", (int)w);

        return w;
    }

    // search a named window (that has a WM_STATE prop)
    // on the descendent windows of the argment Window.
    ::Window GetNamedWindow(Display* d, ::Window start)
    {
        ::Window w;
        //printf("getting named window ... ");
        w = XmuClientWindow(d, start); // see man
        if(w == start)
        {
            //printf("fail\n");
        }
        //printf("success (window: %d)\n", (int) w);
        return w;
    }

    //// (XFetchName cannot get a name with multi-byte chars)
    //void print_window_name(Display* d, ::Window w)
    //{
        //XTextProperty prop;
        //Status s;

        ////printf("window name:\n");

        //s = XGetWMName(d, w, &prop); // see man
        //if(!xerror && s)
        //{
            //int count = 0, result;
            //char **list = NULL;
            //result = XmbTextPropertyToTextList(d, &prop, &list, &count); // see man
            //if(result == Success)
            //{
                ////printf("\t%s\n", list[0]);
            //}
            //else
            //{
                ////printf("ERROR: XmbTextPropertyToTextList\n");
            //}
        //}
        //else
        //{
            ////printf("ERROR: XGetWMName\n");
        //}
    //}

    class UniqueTextProperty
    {
        public:
            UniqueTextProperty()
            {
                p.value = nullptr;
            }

            UniqueTextProperty(const UniqueTextProperty&) = delete;
            UniqueTextProperty& operator=(const UniqueTextProperty&) = delete;

            UniqueTextProperty(UniqueTextProperty&& other):
                p{other.p}
            {
                other.p = XTextProperty{};
            }

            UniqueTextProperty& operator=(UniqueTextProperty&& other)
            {
                swap(*this, other);
                return *this;
            }

            friend void swap(UniqueTextProperty& lhs, UniqueTextProperty& rhs)
            {
                std::swap(lhs.p, rhs.p);
            }

            ~UniqueTextProperty()
            {
                if (p.value) {
                    XFree(p.value);
                }
            }

            auto& get()
            {
                return p;
            }

        private:
            XTextProperty p;
    };

    auto GetWMName(Display* display, Window window)
    {
        auto x = UniqueTextProperty{};
        XGetWMName(display, window, &x.get());
        return x;
    }

    auto TextPropertyToStrings(Display* display, const XTextProperty& prop)
    {
        char **list;
        auto n_strings = 0;
        std::vector<std::string> result;

        auto status = XmbTextPropertyToTextList(
                display,
                &prop,
                &list,
                &n_strings
                );

        if (status < Success or not n_strings or not *list) {
            return result;
        }

        for (auto i = 0; i < n_strings; ++i) {
            result.emplace_back(list[i]);
        }

        XFreeStringList(list);

        return result;
    }
}

namespace SL
{
namespace Screen_Capture
{

    Window GetForegroundWindow()
    {
        Display* display;
        ::Window xwindow;

        // for XmbTextPropertyToTextList
        setlocale(LC_ALL, ""); // see man locale

        display = OpenDisplay();
        XSetErrorHandler(HandleError);

        // get active window
        xwindow = GetFocusWindow(display);
        xwindow = GetTopWindow(display, xwindow);
        xwindow = GetNamedWindow(display, xwindow);

        using namespace std::string_literals;

        auto wm_name = GetWMName(display, xwindow);
        auto candidates = TextPropertyToStrings(display, wm_name.get());
        Window w = {};
        w.Handle = reinterpret_cast<size_t>(xwindow);

        XWindowAttributes wndattr;
        XGetWindowAttributes(display, xwindow, &wndattr);

        w.Position = Point{ wndattr.x, wndattr.y };
        w.Size = Point{ wndattr.width, wndattr.height };

        auto name = candidates.empty() ? ""s : std::move(candidates.front());
        std::transform(name.begin(), name.end(), std::begin(w.Name), ::tolower);
        return w;
    }
}
}
