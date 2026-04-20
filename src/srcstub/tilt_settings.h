#ifndef tilt_settings_h
#define tilt_settings_h

typedef enum {
    kTiltInvert_XNormal_YNormal = 0,
    kTiltInvert_XInvert_YNormal,
    kTiltInvert_XNormal_YInvert,
    kTiltInvert_XInvert_YInvert,
    kTiltInvert_COUNT
} TiltInvertMode;

// Same enum reused for crank X/Y inversion
typedef TiltInvertMode CrankInvertMode;
#define kCrankInvert_XNormal_YNormal kTiltInvert_XNormal_YNormal
#define kCrankInvert_XInvert_YNormal kTiltInvert_XInvert_YNormal
#define kCrankInvert_XNormal_YInvert kTiltInvert_XNormal_YInvert
#define kCrankInvert_XInvert_YInvert kTiltInvert_XInvert_YInvert
#define kCrankInvert_COUNT           kTiltInvert_COUNT

#endif
