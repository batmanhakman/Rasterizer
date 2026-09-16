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
@property(nonatomic, strong) NSTimer* animationTimer;
@end

@implementation RasterizerAppDelegate
{
    Framebuffer* framebuffer;
    float squareY;
    float squareVelocityY;
    float squareAngle;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    (void)notification;

    framebuffer = new Framebuffer(1920, 1080);
    squareY = 140.0f;
    squareVelocityY = 0.0f;
    squareAngle = 0.0f;

    // Clear every pixel first; new[] does not initialize the framebuffer.
    framebuffer->Clear(framebuffer->color(0, 0, 0, 255));

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

    // Run the simulation at approximately 60 frames per second.
    self.animationTimer = [NSTimer scheduledTimerWithTimeInterval:(1.0 / 60.0)
                                                            target:self
                                                          selector:@selector(update:)
                                                          userInfo:nil
                                                           repeats:YES];
    [self update:self.animationTimer];
}

- (void)update:(NSTimer*)timer
{
    (void)timer;

    // This fixed time step describes how much simulated time passes per
    // update. Position and velocity are measured in pixels and pixels/second.
    const float deltaTime = 1.0f / 60.0f;
    const float gravity = 900.0f;
    const int halfSize = 100;
    const float floorY = static_cast<float>(framebuffer->GetHeight() - halfSize);

    // Gravity increases downward velocity. The velocity then changes the
    // square's vertical position.
    squareVelocityY += gravity * deltaTime;
    squareY += squareVelocityY * deltaTime;

    if (squareY >= floorY)
    {
        // Keep the square above the floor, reverse its velocity, and reduce
        // it so each bounce loses energy.
        squareY = floorY;
        squareVelocityY = -squareVelocityY * 0.75f;
    }

    // Increase the angle every update. RotatedSquareDraw expects radians.
    squareAngle += 1.5f * deltaTime;

    // Redraw the entire frame: clear the old position, draw the new position,
    // then ask Cocoa to call drawRect: with the updated framebuffer.
    framebuffer->Clear(framebuffer->color(20, 24, 40, 255));
    uint32_t squareColor = framebuffer->color(0, 255, 0, 255);
    uint32_t circleColor = framebuffer->color(0, 0, 255, 255);
    uint32_t pyramidColor = framebuffer->color(255, 255, 0, 255);
    uint32_t cubeColor = framebuffer->color(0, 255, 0, 255);

    // Keep the static square and the circle next to each other.
   // Rasterizer::SquareDraw(
      //  *framebuffer,
       // Vertex2D{180, 120},
        //Vertex2D{380, 120},
        //Vertex2D{380, 320},
        //Vertex2D{180, 320},
       // squareColor);
   // Rasterizer::CircleDraw(
       // *framebuffer,
       // Vertex2D{680, 220},
       // 120,
       // circleColor);
    // The pyramid is a 3D object: one apex and four base corners. Its Y-axis
    // angle makes it spin horizontally from right to left.
    Rasterizer::Pyramid3DDraw(
        *framebuffer,
        Vertex3D{0.0f, 0.0f, 450.0f},
        120.0f,
        squareAngle,
        500.0f,
        pyramidColor);

    Rasterizer::CubeRaw3DDraw(
        *framebuffer,
        Vertex3D{-450.0f, 0.0f, 450.0f},
        120,
        squareAngle,
        500.0f,
        cubeColor);

    // The rotating square remains available:
    // Rasterizer::RotatedSquareDraw(
    //     *framebuffer,
    //     Vertex2D{280, static_cast<int>(squareY)},
    //     halfSize,
    //     squareAngle,
    //     squareColor);

    [self.window.contentView setNeedsDisplay:YES];
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
