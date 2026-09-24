#import <Cocoa/Cocoa.h>
#include "Scene.h"
#include "macos.h"
#include <chrono>
#include <exception>

@interface RasterizerView : NSView
{
    Scene* scene;
    SceneInput input;
    NSPoint previousMouse;
}
- (instancetype)initWithScene:(Scene*)newScene;
- (SceneInput)sceneInput;
- (void)clearInput;
@end

@implementation RasterizerView
- (instancetype)initWithScene:(Scene*)newScene
{
    self = [super initWithFrame:NSMakeRect(0, 0, 1000, 760)];
    if (self) scene = newScene;
    return self;
}
- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)isOpaque { return YES; }
- (SceneInput)sceneInput { return input; }
- (void)clearInput { input = {}; }
- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;
    const Framebuffer& frame = scene->Frame();
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    // Pixels are 0xAARRGGBB: in memory, little-endian bytes are B,G,R,A.
    CGContextRef bitmap = CGBitmapContextCreate(frame.GetBuffer(), frame.GetWidth(), frame.GetHeight(),
        8, frame.GetWidth() * sizeof(uint32_t), colorSpace,
        kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little);
    if (bitmap)
    {
        CGImageRef image = CGBitmapContextCreateImage(bitmap);
        CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
        if (image && context) CGContextDrawImage(context, self.bounds, image);
        if (image) CGImageRelease(image);
        CGContextRelease(bitmap);
    }
    CGColorSpaceRelease(colorSpace);
}
- (void)setKey:(unsigned short)key pressed:(bool)pressed
{
    switch (key)
    {
    case 123: case 0: input.left = pressed; break;
    case 124: case 2: input.right = pressed; break;
    case 126: input.up = pressed; break;
    case 125: input.down = pressed; break;
    case 13: input.zoomIn = pressed; break;
    case 1: input.zoomOut = pressed; break;
    }
}
- (void)keyDown:(NSEvent*)event
{
    [self setKey:event.keyCode pressed:true];
    if (event.keyCode == 15) scene->Reset();
    if (event.keyCode == 53) [NSApp terminate:nil];
}
- (void)keyUp:(NSEvent*)event { [self setKey:event.keyCode pressed:false]; }
- (BOOL)resignFirstResponder
{
    [self clearInput];
    return [super resignFirstResponder];
}
- (void)mouseDown:(NSEvent*)event
{
    [self.window makeFirstResponder:self];
    previousMouse = [self convertPoint:event.locationInWindow fromView:nil];
}
- (void)mouseDragged:(NSEvent*)event
{
    const NSPoint mouse = [self convertPoint:event.locationInWindow fromView:nil];
    scene->Orbit(static_cast<float>(mouse.x - previousMouse.x) * 0.006f,
                 static_cast<float>(previousMouse.y - mouse.y) * 0.006f);
    previousMouse = mouse;
}
- (void)scrollWheel:(NSEvent*)event
{
    scene->Zoom(static_cast<float>(event.scrollingDeltaY) * (event.hasPreciseScrollingDeltas ? 0.08f : 1.0f));
}
@end

@interface RasterizerAppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@property(nonatomic, strong) NSWindow* window;
@property(nonatomic, strong) NSTimer* animationTimer;
@property(nonatomic, assign) int exitCode;
@end

@implementation RasterizerAppDelegate
{
    Scene* scene;
    RasterizerView* rasterizerView;
    std::chrono::steady_clock::time_point previousTick;
}
- (void)dealloc { delete scene; }
- (void)showError:(const std::string&)message
{
    [self.animationTimer invalidate];
    self.exitCode = 1;
    NSAlert* alert = [[NSAlert alloc] init];
    alert.messageText = @"Rasterizer could not continue";
    alert.informativeText = [NSString stringWithUTF8String:message.c_str()] ?: @"Unknown error";
    [alert runModal];
    [NSApp stop:nil];
    [NSApp abortModal];
}
- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    (void)notification;
    try
    {
        scene = new Scene;
        std::string error;
        if (!scene->Load(Scene::DefaultModelPath(), error))
        {
            [self showError:"Could not load the knight. Keep the assets folder beside Rasterizer.\n\n" + error];
            return;
        }
        scene->Render();
        self.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 1000, 760)
            styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable)
            backing:NSBackingStoreBuffered defer:NO];
        self.window.title = @"Rasterizer | Knight - drag / arrows: orbit  W/S / wheel: zoom  R: reset  Esc: quit";
        self.window.minSize = NSMakeSize(320, 260);
        self.window.delegate = self;
        rasterizerView = [[RasterizerView alloc] initWithScene:scene];
        self.window.contentView = rasterizerView;
        [self.window center];
        [self.window makeKeyAndOrderFront:nil];
        [self.window makeFirstResponder:rasterizerView];
        previousTick = std::chrono::steady_clock::now();
        self.animationTimer = [NSTimer scheduledTimerWithTimeInterval:(1.0 / 60.0)
            target:self selector:@selector(update:) userInfo:nil repeats:YES];
        [[NSRunLoop mainRunLoop] addTimer:self.animationTimer forMode:NSRunLoopCommonModes];
    }
    catch (const std::exception& error) { [self showError:error.what()]; }
}
- (void)update:(NSTimer*)timer
{
    (void)timer;
    try
    {
        const auto now = std::chrono::steady_clock::now();
        const float seconds = std::chrono::duration<float>(now - previousTick).count();
        previousTick = now;
        if (self.window.miniaturized) return;
        const NSSize size = rasterizerView.bounds.size;
        scene->Resize(static_cast<int>(size.width), static_cast<int>(size.height));
        scene->Update([rasterizerView sceneInput], seconds);
        scene->Render();
        [rasterizerView setNeedsDisplay:YES];
    }
    catch (const std::exception& error) { [self showError:error.what()]; }
}
- (void)applicationDidResignActive:(NSNotification*)notification
{
    (void)notification;
    [rasterizerView clearInput];
}
- (void)windowDidResignKey:(NSNotification*)notification
{
    (void)notification;
    [rasterizerView clearInput];
}
- (void)applicationWillTerminate:(NSNotification*)notification
{
    (void)notification;
    [self.animationTimer invalidate];
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender
{
    (void)sender;
    return YES;
}
@end

int RunApplication()
{
    @autoreleasepool
    {
        NSApplication* application = [NSApplication sharedApplication];
        RasterizerAppDelegate* delegate = [[RasterizerAppDelegate alloc] init];
        application.delegate = delegate;
        [application setActivationPolicy:NSApplicationActivationPolicyRegular];
        NSMenu* menuBar = [[NSMenu alloc] init];
        NSMenuItem* applicationItem = [[NSMenuItem alloc] init];
        NSMenu* applicationMenu = [[NSMenu alloc] init];
        [applicationMenu addItemWithTitle:@"Quit Rasterizer" action:@selector(terminate:) keyEquivalent:@"q"];
        applicationItem.submenu = applicationMenu;
        [menuBar addItem:applicationItem];
        application.mainMenu = menuBar;
        [application activateIgnoringOtherApps:YES];
        [application run];
        return delegate.exitCode;
    }
}
