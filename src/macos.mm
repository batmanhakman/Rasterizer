#import <Cocoa/Cocoa.h>

#include "Rasterizer.h"
#include "macos.h"

@interface RasterizerView : NSView
{
    Framebuffer* framebuffer;
    BOOL keyStates[128];
}

- (instancetype)initWithFramebuffer:(Framebuffer*)framebuffer;
- (BOOL)isKeyPressed:(unsigned short)keyCode;
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

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)keyDown:(NSEvent*)event
{
    const unsigned short keyCode = event.keyCode;
    if (keyCode < 128)
    {
        keyStates[keyCode] = YES;
    }
}

- (void)keyUp:(NSEvent*)event
{
    const unsigned short keyCode = event.keyCode;
    if (keyCode < 128)
    {
        keyStates[keyCode] = NO;
    }
}

- (BOOL)isKeyPressed:(unsigned short)keyCode
{
    return keyCode < 128 && keyStates[keyCode];
}

@end

@interface RasterizerAppDelegate : NSObject <NSApplicationDelegate>
@property(nonatomic, strong) NSWindow* window;
@property(nonatomic, strong) NSTimer* animationTimer;
@end

@implementation RasterizerAppDelegate
{
    Framebuffer* framebuffer;
    RasterizerView* rasterizerView;
    Camera camera;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    (void)notification;

    framebuffer = new Framebuffer(1920, 1080);
    camera = Camera{Vector3D{0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 500.0f};

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
    rasterizerView = [[RasterizerView alloc] initWithFramebuffer:framebuffer];
    self.window.contentView = rasterizerView;
    [self.window center];
    [self.window makeKeyAndOrderFront:nil];
    [self.window makeFirstResponder:rasterizerView];

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

    // Use a fixed step to convert held keys into consistent camera movement.
    const float deltaTime = 1.0f / 60.0f;
    const float moveSpeed = 300.0f;
    const float lookSpeed = 1.5f;
    const float moveDistance = moveSpeed * deltaTime;

    // W/S move along the horizontal viewing direction; A/D strafe. Arrow
    // keys adjust yaw and pitch, allowing the camera to look around.
    const Vector3D forward = {std::sin(camera.yaw), 0.0f, std::cos(camera.yaw)};
    const Vector3D right = {std::cos(camera.yaw), 0.0f, -std::sin(camera.yaw)};
    const Vector3D downward = {0.0f, -1.0f, 0.0f};
    const Vector3D up = {0.0f, 1.0f, 0.0f};
    if ([rasterizerView isKeyPressed:13]) // W
    {
        camera.position = Vectors::Addition(camera.position, Vectors::Scalar(forward, moveDistance));
    }
    if ([rasterizerView isKeyPressed:1]) // S
    {
        camera.position = Vectors::Subtraction(camera.position, Vectors::Scalar(forward, moveDistance));
    }
    if ([rasterizerView isKeyPressed:12]) // Q
    {
        camera.position = Vectors::Subtraction(camera.position, Vectors::Scalar(downward, moveDistance));
    }
    if ([rasterizerView isKeyPressed:14]) // E
    {
        camera.position = Vectors::Subtraction(camera.position, Vectors::Scalar(up, moveDistance));
    }
    if ([rasterizerView isKeyPressed:0]) // A
    {
        camera.position = Vectors::Subtraction(camera.position, Vectors::Scalar(right, moveDistance));
    }
    if ([rasterizerView isKeyPressed:2]) // D
    {
        camera.position = Vectors::Addition(camera.position, Vectors::Scalar(right, moveDistance));
    }
    if ([rasterizerView isKeyPressed:123]) // Left arrow
    {
        camera.yaw -= lookSpeed * deltaTime;
    }
    if ([rasterizerView isKeyPressed:124]) // Right arrow
    {
        camera.yaw += lookSpeed * deltaTime;
    }
    if ([rasterizerView isKeyPressed:126]) // Up arrow
    {
        camera.pitch = std::min(camera.pitch + lookSpeed * deltaTime, 1.4f);
    }
    if ([rasterizerView isKeyPressed:125]) // Down arrow
    {
        camera.pitch = std::max(camera.pitch - lookSpeed * deltaTime, -1.4f);
    }

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
    // The models are stationary in world space. The camera supplies all view
    // movement and orientation, so neither shape has a spin angle anymore.
    Rasterizer::Pyramid3DDraw(
        *framebuffer,
        Vertex3D{0.0f, 0.0f, 450.0f},
        120.0f,
        camera,
        pyramidColor);

    Rasterizer::CubeRaw3DDraw(
        *framebuffer,
        Vertex3D{-450.0f, 0.0f, 450.0f},
        120,
        camera,
        cubeColor);

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
