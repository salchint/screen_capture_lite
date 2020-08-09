#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>


pid_t GetForegroundProcessId() {
    pid_t windowPid = 0ul;

    @autoreleasepool {
        NSRunningApplication* app = [[NSWorkspace sharedWorkspace] frontmostApplication];
        pid_t pid = [app processIdentifier];
        windowPid = pid;
    }

    return windowPid;
}
