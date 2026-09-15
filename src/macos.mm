#import <Cocoa/Cocoa.h>

#include "Rasterizer.h"
#include "macos.h"

@interface RasterizerView : NSView
{
    Framebuffer* framebuffer;
}

- (instancetype)initWithFramebuffer:(Framebuffer*)framebuffer;
@end

@implementation RasterizerView

- (instancetype)initWithFramebuffer:(Framebuffer*)newFramebuffer
{
    self = [super initWithFrame:NSMakeRect(0, 0, newFramebuffer->GetWidth(), newFramebuffer->GetHeight())];
    if (self != nil)
    {
        framebuffer = newFramebuffer;
    }
    return self;
}

- (void)dealloc
{
    delete framebuffer;
}

- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;

    // The framebuffer stores pixels as 0xRRGGBBAA. Core Graphics reads those
    // bytes as red, green, blue, and alpha because of the bitmap options below.
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef bitmapContext = CGBitmapContextCreate(
        framebuffer->GetBuffer(),
        framebuffer->GetWidth(),
        framebuffer->GetHeight(),
        8,
        framebuffer->GetWidth() * sizeof(uint32_t),
        colorSpace,
        kCGImageAlphaNoneSkipLast | kCGBitmapByteOrder32Big);

    if (bitmapContext == nullptr)
    {
        CGColorSpaceRelease(colorSpace);
        return;
    }

    // Create an image from our software-rendered pixels, then draw that image
    // into Cocoa's current window context. Drawing into bitmapContext itself
    // would only draw back into the offscreen framebuffer.
    CGImageRef image = CGBitmapContextCreateImage(bitmapContext);
    CGContextRef windowContext = [[NSGraphicsContext currentContext] CGContext];
    if (image != nullptr && windowContext != nullptr)
    {
        CGContextDrawImage(windowContext, self.bounds, image);
    }

    if (image != nullptr)
    {
        CGImageRelease(image);
    }
    CGContextRelease(bitmapContext);
    CGColorSpaceRelease(colorSpace);
}

@end

@interface RasterizerAppDelegate : NSObject <NSApplicationDelegate>
@property(nonatomic, strong) NSWindow* window;
@end

@implementation RasterizerAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    (void)notification;

    Framebuffer* framebuffer = new Framebuffer(1920, 1080);

    // Clear every pixel first; new[] does not initialize the framebuffer.
    framebuffer->Clear(framebuffer->color(0, 0, 0, 0));

    // Build a green color as 0xRRGGBBAA and draw a line into the buffer.
    uint32_t color = framebuffer->color(0, 255, 0, 255);
    uint32_t red = framebuffer->color(0, 0, 255, 255);
    uint32_t SquareColor = framebuffer->color(255, 0, 128, 255);
    // a Square has 4 sides, each Vertex2D will connect themsevles.
    Rasterizer::SquareDraw(
        *framebuffer,
        Vertex2D{200, 100},
        Vertex2D{800, 100},
        Vertex2D{800, 700},
        Vertex2D{200, 700},
        SquareColor);

    // Each triangle corner is a Vertex2D containing one x and y coordinate.
    //Rasterizer::TriangleDraw(
      //  *framebuffer,
      //  Vertex2D{500, 100},
       // Vertex2D{200, 700},
       // Vertex2D{800, 700},
       // color);
    // Draw the circle
    Rasterizer::CircleDraw(
        *framebuffer,
        Vertex2D{1200, 450},
        300,
        red);

    NSRect frame = NSMakeRect(0, 0, framebuffer->GetWidth(), framebuffer->GetHeight());
    self.window = [[NSWindow alloc]
        initWithContentRect:frame
                  styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                             NSWindowStyleMaskResizable)
                    backing:NSBackingStoreBuffered
                      defer:NO];
    self.window.title = @"Rasterizer";
    self.window.contentView = [[RasterizerView alloc] initWithFramebuffer:framebuffer];
    [self.window center];
    [self.window makeKeyAndOrderFront:nil];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender
{
    (void)sender;
    return YES;
}

@end

int RunApplication()
{
    NSApplication* application = [NSApplication sharedApplication];
    RasterizerAppDelegate* delegate = [[RasterizerAppDelegate alloc] init];
    application.delegate = delegate;
    [application setActivationPolicy:NSApplicationActivationPolicyRegular];
    [application activateIgnoringOtherApps:YES];
    [application run];
    return 0;
}
