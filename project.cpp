#ifdef _WIN32
#  include <windows.h>
#  include <mmsystem.h>  // For sound effects
#  pragma comment(lib, "winmm.lib")  // Link Windows Multimedia library
#endif

#include <GL/freeglut.h>
#include <cmath>
#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <direct.h>

int winW = 1000, winH = 700;
float viewportTopY = 20.0f;  // Current viewport top Y coordinate (updated in reshape)

// Color struct definition (needed before Firecracker struct)
struct Color { float r, g, b; };

// Rocket animation state
float rocketAnimTime = 0.0f;      // Animation progress (0.0 to 1.0)
float rocketSpeed = 0.0083f;      // Animation speed (completes in ~2 seconds at 60fps)

// Drone animation state
float droneAnimTime = 0.0f;       // Animation progress (0.0 to 1.0)
float droneSpeed = 0.00238f;      // Animation speed (completes in ~7 seconds at 60fps)
bool droneDirection = true;       // true = DIU→City, false = City→DIU

// Cloud animation state
float cloudOffsetX = 0.0f;        // Current X offset for cloud movement
float cloudSpeed = 0.008f;        // Cloud movement speed (slower - pixels per frame)
float cloudLoopWidth = 80.0f;     // Width of the loop (when cloud goes beyond this, reset)
float cloudYPositions[6];         // Random Y positions for each cloud (initialized once)
bool cloudYInitialized = false;   // Whether cloud Y positions have been initialized

// Vehicle animation state
float vehicleOffsetX = 0.0f;      // Current X offset for vehicle movement
float vehicleSpeed = 0.05f;       // Vehicle movement speed
float vehicleLoopWidth = 80.0f;   // Width of the loop

// Flag animation state
float flagWaveTime = 0.0f;        // Time for flag waving animation
float flagWaveSpeed = 0.05f;      // Flag wave animation speed

// Building destruction state
bool cityBuildingDestroyed = false;
float explosionTime = 0.0f;      // Time since explosion (increases after impact)
float explosionSpeed = 0.02f;    // Speed of explosion animation
int missileHitCount = 0;          // Number of times missile has hit (max 2)
bool missileAttackActive = false; // Whether missiles should continue launching (starts false)
float programStartTime = 0.0f;    // Time when program started (in seconds)
float missileDelay = 10.0f;       // Delay before first missile launches (10 seconds)
float secondMissileDelay = 4.0f;  // Delay before second missile launches after first hit (4 seconds)
float firstMissileHitTime = -1.0f; // Time when first missile hit (-1 if not hit yet)
bool timeInitialized = false;     // Whether timer has been initialized
int startTime = 0;                // Start time for delay calculation
int countdownValue = 10;          // Countdown timer value (10 to 0)
int lastFrameTime = 0;            // Last frame time for deltaTime calculation
float deltaTime = 0.0167f;        // Frame time delta (defaults to 60 FPS)

// Firecracker jubilation state
struct Firecracker {
    float x, y;                   // Current position
    float vx, vy;                 // Velocity components
    float launchAngle;            // Launch angle in degrees
    float travelDistance;         // Distance traveled
    float maxDistance;            // Distance before explosion
    bool exploded;                // Whether it has exploded
    float explosionTime;          // Time since explosion (0.0 to 2.0)
    int trajectoryType;           // 0=straight up, 1=-x then up, 2=+x then up
    Color color;                   // Random color (red, yellow, blue, green)
};

// People animation state
struct Person {
    float x, y;                   // Position on road
    float walkCycle;              // Walking animation cycle (0.0 to 1.0)
    bool walkingToCity;           // Walking towards City University
    bool walkingToDIU;            // Walking back to DIU
    bool stopped;                 // Stopped in front of DIU
    bool celebrating;             // Celebrating (raising/lowering hands)
    float celebrationCycle;       // Celebration animation cycle (0.0 to 1.0)
    Color shirtColor;             // Random shirt color
    float speed;                  // Walking speed
};

const int MAX_PEOPLE = 30;
Person people[MAX_PEOPLE];
int numPeople = 30;
bool peopleInitialized = false;

const int MAX_FIRECRACKERS = 7;
Firecracker firecrackers[MAX_FIRECRACKERS];
bool firecrackersInitialized = false;
bool firecrackersActive = false;
float firecrackerJubilationTime = 0.0f;  // Overall animation time
float firecrackerDelay = 4.0f;            // Delay before firecrackers launch (4 seconds after building destruction)
float buildingDestroyedTime = -1.0f;     // Time when building was destroyed (-1 if not destroyed yet)

// Sound effect state
bool rocketSoundPlayed = false;      // Track if rocket sound was played for current launch
bool explosionSoundPlayed = false;  // Track if explosion sound was played
bool firecrackerSoundPlayed = false; // Track if firecracker sound was played

// -------------------------------------------------------------
// Debug logging helper
// -------------------------------------------------------------
void debugLog(const std::string& location, const std::string& message, const std::string& dataJson, const std::string& runId = "run1", const std::string& hypothesisId = "A") {
    // Ensure .cursor directory exists
    #ifdef _WIN32
    _mkdir(".cursor");
    #else
    mkdir(".cursor", 0755);
    #endif
    
    std::ofstream logFile(".cursor/debug.log", std::ios::app);
    if (logFile.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        logFile << "{\"sessionId\":\"debug-session\",\"runId\":\"" << runId 
                << "\",\"hypothesisId\":\"" << hypothesisId 
                << "\",\"location\":\"" << location 
                << "\",\"message\":\"" << message 
                << "\",\"data\":" << dataJson 
                << ",\"timestamp\":" << timestamp << "}" << std::endl;
        logFile.close();
    }
}

// -------------------------------------------------------------
// Night Sky Colors
// -------------------------------------------------------------
const Color NIGHT_SKY = { 0.02f, 0.05f, 0.15f };        // dark night-blue
const Color HORIZON_LIGHT = { 0.04f, 0.08f, 0.20f };    // slightly lighter near horizon
const Color MOON_COLOR = { 0.98f, 0.98f, 0.95f };       // soft off-white moon
const Color MOON_GLOW = { 0.10f, 0.12f, 0.25f };       // moon glow halo
const Color STAR_BRIGHT = { 1.00f, 1.00f, 1.00f };     // bright stars
const Color STAR_MEDIUM = { 0.90f, 0.90f, 0.95f };     // medium stars
const Color STAR_DIM = { 0.70f, 0.75f, 0.85f };         // dim stars
const Color CLOUD_COLOR = { 0.75f, 0.78f, 0.85f };     // light grey-blue clouds

// -------------------------------------------------------------
// Environment Colors
// -------------------------------------------------------------
const Color MOUNTAIN_COLOR = { 0.05f, 0.08f, 0.12f };  // dark mountain silhouette
const Color ROAD_COLOR = { 0.15f, 0.15f, 0.18f };      // dark grey road
const Color ROAD_LINE = { 0.4f, 0.4f, 0.4f };          // road marking lines
const Color STREET_LIGHT_POLE = { 0.2f, 0.2f, 0.2f };  // dark grey pole
const Color STREET_LIGHT_GLOW = { 1.0f, 0.95f, 0.7f }; // warm yellow light glow
const Color TREE_TRUNK = { 0.3f, 0.2f, 0.15f };        // brown tree trunk
const Color TREE_LEAVES = { 0.1f, 0.25f, 0.1f };       // dark green leaves (night)
const Color CAR_BODY = { 0.3f, 0.4f, 0.6f };           // blue car
const Color CAR_LIGHT = { 1.0f, 0.9f, 0.7f };          // car headlight
const Color FLAG_POLE = { 0.25f, 0.25f, 0.25f };       // grey flagpole
const Color FLAG_RED = { 0.8f, 0.1f, 0.1f };           // red flag
const Color FLAG_GREEN = { 0.1f, 0.5f, 0.2f };         // green flag

// -------------------------------------------------------------
// Rocket Colors
// -------------------------------------------------------------
const Color ROCKET_BODY = { 0.9f, 0.3f, 0.1f };        // red/orange body
const Color ROCKET_NOSE = { 0.7f, 0.2f, 0.05f };      // darker red nose
const Color ROCKET_FIN = { 0.9f, 0.3f, 0.1f };        // same as body
const Color FLAME_ORANGE = { 1.0f, 0.6f, 0.2f };      // bright orange flame
const Color FLAME_YELLOW = { 1.0f, 0.9f, 0.3f };     // yellow flame

// -------------------------------------------------------------
// Destruction Effect Colors
// -------------------------------------------------------------
const Color FIRE_RED = { 1.0f, 0.2f, 0.0f };         // bright red fire
const Color FIRE_ORANGE = { 1.0f, 0.5f, 0.1f };      // orange fire
const Color FIRE_YELLOW = { 1.0f, 0.8f, 0.2f };      // yellow fire
const Color SMOKE_DARK = { 0.2f, 0.2f, 0.2f };      // dark gray smoke
const Color SMOKE_LIGHT = { 0.4f, 0.4f, 0.4f };     // light gray smoke
const Color EXPLOSION_YELLOW = { 1.0f, 1.0f, 0.3f }; // bright yellow explosion
const Color EXPLOSION_ORANGE = { 1.0f, 0.6f, 0.1f };  // orange explosion
const Color DEBRIS_COLOR = { 0.5f, 0.4f, 0.3f };     // brown/gray debris

// -------------------------------------------------------------
// Firecracker Colors
// -------------------------------------------------------------
const Color FIRECRACKER_RED = { 1.0f, 0.2f, 0.1f };      // bright red
const Color FIRECRACKER_ORANGE = { 1.0f, 0.5f, 0.0f };   // bright orange
const Color FIRECRACKER_YELLOW = { 1.0f, 0.9f, 0.2f };   // bright yellow
const Color FIRECRACKER_BLUE = { 0.2f, 0.4f, 1.0f };     // bright blue
const Color FIRECRACKER_GREEN = { 0.2f, 1.0f, 0.3f };   // bright green
const Color FIRECRACKER_WHITE = { 1.0f, 1.0f, 0.9f };    // white/yellow
const Color FIRECRACKER_SPARK = { 1.0f, 0.8f, 0.3f };   // golden spark

// -------------------------------------------------------------
// Drone Colors
// -------------------------------------------------------------
const Color DRONE_BODY = { 0.3f, 0.3f, 0.35f };       // dark gray body
const Color DRONE_PROP = { 0.2f, 0.2f, 0.25f };      // darker gray propellers
const Color DRONE_LIGHT = { 0.8f, 0.8f, 1.0f };       // light blue LED

// -------------------------------------------------------------
// Global helpers
// -------------------------------------------------------------
inline void setColor(const Color& c) { glColor3f(c.r, c.g, c.b); }

void filledRect(float x1, float y1, float x2, float y2, const Color& c) {
    setColor(c);
    glBegin(GL_QUADS);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
    glEnd();
}

void outlineRect(float x1, float y1, float x2, float y2,
    const Color& c, float lineW = 2.0f) {
    setColor(c);
    glLineWidth(lineW);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
    glEnd();
    glLineWidth(1.0f);
}

// alias for city code
void outlinedRect(float x1, float y1, float x2, float y2,
    const Color& border, float lineW = 2.0f) {
    outlineRect(x1, y1, x2, y2, border, lineW);
}

void framedRect(float x1, float y1, float x2, float y2,
    const Color& fill, const Color& border, float lw = 2.0f) {
    filledRect(x1, y1, x2, y2, fill);
    outlineRect(x1, y1, x2, y2, border, lw);
}

void drawText(const std::string& s, float x, float y, void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (char c : s) glutBitmapCharacter(font, c);
}

void drawCenteredText(const std::string& s, float centerX, float y,
    float charWidth, void* font = GLUT_BITMAP_HELVETICA_18) {
    float approxWidth = s.size() * charWidth;
    float startX = centerX - approxWidth * 0.5f;
    drawText(s, startX, y, font);
}

// Draw large text using stroke fonts (much bigger than bitmap fonts)
void drawLargeStrokeText(const std::string& s, float centerX, float y, float scale) {
    glPushMatrix();
    
    // Calculate text width first (before scaling)
    float textWidth = 0.0f;
    for (char c : s) {
        textWidth += glutStrokeWidth(GLUT_STROKE_ROMAN, c);
    }
    
    // Translate to center position, then scale, then offset by half text width
    glTranslatef(centerX, y, 0.0f);
    glScalef(scale, scale, 1.0f);
    glTranslatef(-textWidth * 0.5f, 0.0f, 0.0f);
    
    // Draw text with stroke font
    for (char c : s) {
        glutStrokeCharacter(GLUT_STROKE_ROMAN, c);
    }
    
    glPopMatrix();
}

// -------------------------------------------------------------
// Sound effect functions
// -------------------------------------------------------------
void playSound(const char* soundFile) {
#ifdef _WIN32
    // Extract just the filename if a path was provided
    std::string filename = soundFile;
    size_t lastSlash = filename.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        filename = filename.substr(lastSlash + 1);
    }
    
    // Try multiple path options
    std::string fullPath;
    BOOL result = FALSE;
    
    // First try: Music folder in current directory (same as project.cpp)
    fullPath = "Music/" + filename;
    result = PlaySound(fullPath.c_str(), NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
    if (result) {
        return;  // Success
    }
    
    // Second try: directly in current directory
    fullPath = filename;
    result = PlaySound(fullPath.c_str(), NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
    if (result) {
        return;  // Success
    }
    
    // Third try: relative to executable directory
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string exeDir = exePath;
    size_t lastSlash2 = exeDir.find_last_of("\\/");
    if (lastSlash2 != std::string::npos) {
        exeDir = exeDir.substr(0, lastSlash2 + 1);
        // Try in Music folder relative to exe
        fullPath = exeDir + "Music/" + filename;
        result = PlaySound(fullPath.c_str(), NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
        if (result) {
            return;  // Success
        }
        // Try directly in exe directory
        fullPath = exeDir + filename;
        result = PlaySound(fullPath.c_str(), NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
        if (result) {
            return;  // Success
        }
    }
    
    // Fourth try: absolute path from project root (fallback)
    fullPath = "E:/Projects/Computer Graphics/Computer Graphics/Computer-Graphics-main/Music/" + filename;
    PlaySound(fullPath.c_str(), NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
#endif
}

void playSoundSync(const char* soundFile) {
#ifdef _WIN32
    // Play sound synchronously (blocks until done)
    PlaySound(soundFile, NULL, SND_FILENAME | SND_SYNC);
#endif
}

void stopSound() {
#ifdef _WIN32
    // Stop any currently playing sound
    PlaySound(NULL, NULL, 0);
#endif
}

// -------------------------------------------------------------
// Circle drawing helper for moon and clouds
// -------------------------------------------------------------
void drawCircle(float cx, float cy, float r, const Color& fill, int segments = 40) {
    setColor(fill);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);  // center
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * 3.14159265358979323846f * i / segments;
        glVertex2f(cx + r * cosf(angle), cy + r * sinf(angle));
    }
    glEnd();
}

// -------------------------------------------------------------
// Moon drawing
// -------------------------------------------------------------
void drawMoon() {
    float moonX = 3.0f;      // slightly right of center
    float moonY = viewportTopY * 0.90f;  // 90% of viewport height (dynamically adjusts, leaves space at top)
    
    // #region agent log
    {
        std::ostringstream json;
        json << "{\"moonX\":" << moonX << ",\"moonY\":" << moonY << ",\"viewportTopY\":" << viewportTopY << "}";
        debugLog("project.cpp:128", "Moon position (dynamic)", json.str(), "run3", "D");
    }
    // #endregion
    
    // Moon glow halo (slightly larger, darker)
    drawCircle(moonX, moonY, 1.3f, MOON_GLOW, 40);
    
    // Main moon
    drawCircle(moonX, moonY, 1.0f, MOON_COLOR, 40);
}

// -------------------------------------------------------------
// Stars drawing
// -------------------------------------------------------------
void drawStars() {
    // Scale factor: convert from original [0, 20] range to current viewport range
    float scaleFactor = viewportTopY / 20.0f;
    
    // Predefined star positions (original Y values in [0, 20] range, will be scaled)
    struct Star { float x; float origY; const Color* color; float size; };
    
    Star stars[] = {
        // Bright stars (original Y values)
        { -35.0f, 16.5f, &STAR_BRIGHT, 0.08f },
        { -30.0f, 18.0f, &STAR_BRIGHT, 0.08f },
        { -25.0f, 15.5f, &STAR_BRIGHT, 0.08f },
        { -15.0f, 17.5f, &STAR_BRIGHT, 0.08f },
        { -10.0f, 16.0f, &STAR_BRIGHT, 0.08f },
        { 5.0f, 18.5f, &STAR_BRIGHT, 0.08f },
        { 10.0f, 15.0f, &STAR_BRIGHT, 0.08f },
        { 15.0f, 17.0f, &STAR_BRIGHT, 0.08f },
        { 25.0f, 16.5f, &STAR_BRIGHT, 0.08f },
        { 30.0f, 18.0f, &STAR_BRIGHT, 0.08f },
        { 35.0f, 15.5f, &STAR_BRIGHT, 0.08f },
        
        // Medium stars
        { -32.0f, 14.0f, &STAR_MEDIUM, 0.06f },
        { -28.0f, 17.0f, &STAR_MEDIUM, 0.06f },
        { -20.0f, 16.0f, &STAR_MEDIUM, 0.06f },
        { -18.0f, 13.5f, &STAR_MEDIUM, 0.06f },
        { -12.0f, 18.0f, &STAR_MEDIUM, 0.06f },
        { -8.0f, 14.5f, &STAR_MEDIUM, 0.06f },
        { -5.0f, 17.0f, &STAR_MEDIUM, 0.06f },
        { 0.0f, 15.5f, &STAR_MEDIUM, 0.06f },
        { 8.0f, 18.0f, &STAR_MEDIUM, 0.06f },
        { 12.0f, 14.0f, &STAR_MEDIUM, 0.06f },
        { 18.0f, 16.5f, &STAR_MEDIUM, 0.06f },
        { 22.0f, 17.5f, &STAR_MEDIUM, 0.06f },
        { 28.0f, 14.5f, &STAR_MEDIUM, 0.06f },
        { 32.0f, 16.0f, &STAR_MEDIUM, 0.06f },
        
        // Dim stars
        { -33.0f, 15.0f, &STAR_DIM, 0.05f },
        { -27.0f, 13.0f, &STAR_DIM, 0.05f },
        { -22.0f, 18.0f, &STAR_DIM, 0.05f },
        { -16.0f, 15.0f, &STAR_DIM, 0.05f },
        { -14.0f, 12.5f, &STAR_DIM, 0.05f },
        { -6.0f, 16.5f, &STAR_DIM, 0.05f },
        { -3.0f, 13.0f, &STAR_DIM, 0.05f },
        { 3.0f, 16.0f, &STAR_DIM, 0.05f },
        { 6.0f, 13.5f, &STAR_DIM, 0.05f },
        { 14.0f, 17.0f, &STAR_DIM, 0.05f },
        { 20.0f, 14.0f, &STAR_DIM, 0.05f },
        { 27.0f, 15.5f, &STAR_DIM, 0.05f },
        { 33.0f, 17.0f, &STAR_DIM, 0.05f }
    };
    
    int numStars = sizeof(stars) / sizeof(stars[0]);
    for (int i = 0; i < numStars; ++i) {
        float scaledY = stars[i].origY * scaleFactor;
        drawCircle(stars[i].x, scaledY, stars[i].size, *(stars[i].color), 20);
    }
}

// -------------------------------------------------------------
// Cloud drawing
// -------------------------------------------------------------
void drawCloud(float cx, float cy, float scale) {
    // Create cloud using overlapping circles
    float r1 = 0.6f * scale;
    float r2 = 0.5f * scale;
    float r3 = 0.55f * scale;
    float r4 = 0.45f * scale;
    float r5 = 0.5f * scale;
    
    drawCircle(cx - 0.4f * scale, cy, r1, CLOUD_COLOR, 30);
    drawCircle(cx + 0.2f * scale, cy, r2, CLOUD_COLOR, 30);
    drawCircle(cx + 0.6f * scale, cy, r3, CLOUD_COLOR, 30);
    drawCircle(cx - 0.1f * scale, cy + 0.2f * scale, r4, CLOUD_COLOR, 30);
    drawCircle(cx + 0.4f * scale, cy + 0.15f * scale, r5, CLOUD_COLOR, 30);
}

void initializeCloudYPositions() {
    // Initialize random Y positions for each cloud (in top area of screen)
    // Random Y between 85% and 95% of viewport top
    for (int i = 0; i < 6; ++i) {
        float minY = viewportTopY * 0.85f;
        float maxY = viewportTopY * 0.95f;
        cloudYPositions[i] = minY + (rand() % 100) * (maxY - minY) / 100.0f;
    }
    cloudYInitialized = true;
}

void drawClouds() {
    // Initialize random Y positions if not already done (or if viewport changed significantly)
    if (!cloudYInitialized) {
        initializeCloudYPositions();
    }
    
    // Base X positions for clouds (will be offset by cloudOffsetX for animation)
    float baseX[] = { -30.0f, -20.0f, -8.0f, 8.0f, 20.0f, 30.0f };
    float cloudScale[] = { 1.2f, 1.0f, 1.1f, 0.9f, 1.0f, 1.1f };
    
    // Draw clouds with animated X positions (looping from left to right)
    for (int i = 0; i < 6; ++i) {
        float cloudX = baseX[i] + cloudOffsetX;
        // Loop clouds: when they go off the right side, wrap to left side
        if (cloudX > 40.0f) {
            cloudX -= cloudLoopWidth;
        }
        drawCloud(cloudX, cloudYPositions[i], cloudScale[i]);
    }
}

// -------------------------------------------------------------
// Environment Elements Drawing
// -------------------------------------------------------------
void drawBackgroundMountains() {
    // Draw mountain silhouettes in the background
    setColor(MOUNTAIN_COLOR);
    
    // Left mountains
    glBegin(GL_TRIANGLES);
    glVertex2f(-40.0f, 0.0f);
    glVertex2f(-35.0f, 4.0f);
    glVertex2f(-30.0f, 0.0f);
    glEnd();
    
    glBegin(GL_TRIANGLES);
    glVertex2f(-32.0f, 0.0f);
    glVertex2f(-28.0f, 5.5f);
    glVertex2f(-24.0f, 0.0f);
    glEnd();
    
    // Right mountains
    glBegin(GL_TRIANGLES);
    glVertex2f(24.0f, 0.0f);
    glVertex2f(28.0f, 5.0f);
    glVertex2f(32.0f, 0.0f);
    glEnd();
    
    glBegin(GL_TRIANGLES);
    glVertex2f(30.0f, 0.0f);
    glVertex2f(35.0f, 4.5f);
    glVertex2f(40.0f, 0.0f);
    glEnd();
}

void drawRoad() {
    // Road connecting the two universities
    float roadY = 1.8f;  // Road position (above ground)
    float roadWidth = 8.0f;
    
    // Main road
    filledRect(-40.0f, roadY, 40.0f, roadY + 0.4f, ROAD_COLOR);
    
    // Road markings (center line)
    setColor(ROAD_LINE);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (float x = -38.0f; x < 38.0f; x += 2.0f) {
        glVertex2f(x, roadY + 0.2f);
        glVertex2f(x + 1.0f, roadY + 0.2f);
    }
    glEnd();
    glLineWidth(1.0f);
}

void drawStreetLight(float x, float y) {
    // Street light pole
    setColor(STREET_LIGHT_POLE);
    filledRect(x - 0.08f, y, x + 0.08f, y + 2.5f, STREET_LIGHT_POLE);
    
    // Light fixture
    filledRect(x - 0.3f, y + 2.5f, x + 0.3f, y + 2.7f, STREET_LIGHT_POLE);
    
    // Glowing light effect
    setColor(STREET_LIGHT_GLOW);
    drawCircle(x, y + 2.6f, 0.4f, STREET_LIGHT_GLOW, 20);
    // Inner bright core
    drawCircle(x, y + 2.6f, 0.2f, {1.0f, 1.0f, 0.9f}, 15);
}

void drawStreetLights() {
    float roadY = 1.8f;
    float lightY = roadY + 0.4f;
    
    // Street lights along the road
    drawStreetLight(-30.0f, lightY);
    drawStreetLight(-15.0f, lightY);
    drawStreetLight(0.0f, lightY);
    drawStreetLight(15.0f, lightY);
    drawStreetLight(30.0f, lightY);
}

void drawTree(float x, float y, float scale) {
    // Tree trunk
    setColor(TREE_TRUNK);
    filledRect(x - 0.15f * scale, y, x + 0.15f * scale, y + 1.2f * scale, TREE_TRUNK);
    
    // Tree leaves (circular)
    setColor(TREE_LEAVES);
    drawCircle(x, y + 1.5f * scale, 0.8f * scale, TREE_LEAVES, 20);
    drawCircle(x - 0.3f * scale, y + 1.3f * scale, 0.6f * scale, TREE_LEAVES, 15);
    drawCircle(x + 0.3f * scale, y + 1.3f * scale, 0.6f * scale, TREE_LEAVES, 15);
}

void drawTrees() {
    // Trees around the buildings
    // Left side (near City University)
    drawTree(-28.0f, 2.0f, 1.0f);
    drawTree(-26.0f, 2.0f, 0.9f);
    drawTree(-24.0f, 2.0f, 1.1f);
    
    // Right side (near DIU)
    drawTree(24.0f, 2.0f, 1.0f);
    drawTree(26.0f, 2.0f, 0.9f);
    drawTree(28.0f, 2.0f, 1.1f);
    
    // Center area
    drawTree(-10.0f, 2.0f, 0.8f);
    drawTree(10.0f, 2.0f, 0.8f);
}

void drawVehicle(float x, float y) {
    // Car body
    setColor(CAR_BODY);
    filledRect(x - 0.8f, y, x + 0.8f, y + 0.5f, CAR_BODY);
    
    // Car roof
    filledRect(x - 0.5f, y + 0.5f, x + 0.5f, y + 0.9f, CAR_BODY);
    
    // Windows
    setColor({0.1f, 0.15f, 0.2f});
    filledRect(x - 0.4f, y + 0.55f, x - 0.1f, y + 0.85f, {0.1f, 0.15f, 0.2f});
    filledRect(x + 0.1f, y + 0.55f, x + 0.4f, y + 0.85f, {0.1f, 0.15f, 0.2f});
    
    // Headlights
    setColor(CAR_LIGHT);
    drawCircle(x + 0.75f, y + 0.25f, 0.12f, CAR_LIGHT, 10);
    drawCircle(x - 0.75f, y + 0.25f, 0.12f, CAR_LIGHT, 10);
    
    // Wheels
    setColor({0.1f, 0.1f, 0.1f});
    drawCircle(x - 0.5f, y, 0.15f, {0.1f, 0.1f, 0.1f}, 12);
    drawCircle(x + 0.5f, y, 0.15f, {0.1f, 0.1f, 0.1f}, 12);
}

void drawVehicles() {
    float roadY = 1.8f;
    float vehicleY = roadY + 0.5f;
    
    // Draw vehicles moving on the road
    float vehicle1X = -35.0f + vehicleOffsetX;
    if (vehicle1X > 40.0f) vehicle1X -= vehicleLoopWidth;
    drawVehicle(vehicle1X, vehicleY);
    
    float vehicle2X = -10.0f + vehicleOffsetX;
    if (vehicle2X > 40.0f) vehicle2X -= vehicleLoopWidth;
    drawVehicle(vehicle2X, vehicleY);
    
    float vehicle3X = 15.0f + vehicleOffsetX;
    if (vehicle3X > 40.0f) vehicle3X -= vehicleLoopWidth;
    drawVehicle(vehicle3X, vehicleY);
}

void drawFlag(float x, float y, const Color& flagColor) {
    // Flagpole
    setColor(FLAG_POLE);
    filledRect(x - 0.05f, y, x + 0.05f, y + 3.0f, FLAG_POLE);
    
    // Flag with waving animation
    float waveOffset = sinf(flagWaveTime + x * 0.5f) * 0.3f;
    
    glPushMatrix();
    glTranslatef(x, y + 3.0f, 0.0f);
    
    setColor(flagColor);
    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(1.2f + waveOffset, -0.2f);
    glVertex2f(1.2f + waveOffset, -0.8f);
    glVertex2f(0.0f, -0.6f);
    glEnd();
    
    glPopMatrix();
}

void drawFlags() {
    // Flags on buildings
    // City University flag
    drawFlag(-22.0f, 10.9f, FLAG_RED);
    
    // DIU flag
    drawFlag(22.0f, 16.8f, FLAG_GREEN);
}

// -------------------------------------------------------------
// Rocket Animation
// -------------------------------------------------------------
void calculateRocketPosition(float t, float& x, float& y, float& angle) {
    // Start point: DIU roof
    float x1 = 22.0f, y1 = 16.8f;  // DIU roof top
    // End point: City University roof
    float x2 = -22.0f, y2 = 10.9f;  // City roof top
    
    // Linear X interpolation
    x = x1 + t * (x2 - x1);
    
    // Parabolic Y with peak height (creates arc with ~4 unit peak)
    float linearY = y1 + t * (y2 - y1);
    float arcHeight = 4.0f * t * (1.0f - t);
    y = linearY + arcHeight;
    
    // Calculate angle based on trajectory direction
    // Use a small delta to approximate the tangent
    float delta = 0.01f;
    float tNext = t + delta;
    if (tNext > 1.0f) tNext = 1.0f;
    
    float xNext = x1 + tNext * (x2 - x1);
    float linearYNext = y1 + tNext * (y2 - y1);
    float arcHeightNext = 4.0f * tNext * (1.0f - tNext);
    float yNext = linearYNext + arcHeightNext;
    
    float dx = xNext - x;
    float dy = yNext - y;
    angle = atan2f(dy, dx) * 180.0f / 3.14159265358979323846f;  // Convert to degrees
}

void drawRocket(float x, float y, float angle) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glRotatef(angle, 0.0f, 0.0f, 1.0f);
    
    // Rocket body (horizontal rectangle, pointing right initially)
    float bodyWidth = 2.0f;   // length (horizontal)
    float bodyHeight = 0.8f;  // width (vertical)
    float bodyX1 = -bodyWidth * 0.5f;  // left (back of rocket)
    float bodyX2 = bodyWidth * 0.5f;   // right (front of rocket)
    float bodyY1 = -bodyHeight * 0.5f; // bottom
    float bodyY2 = bodyHeight * 0.5f;  // top
    framedRect(bodyX1, bodyY1, bodyX2, bodyY2, ROCKET_BODY, {0.0f, 0.0f, 0.0f}, 1.5f);
    
    // Nose cone (triangle at front/right)
    float noseLength = 0.4f;
    setColor(ROCKET_NOSE);
    glBegin(GL_TRIANGLES);
    glVertex2f(bodyX2 + noseLength, 0.0f);        // tip (pointing right)
    glVertex2f(bodyX2, bodyY1);                  // bottom base
    glVertex2f(bodyX2, bodyY2);                   // top base
    glEnd();
    
    // Outline nose
    setColor({0.0f, 0.0f, 0.0f});
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(bodyX2 + noseLength, 0.0f);
    glVertex2f(bodyX2, bodyY1);
    glVertex2f(bodyX2, bodyY2);
    glEnd();
    glLineWidth(1.0f);
    
    // Fins (3 triangular fins at back/left)
    float finSize = 0.3f;
    float finX = bodyX1;
    setColor(ROCKET_FIN);
    
    // Top fin
    glBegin(GL_TRIANGLES);
    glVertex2f(finX, bodyY2);
    glVertex2f(finX - finSize, bodyY2 + finSize * 0.5f);
    glVertex2f(finX - finSize * 0.3f, bodyY2);
    glEnd();
    
    // Bottom fin
    glBegin(GL_TRIANGLES);
    glVertex2f(finX, bodyY1);
    glVertex2f(finX - finSize, bodyY1 - finSize * 0.5f);
    glVertex2f(finX - finSize * 0.3f, bodyY1);
    glEnd();
    
    // Left fin (center, pointing backward)
    glBegin(GL_TRIANGLES);
    glVertex2f(finX, 0.0f);
    glVertex2f(finX - finSize, -finSize * 0.3f);
    glVertex2f(finX - finSize, finSize * 0.3f);
    glEnd();
    
    // Outline fins
    setColor({0.0f, 0.0f, 0.0f});
    glLineWidth(1.0f);
    // Top fin outline
    glBegin(GL_LINE_LOOP);
    glVertex2f(finX, bodyY2);
    glVertex2f(finX - finSize, bodyY2 + finSize * 0.5f);
    glVertex2f(finX - finSize * 0.3f, bodyY2);
    glEnd();
    // Bottom fin outline
    glBegin(GL_LINE_LOOP);
    glVertex2f(finX, bodyY1);
    glVertex2f(finX - finSize, bodyY1 - finSize * 0.5f);
    glVertex2f(finX - finSize * 0.3f, bodyY1);
    glEnd();
    // Left fin outline
    glBegin(GL_LINE_LOOP);
    glVertex2f(finX, 0.0f);
    glVertex2f(finX - finSize, -finSize * 0.3f);
    glVertex2f(finX - finSize, finSize * 0.3f);
    glEnd();
    
    // Flame trail (behind rocket at left/back)
    float flameX = finX - finSize;
    
    // Outer flame (orange)
    drawCircle(flameX, 0.0f, 0.4f, FLAME_ORANGE, 20);
    // Inner flame (yellow)
    drawCircle(flameX, 0.0f, 0.25f, FLAME_YELLOW, 20);
    // Small center (bright yellow)
    drawCircle(flameX, 0.0f, 0.15f, {1.0f, 1.0f, 0.5f}, 15);
    
    glPopMatrix();
}

// -------------------------------------------------------------
// Drone Animation
// -------------------------------------------------------------
void calculateDronePosition(float t, bool direction, float& x, float& y) {
    // Start point: Above DIU roof
    float x1 = 22.0f;  // DIU X position
    // End point: Above City University roof
    float x2 = -22.0f;  // City X position
    
    // Fixed Y position: Above City University rooftop
    float fixedY = 12.0f;  // Above City roof top
    
    // Use t directly for forward direction, (1-t) for reverse
    float tParam = direction ? t : (1.0f - t);
    
    // Only interpolate X position (horizontal movement only)
    x = x1 + tParam * (x2 - x1);
    y = fixedY;  // Constant Y position above City University rooftop
}

void drawDrone(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    
    // Drone body (central square)
    float bodySize = 0.6f;
    float bodyHalf = bodySize * 0.5f;
    framedRect(-bodyHalf, -bodyHalf, bodyHalf, bodyHalf, DRONE_BODY, {0.0f, 0.0f, 0.0f}, 1.0f);
    
    // Four propellers (one at each corner)
    float propRadius = 0.25f;
    float propOffset = bodyHalf + 0.15f;
    
    // Top-left propeller
    drawCircle(-propOffset, propOffset, propRadius, DRONE_PROP, 12);
    // Top-right propeller
    drawCircle(propOffset, propOffset, propRadius, DRONE_PROP, 12);
    // Bottom-left propeller
    drawCircle(-propOffset, -propOffset, propRadius, DRONE_PROP, 12);
    // Bottom-right propeller
    drawCircle(propOffset, -propOffset, propRadius, DRONE_PROP, 12);
    
    // Propeller arms (thin lines connecting body to propellers)
    setColor({0.15f, 0.15f, 0.2f});
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    // Top-left arm
    glVertex2f(-bodyHalf, bodyHalf);
    glVertex2f(-propOffset, propOffset);
    // Top-right arm
    glVertex2f(bodyHalf, bodyHalf);
    glVertex2f(propOffset, propOffset);
    // Bottom-left arm
    glVertex2f(-bodyHalf, -bodyHalf);
    glVertex2f(-propOffset, -propOffset);
    // Bottom-right arm
    glVertex2f(bodyHalf, -bodyHalf);
    glVertex2f(propOffset, -propOffset);
    glEnd();
    
    // LED light (small circle in center)
    drawCircle(0.0f, 0.0f, 0.08f, DRONE_LIGHT, 10);
    
    glPopMatrix();
}

// -------------------------------------------------------------
// Destruction Effects
// -------------------------------------------------------------
struct DebrisParticle {
    float x, y;
    float vx, vy;  // velocity
    float life;     // 0.0 to 1.0, decreases over time
    float size;
};

const int MAX_DEBRIS = 30;
DebrisParticle debris[MAX_DEBRIS];
bool debrisInitialized = false;

void initializeDebris(float impactX, float impactY) {
    for (int i = 0; i < MAX_DEBRIS; ++i) {
        debris[i].x = impactX + (rand() % 100 - 50) * 0.1f;
        debris[i].y = impactY + (rand() % 100 - 50) * 0.1f;
        debris[i].vx = (rand() % 200 - 100) * 0.01f;
        debris[i].vy = (rand() % 100) * 0.02f + 0.1f;  // upward and random
        debris[i].life = 1.0f;
        debris[i].size = 0.2f + (rand() % 50) * 0.01f;
    }
    debrisInitialized = true;
}

void updateDebris() {
    if (!debrisInitialized) return;
    
    float timeScale = deltaTime / 0.0167f;  // Normalize to 60 FPS
    for (int i = 0; i < MAX_DEBRIS; ++i) {
        if (debris[i].life > 0.0f) {
            debris[i].x += debris[i].vx * timeScale;
            debris[i].y += debris[i].vy * timeScale;
            debris[i].vy -= 0.01f * timeScale;  // gravity (frame-rate independent)
            debris[i].life -= 0.01f * timeScale;  // fade out (frame-rate independent)
            if (debris[i].life < 0.0f) debris[i].life = 0.0f;
        }
    }
}

void drawDebris() {
    if (!debrisInitialized) return;
    
    for (int i = 0; i < MAX_DEBRIS; ++i) {
        if (debris[i].life > 0.0f) {
            float alpha = debris[i].life;
            Color c = {DEBRIS_COLOR.r * alpha, DEBRIS_COLOR.g * alpha, DEBRIS_COLOR.b * alpha};
            filledRect(debris[i].x - debris[i].size, debris[i].y - debris[i].size,
                      debris[i].x + debris[i].size, debris[i].y + debris[i].size, c);
        }
    }
}

void drawExplosion(float x, float y, float time) {
    if (time > 2.0f) return;  // Explosion lasts 2 seconds
    
    float maxRadius = 3.0f * time;  // Expand over time
    float fade = 1.0f - (time / 2.0f);  // Fade out
    
    // Outer explosion (orange)
    if (fade > 0.0f) {
        Color outer = {EXPLOSION_ORANGE.r * fade, EXPLOSION_ORANGE.g * fade, EXPLOSION_ORANGE.b * fade};
        drawCircle(x, y, maxRadius, outer, 30);
    }
    
    // Inner explosion (yellow)
    if (fade > 0.3f) {
        float innerRadius = maxRadius * 0.6f;
        Color inner = {EXPLOSION_YELLOW.r * fade, EXPLOSION_YELLOW.g * fade, EXPLOSION_YELLOW.b * fade};
        drawCircle(x, y, innerRadius, inner, 20);
    }
    
    // Core (bright white/yellow)
    if (fade > 0.5f) {
        float coreRadius = maxRadius * 0.3f;
        Color core = {1.0f * fade, 1.0f * fade, 0.5f * fade};
        drawCircle(x, y, coreRadius, core, 15);
    }
}

void drawFire(float x, float y, float baseWidth, float height, float flicker) {
    // Flicker effect using time-based offset
    float flickerOffset = sinf(flicker * 10.0f) * 0.1f;
    
    // Base of fire (wider)
    setColor(FIRE_RED);
    glBegin(GL_TRIANGLES);
    glVertex2f(x, y);  // base center
    glVertex2f(x - baseWidth * 0.5f, y);
    glVertex2f(x - baseWidth * 0.3f + flickerOffset, y + height * 0.3f);
    glEnd();
    
    glBegin(GL_TRIANGLES);
    glVertex2f(x, y);
    glVertex2f(x + baseWidth * 0.5f, y);
    glVertex2f(x + baseWidth * 0.3f - flickerOffset, y + height * 0.3f);
    glEnd();
    
    // Middle flame (orange)
    setColor(FIRE_ORANGE);
    glBegin(GL_TRIANGLES);
    glVertex2f(x - baseWidth * 0.2f + flickerOffset * 0.5f, y + height * 0.3f);
    glVertex2f(x + baseWidth * 0.2f - flickerOffset * 0.5f, y + height * 0.3f);
    glVertex2f(x + flickerOffset, y + height * 0.7f);
    glEnd();
    
    // Top flame (yellow)
    setColor(FIRE_YELLOW);
    glBegin(GL_TRIANGLES);
    glVertex2f(x - baseWidth * 0.15f, y + height * 0.7f);
    glVertex2f(x + baseWidth * 0.15f, y + height * 0.7f);
    glVertex2f(x + flickerOffset * 0.5f, y + height);
    glEnd();
}

void drawSmoke(float x, float y, float time, float baseRadius) {
    // Multiple smoke puffs rising
    for (int i = 0; i < 3; ++i) {
        float offsetY = y + time * 2.0f + i * 0.5f;
        float offsetX = x + sinf(time * 2.0f + i) * 0.3f;
        float radius = baseRadius * (1.0f + time * 0.5f + i * 0.2f);
        float alpha = 1.0f - (time * 0.3f + i * 0.1f);
        if (alpha < 0.0f) alpha = 0.0f;
        
        Color smoke = {SMOKE_DARK.r * alpha, SMOKE_DARK.g * alpha, SMOKE_DARK.b * alpha};
        drawCircle(offsetX, offsetY, radius, smoke, 20);
    }
}

// -------------------------------------------------------------
// Firecracker Functions
// -------------------------------------------------------------
void initializeFirecrackers() {
    // Launch point: DIU rooftop
    float launchX = 22.0f;
    float launchY = 16.8f;
    
    // Determine number of firecrackers (5-7)
    int numFirecrackers = 5 + (rand() % 3);  // Random between 5 and 7
    
    for (int i = 0; i < numFirecrackers && i < MAX_FIRECRACKERS; ++i) {
        firecrackers[i].x = launchX;
        firecrackers[i].y = launchY;
        firecrackers[i].travelDistance = 0.0f;
        firecrackers[i].exploded = false;
        firecrackers[i].explosionTime = 0.0f;
        
        // Randomly assign trajectory type
        firecrackers[i].trajectoryType = rand() % 3;  // 0, 1, or 2
        
        // Set velocity based on trajectory type
        if (firecrackers[i].trajectoryType == 0) {
            // Type 0: Straight up - much higher velocity
            firecrackers[i].vx = 0.0f;
            firecrackers[i].vy = 0.25f + (rand() % 10) * 0.01f;  // 0.25 to 0.34 (much higher)
            firecrackers[i].launchAngle = 90.0f;
        } else if (firecrackers[i].trajectoryType == 1) {
            // Type 1: -x then up (left and up) - increased velocities
            firecrackers[i].vx = -(0.08f + (rand() % 6) * 0.01f);  // -0.08 to -0.13
            firecrackers[i].vy = 0.22f + (rand() % 8) * 0.01f;     // 0.22 to 0.29 (much higher)
            firecrackers[i].launchAngle = 135.0f;  // Up-left
        } else {
            // Type 2: +x then up (right and up) - increased velocities
            firecrackers[i].vx = 0.08f + (rand() % 6) * 0.01f;   // 0.08 to 0.13
            firecrackers[i].vy = 0.22f + (rand() % 8) * 0.01f;   // 0.22 to 0.29 (much higher)
            firecrackers[i].launchAngle = 45.0f;  // Up-right
        }
        
        // Set max distance before explosion (random between 8.0 and 12.0 - much higher)
        firecrackers[i].maxDistance = 8.0f + (rand() % 41) * 0.1f;  // 8.0 to 12.0 (much higher)
        
        // Assign random color (red, yellow, blue, green)
        int colorType = rand() % 4;  // 0=red, 1=yellow, 2=blue, 3=green
        if (colorType == 0) {
            firecrackers[i].color = FIRECRACKER_RED;
        } else if (colorType == 1) {
            firecrackers[i].color = FIRECRACKER_YELLOW;
        } else if (colorType == 2) {
            firecrackers[i].color = FIRECRACKER_BLUE;
        } else {
            firecrackers[i].color = FIRECRACKER_GREEN;
        }
    }
    
    // Mark remaining firecrackers as inactive
    for (int i = numFirecrackers; i < MAX_FIRECRACKERS; ++i) {
        firecrackers[i].exploded = true;
        firecrackers[i].explosionTime = 2.1f;  // Mark as finished
    }
}

bool areAllFirecrackersFinished() {
    // Check if all firecrackers have finished their explosion animations
    for (int i = 0; i < MAX_FIRECRACKERS; ++i) {
        // If firecracker hasn't exploded yet, or explosion is still animating, not finished
        if (!firecrackers[i].exploded || firecrackers[i].explosionTime < 2.0f) {
            return false;
        }
    }
    return true;
}

void updateFirecrackers() {
    if (!firecrackersActive) return;
    
    // Check if all firecrackers have finished - if so, launch a new batch
    if (areAllFirecrackersFinished()) {
        initializeFirecrackers();  // Launch new batch
    }
    
    float timeScale = deltaTime / 0.0167f;  // Normalize to 60 FPS (defined once for all firecrackers)
    
    for (int i = 0; i < MAX_FIRECRACKERS; ++i) {
        if (!firecrackers[i].exploded) {
            // Update position (frame-rate independent)
            firecrackers[i].x += firecrackers[i].vx * timeScale;
            firecrackers[i].y += firecrackers[i].vy * timeScale;
            
            // Apply gravity (reduced downward acceleration for higher flight)
            firecrackers[i].vy -= 0.0008f * timeScale;  // Slightly reduced gravity
            
            // Update travel distance (frame-rate independent)
            float dx = firecrackers[i].vx * timeScale;
            float dy = firecrackers[i].vy * timeScale;
            firecrackers[i].travelDistance += sqrtf(dx * dx + dy * dy);
            
            // Check if firecracker should explode
            if (firecrackers[i].travelDistance >= firecrackers[i].maxDistance) {
                firecrackers[i].exploded = true;
                firecrackers[i].explosionTime = 0.0f;
            }
        } else {
            // Update explosion animation (frame-rate independent)
            if (firecrackers[i].explosionTime < 2.0f) {
                firecrackers[i].explosionTime += explosionSpeed * (deltaTime / 0.0167f);
            }
        }
    }
}

void drawFirecracker(float x, float y, const Color& color) {
    // Draw small firecracker body with random color
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    
    // Body with assigned color
    filledRect(-0.08f, -0.15f, 0.08f, 0.15f, color);
    outlineRect(-0.08f, -0.15f, 0.08f, 0.15f, {0.0f, 0.0f, 0.0f}, 1.0f);
    
    // Small spark trail (behind) - use lighter version of the color
    Color sparkColor = {color.r * 1.2f, color.g * 1.2f, color.b * 1.2f};
    if (sparkColor.r > 1.0f) sparkColor.r = 1.0f;
    if (sparkColor.g > 1.0f) sparkColor.g = 1.0f;
    if (sparkColor.b > 1.0f) sparkColor.b = 1.0f;
    drawCircle(-0.12f, 0.0f, 0.03f, sparkColor, 8);
    
    glPopMatrix();
}

void drawFirecrackerExplosion(float x, float y, float time, const Color& color) {
    if (time > 2.0f) return;  // Explosion lasts 2 seconds
    
    float maxRadius = 2.0f * time;  // Expand over time (smaller than building explosion)
    float fade = 1.0f - (time / 2.0f);  // Fade out
    
    if (fade <= 0.0f) return;
    
    // Outer explosion (using firecracker color) - largest
    Color outerColor = {color.r * fade, color.g * fade, color.b * fade};
    drawCircle(x, y, maxRadius, outerColor, 30);
    
    // Middle explosion (lighter version of color)
    if (fade > 0.2f) {
        float midRadius = maxRadius * 0.7f;
        Color midColor = {color.r * 1.3f * fade, color.g * 1.3f * fade, color.b * 1.3f * fade};
        if (midColor.r > 1.0f) midColor.r = 1.0f;
        if (midColor.g > 1.0f) midColor.g = 1.0f;
        if (midColor.b > 1.0f) midColor.b = 1.0f;
        drawCircle(x, y, midRadius, midColor, 25);
    }
    
    // Inner explosion (even lighter/brighter version)
    if (fade > 0.4f) {
        float innerRadius = maxRadius * 0.5f;
        Color innerColor = {color.r * 1.5f * fade, color.g * 1.5f * fade, color.b * 1.5f * fade};
        if (innerColor.r > 1.0f) innerColor.r = 1.0f;
        if (innerColor.g > 1.0f) innerColor.g = 1.0f;
        if (innerColor.b > 1.0f) innerColor.b = 1.0f;
        drawCircle(x, y, innerRadius, innerColor, 20);
    }
    
    // Core (white/bright version)
    if (fade > 0.6f) {
        float coreRadius = maxRadius * 0.3f;
        Color core = {1.0f * fade, 1.0f * fade, 1.0f * fade};
        drawCircle(x, y, coreRadius, core, 15);
    }
    
    // Spark particles radiating outward (8 directions) - using color
    int numSparks = 8;
    float sparkRadius = 0.08f * fade;
    Color sparkColor = {color.r * 1.2f * fade, color.g * 1.2f * fade, color.b * 1.2f * fade};
    if (sparkColor.r > 1.0f) sparkColor.r = 1.0f;
    if (sparkColor.g > 1.0f) sparkColor.g = 1.0f;
    if (sparkColor.b > 1.0f) sparkColor.b = 1.0f;
    for (int i = 0; i < numSparks; ++i) {
        float angle = (i * 2.0f * 3.14159265358979323846f) / numSparks;
        float sparkDist = maxRadius * 0.6f;
        float sparkX = x + cosf(angle) * sparkDist;
        float sparkY = y + sinf(angle) * sparkDist;
        drawCircle(sparkX, sparkY, sparkRadius, sparkColor, 8);
    }
}

// -------------------------------------------------------------
// People Animation Functions
// -------------------------------------------------------------
void initializePeople() {
    numPeople = 30;  // Fixed 30 people
    float startX = 22.0f;  // DIU position
    float roadY = 1.8f;    // Road Y position
    
    // Calculate spacing - spread people out in a line from DIU
    // People will be positioned from DIU (x=22) going backwards
    float spacing = 1.2f;  // Distance between people (spacing for visibility)
    
    for (int i = 0; i < numPeople; ++i) {
        // Position people with spacing, starting from DIU going backwards
        // First person at DIU (x=22), others spread behind
        people[i].x = startX - (i * spacing);
        // Add slight Y variation for more natural look
        people[i].y = roadY + 0.3f + (rand() % 5) * 0.1f;  // On road, slight variation
        people[i].walkCycle = (rand() % 100) / 100.0f;  // Random starting walk cycle
        people[i].walkingToCity = true;
        people[i].walkingToDIU = false;
        people[i].stopped = false;
        people[i].celebrating = false;
        people[i].celebrationCycle = (rand() % 100) / 100.0f;  // Random starting celebration cycle
        
        // Calculate speed so the furthest person reaches City University in 10 seconds
        // Furthest person starts at: startX - ((numPeople-1) * spacing)
        // Needs to reach: -22.0f
        // Distance to travel: (startX - ((numPeople-1) * spacing)) - (-22.0f)
        float furthestStartX = startX - ((numPeople - 1) * spacing);
        float distanceToTravel = furthestStartX - (-22.0f);
        float requiredSpeed = distanceToTravel / 10.0f;  // Reach in 10 seconds
        
        // Use slightly faster speed to ensure they all arrive in time
        people[i].speed = requiredSpeed * 1.1f;  // 10% faster to ensure arrival
        
        // Random shirt colors
        int colorType = rand() % 4;
        if (colorType == 0) {
            people[i].shirtColor = {0.8f, 0.2f, 0.2f};  // Red
        } else if (colorType == 1) {
            people[i].shirtColor = {0.2f, 0.2f, 0.8f};  // Blue
        } else if (colorType == 2) {
            people[i].shirtColor = {0.2f, 0.8f, 0.2f};  // Green
        } else {
            people[i].shirtColor = {0.8f, 0.8f, 0.2f};  // Yellow
        }
    }
    peopleInitialized = true;
}

void updatePeople() {
    if (!peopleInitialized) {
        initializePeople();
    }
    
    // Use global deltaTime (calculated in idle() function) instead of hardcoded value
    
    for (int i = 0; i < numPeople; ++i) {
        if (people[i].walkingToCity) {
            // Walk towards City University (from x=22 to x=-22)
            people[i].x -= people[i].speed * deltaTime;
            people[i].walkCycle += 0.1f * (deltaTime / 0.0167f);  // Walking animation speed (frame-rate independent)
            if (people[i].walkCycle > 1.0f) people[i].walkCycle -= 1.0f;
            
            // Check if reached City University
            if (people[i].x <= -22.0f) {
                people[i].x = -22.0f;
                people[i].walkingToCity = false;
                people[i].walkingToDIU = true;
            }
        } else if (people[i].walkingToDIU) {
            // Walk back to DIU (from x=-22 to x=22)
            people[i].x += people[i].speed * deltaTime;
            people[i].walkCycle += 0.1f * (deltaTime / 0.0167f);  // Frame-rate independent
            if (people[i].walkCycle > 1.0f) people[i].walkCycle -= 1.0f;
            
            // Check if reached DIU area
            if (people[i].x >= 22.0f) {
                // Assign random position in front of DIU (spread out)
                // Random X position between 18.0 and 26.0 (around DIU at x=22)
                people[i].x = 18.0f + (rand() % 81) * 0.1f;  // Random between 18.0 and 26.0
                // Random Y position variation
                people[i].y = 1.8f + 0.3f + (rand() % 8) * 0.1f;  // Random Y variation
                people[i].walkingToDIU = false;
                people[i].stopped = true;
                people[i].celebrating = true;  // Start celebrating
            }
        }
        // If stopped and celebrating, update celebration animation
        if (people[i].stopped && people[i].celebrating) {
            people[i].celebrationCycle += 0.08f * (deltaTime / 0.0167f);  // Celebration animation speed (frame-rate independent)
            if (people[i].celebrationCycle > 1.0f) people[i].celebrationCycle -= 1.0f;
        }
    }
}

void drawPerson(float x, float y, float walkCycle, const Color& shirtColor, bool isWalking, bool isCelebrating = false, float celebrationCycle = 0.0f) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    
    // Head (circle)
    drawCircle(0.0f, 0.4f, 0.15f, {0.9f, 0.8f, 0.7f}, 10);  // Skin color
    
    // Body (shirt)
    filledRect(-0.1f, 0.1f, 0.1f, 0.4f, shirtColor);
    
    // Legs (animated if walking, static if celebrating)
    float legOffset = isWalking ? sinf(walkCycle * 2.0f * 3.14159265358979323846f) * 0.1f : 0.0f;
    setColor({0.2f, 0.2f, 0.2f});  // Dark pants
    glLineWidth(2.5f);
    glBegin(GL_LINES);
    // Left leg
    glVertex2f(-0.05f, 0.1f);
    glVertex2f(-0.05f + legOffset, -0.2f);
    // Right leg
    glVertex2f(0.05f, 0.1f);
    glVertex2f(0.05f - legOffset, -0.2f);
    glEnd();
    
    // Arms - different behavior for walking vs celebrating
    if (isCelebrating) {
        // Celebration: raise and lower hands (up and down motion)
        // Use sine wave: 0.0 = down, 0.5 = up, 1.0 = down
        float armHeight = sinf(celebrationCycle * 2.0f * 3.14159265358979323846f) * 0.3f;  // Raise up to 0.3 units
        float armY = 0.3f + armHeight;  // Base Y position + raise amount
        glBegin(GL_LINES);
        // Left arm (raised up)
        glVertex2f(-0.1f, 0.3f);
        glVertex2f(-0.15f, armY);
        // Right arm (raised up)
        glVertex2f(0.1f, 0.3f);
        glVertex2f(0.15f, armY);
        glEnd();
    } else if (isWalking) {
        // Walking: swinging arms
        float armOffset = sinf(walkCycle * 2.0f * 3.14159265358979323846f) * 0.15f;
        glBegin(GL_LINES);
        // Left arm
        glVertex2f(-0.1f, 0.3f);
        glVertex2f(-0.15f - armOffset, 0.2f);
        // Right arm
        glVertex2f(0.1f, 0.3f);
        glVertex2f(0.15f + armOffset, 0.2f);
        glEnd();
    } else {
        // Standing: arms at sides
        glBegin(GL_LINES);
        // Left arm
        glVertex2f(-0.1f, 0.3f);
        glVertex2f(-0.15f, 0.2f);
        // Right arm
        glVertex2f(0.1f, 0.3f);
        glVertex2f(0.15f, 0.2f);
        glEnd();
    }
    glLineWidth(1.0f);
    
    glPopMatrix();
}

void drawPeople() {
    for (int i = 0; i < numPeople; ++i) {
        bool isWalking = people[i].walkingToCity || people[i].walkingToDIU;
        bool isCelebrating = people[i].stopped && people[i].celebrating;
        drawPerson(people[i].x, people[i].y, people[i].walkCycle, people[i].shirtColor, isWalking, isCelebrating, people[i].celebrationCycle);
    }
}

// -------------------------------------------------------------
// CITY UNIVERSITY (left side)
// -------------------------------------------------------------
namespace City {
    const Color SKY_BG = { 0.90f, 0.96f, 1.00f };
    const Color GROUND = { 0.26f, 0.66f, 0.35f };
    const Color GROUND_EDGE = { 0.05f, 0.25f, 0.09f };

    const Color BUILDING = { 0.98f, 0.98f, 0.98f };
    const Color OUTLINE = { 0.00f, 0.00f, 0.00f };

    const Color WINDOW_FILL = { 0.97f, 0.42f, 0.33f };
    const Color WINDOW_BORDER = { 0.10f, 0.10f, 0.10f };

    const Color DOOR_FILL = { 0.16f, 0.16f, 0.16f };
    const Color DOOR_PANEL = { 0.98f, 0.98f, 0.98f };

    const Color SIGN_BG = { 0.00f, 0.00f, 0.00f };
    const Color SIGN_TEXT = { 1.00f, 1.00f, 1.00f };

    const Color BUSH_DARK = { 0.03f, 0.55f, 0.15f };
    const Color BUSH_LIGHT = { 0.14f, 0.82f, 0.27f };

    void drawBush(float cx, float cy, float width, float height) {
        float hw = width * 0.5f;
        float baseH = height * 0.45f;

        // base
        filledRect(cx - hw, cy, cx + hw, cy + baseH, BUSH_DARK);

        // rounded top
        setColor(BUSH_LIGHT);
        glBegin(GL_POLYGON);
        glVertex2f(cx - hw, cy + baseH);
        glVertex2f(cx - hw * 0.7f, cy + height * 0.9f);
        glVertex2f(cx, cy + height);
        glVertex2f(cx + hw * 0.7f, cy + height * 0.9f);
        glVertex2f(cx + hw, cy + baseH);
        glEnd();

        outlinedRect(cx - hw, cy, cx + hw, cy + height, OUTLINE, 2.0f);
    }

    void drawSideBlock(int side) {
        float x1 = -9.0f;
        float x2 = -3.0f;
        if (side > 0) { // right block
            float tx1 = -x2, tx2 = -x1;
            x1 = tx1; x2 = tx2;
        }
        float y1 = 1.5f;
        float y2 = 9.0f;

        framedRect(x1, y1, x2, y2, BUILDING, OUTLINE, 3.0f);

        const int cols = 3;
        const int rows = 5;
        float marginX = 0.6f;
        float marginY = 0.7f;
        float winW = 1.2f;
        float winH = 1.0f;
        float gapX = ((x2 - x1) - 2.0f * marginX - cols * winW) / (cols - 1);
        float gapY = ((y2 - y1) - 2.0f * marginY - rows * winH) / (rows - 1);

        for (int r = 0; r < rows; ++r) {
            float wy1 = y1 + marginY + r * (winH + gapY);
            float wy2 = wy1 + winH;
            for (int c = 0; c < cols; ++c) {
                float wx1 = x1 + marginX + c * (winW + gapX);
                float wx2 = wx1 + winW;
                framedRect(wx1, wy1, wx2, wy2, WINDOW_FILL, WINDOW_BORDER, 2.0f);
            }
        }
    }

    void drawCenterBlock() {
        float x1 = -3.0f, x2 = 3.0f;
        float y1 = 1.5f, y2 = 9.5f;

        framedRect(x1, y1, x2, y2, BUILDING, OUTLINE, 3.0f);

        // Entrance steps
        filledRect(x1 - 0.7f, 1.5f, x2 + 0.7f, 2.0f, OUTLINE);

        // Dark door region
        float dX1 = -1.3f, dX2 = 1.3f;
        float dY1 = 2.0f, dY2 = 3.2f;
        framedRect(dX1, dY1, dX2, dY2, DOOR_FILL, OUTLINE, 3.0f);

        // Two tall door panels
        float panelW = 0.9f, panelH = 0.9f;
        float px1 = dX1 + 0.25f;
        float px2 = px1 + panelW;
        float py1 = dY1 + 0.4f;
        float py2 = py1 + panelH;
        framedRect(px1, py1, px2, py2, DOOR_PANEL, OUTLINE, 2.0f);

        float px3 = dX2 - 0.25f - panelW;
        float px4 = px3 + panelW;
        framedRect(px3, py1, px4, py2, DOOR_PANEL, OUTLINE, 2.0f);

        // Small square panel above door
        float w0x1 = -0.6f, w0x2 = 0.6f;
        float w0y1 = 3.4f, w0y2 = 4.2f;
        framedRect(w0x1, w0y1, w0x2, w0y2, WINDOW_FILL, WINDOW_BORDER, 2.0f);

        // Floors above: 4 rows of windows, 2 columns each
        const int rows = 4;
        const int cols = 2;
        float marginX = 0.6f;
        float marginY = 4.7f;
        float winW = 1.0f;
        float winH = 1.0f;
        float gapX = ((x2 - x1) - 2.0f * marginX - cols * winW) / (cols - 1);
        float totalHeight = (y2 - marginY - 0.7f);
        float gapY = (totalHeight - rows * winH) / (rows - 1);

        for (int r = 0; r < rows; ++r) {
            float wy1 = marginY + r * (winH + gapY);
            float wy2 = wy1 + winH;
            for (int c = 0; c < cols; ++c) {
                float wx1 = x1 + marginX + c * (winW + gapX);
                float wx2 = wx1 + winW;
                framedRect(wx1, wy1, wx2, wy2, WINDOW_FILL, WINDOW_BORDER, 2.0f);
            }
        }

        // Thin horizontal floor lines
        setColor(OUTLINE);
        glLineWidth(1.5f);
        glBegin(GL_LINES);
        for (int i = 1; i <= 4; ++i) {
            float fy = 2.7f + i * 1.5f;
            if (fy < y2 - 0.7f) {
                glVertex2f(x1 + 0.2f, fy);
                glVertex2f(x2 - 0.2f, fy);
            }
        }
        glEnd();
        glLineWidth(1.0f);
    }

    void drawRoofAndSign() {
        float roofY1 = 9.5f, roofY2 = 10.0f;
        framedRect(-9.4f, roofY1, 9.4f, roofY2, BUILDING, OUTLINE, 3.0f);

        float pedY1 = 10.0f, pedY2 = 10.35f;
        framedRect(-3.5f, pedY1, 3.5f, pedY2, BUILDING, OUTLINE, 2.0f);

        float signY1 = 10.35f, signY2 = 10.9f;
        framedRect(-3.0f, signY1, 3.0f, signY2, SIGN_BG, OUTLINE, 2.0f);

        setColor(SIGN_TEXT);
        drawText("CITY UNIVERSITY", -2.35f, 10.55f, GLUT_BITMAP_HELVETICA_18);
    }

    void drawBushes() {
        drawBush(-8.5f, 1.0f, 2.0f, 1.1f);
        drawBush(-6.5f, 1.0f, 2.4f, 1.2f);
        drawBush(6.5f, 1.0f, 2.4f, 1.2f);
        drawBush(8.5f, 1.0f, 2.0f, 1.1f);
    }

    void drawDestroyedBuilding(float fireTime) {
        // Draw partially collapsed building with damage
        // Left side block - partially destroyed
        float x1 = -9.0f;
        float x2 = -3.0f;
        float y1 = 1.5f;
        float y2 = 7.0f;  // Reduced height (collapsed top)
        
        // Damaged left block
        Color damagedColor = {0.6f, 0.5f, 0.4f};  // Darker, damaged color
        framedRect(x1, y1, x2, y2, damagedColor, OUTLINE, 3.0f);
        
        // Cracks on left block
        setColor({0.2f, 0.2f, 0.2f});
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex2f(x1 + 1.0f, y1 + 2.0f);
        glVertex2f(x1 + 2.5f, y1 + 4.0f);
        glVertex2f(x1 + 3.5f, y1 + 1.5f);
        glVertex2f(x1 + 4.5f, y1 + 3.5f);
        glEnd();
        
        // Right side block - partially destroyed
        x1 = 3.0f;
        x2 = 9.0f;
        y2 = 6.5f;  // Reduced height
        
        framedRect(x1, y1, x2, y2, damagedColor, OUTLINE, 3.0f);
        
        // Cracks on right block
        glBegin(GL_LINES);
        glVertex2f(x1 + 1.0f, y1 + 2.5f);
        glVertex2f(x1 + 2.0f, y1 + 4.5f);
        glVertex2f(x1 + 4.0f, y1 + 1.8f);
        glVertex2f(x1 + 5.5f, y1 + 3.8f);
        glEnd();
        glLineWidth(1.0f);
        
        // Center block - heavily damaged
        x1 = -3.0f;
        x2 = 3.0f;
        y2 = 8.0f;  // Reduced height
        
        framedRect(x1, y1, x2, y2, damagedColor, OUTLINE, 3.0f);
        
        // Collapsed roof section
        float collapsedY = 8.5f;
        filledRect(-2.0f, collapsedY, 2.0f, collapsedY + 0.3f, {0.4f, 0.3f, 0.2f});
        
        // Fire on windows
        float windowFireY = 4.0f;
        drawFire(-6.0f, windowFireY, 0.8f, 1.2f, fireTime);
        drawFire(-1.5f, windowFireY, 0.8f, 1.2f, fireTime);
        drawFire(1.5f, windowFireY, 0.8f, 1.2f, fireTime);
        drawFire(6.0f, windowFireY, 0.8f, 1.2f, fireTime);
        
        // Fire on roof
        drawFire(0.0f, collapsedY, 1.2f, 1.5f, fireTime);
        
        // Smoke rising from building
        drawSmoke(-5.0f, collapsedY, fireTime * 0.5f, 0.4f);
        drawSmoke(0.0f, collapsedY, fireTime * 0.5f + 0.3f, 0.5f);
        drawSmoke(5.0f, collapsedY, fireTime * 0.5f + 0.6f, 0.4f);
    }

    void drawScene() {
        // Ground under city
        filledRect(-11.0f, 0.0f, 11.0f, 1.5f, GROUND);
        setColor(GROUND_EDGE);
        glLineWidth(3.0f);
        glBegin(GL_LINES);
        glVertex2f(-11.0f, 1.5f);
        glVertex2f(11.0f, 1.5f);
        glEnd();
        glLineWidth(1.0f);

        // Building - check if destroyed
        if (!cityBuildingDestroyed) {
            // Draw normal building
        drawSideBlock(-1);
        drawSideBlock(1);
        drawCenterBlock();
        drawRoofAndSign();
        } else {
            // Draw destroyed building with fire
            drawDestroyedBuilding(explosionTime);
        }
        
        drawBushes();
    }
} // namespace City

// -------------------------------------------------------------
// DIU (right side)
// -------------------------------------------------------------
namespace Diu {
    const Color SKY_BG = { 0.94f, 0.97f, 1.00f };
    const Color GROUND = { 0.40f, 0.80f, 0.40f };
    const Color GROUND_DARK = { 0.26f, 0.63f, 0.28f };

    const Color WALL = { 0.97f, 0.93f, 0.85f };
    const Color WALL_DARK = { 0.90f, 0.86f, 0.78f };
    const Color ROOF = { 0.98f, 0.95f, 0.89f };

    const Color WIN_BLUE = { 0.25f, 0.57f, 0.92f };
    const Color OUTLINE = { 0.05f, 0.05f, 0.07f };

    const Color PATH_COLOR = { 0.93f, 0.84f, 0.68f };
    const Color BUSH_DARK = { 0.04f, 0.50f, 0.16f };
    const Color BUSH_LIGHT = { 0.18f, 0.76f, 0.28f };

    const Color SIGN_BG = { 0.04f, 0.22f, 0.45f };
    const Color SIGN_TEXT = { 1.00f, 1.00f, 1.00f };

    void drawBush(float cx, float cy, float width, float height) {
        float hw = width * 0.5f;
        float baseH = height * 0.45f;

        filledRect(cx - hw, cy, cx + hw, cy + baseH, BUSH_DARK);

        setColor(BUSH_LIGHT);
        glBegin(GL_POLYGON);
        glVertex2f(cx - hw, cy + baseH);
        glVertex2f(cx - hw * 0.75f, cy + height * 0.9f);
        glVertex2f(cx, cy + height);
        glVertex2f(cx + hw * 0.75f, cy + height * 0.9f);
        glVertex2f(cx + hw, cy + baseH);
        glEnd();

        outlineRect(cx - hw, cy, cx + hw, cy + height, OUTLINE, 1.5f);
    }

    void drawWindowGrid(float x1, float y1, float x2, float y2,
        int rows, int cols, float marginX, float marginY) {
        float cellW = (x2 - x1 - 2 * marginX) / cols;
        float cellH = (y2 - y1 - 2 * marginY) / rows;

        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                float wx1 = x1 + marginX + c * cellW + cellW * 0.15f;
                float wx2 = wx1 + cellW * 0.7f;
                float wy1 = y1 + marginY + r * cellH + cellH * 0.15f;
                float wy2 = wy1 + cellH * 0.7f;
                framedRect(wx1, wy1, wx2, wy2, WIN_BLUE, OUTLINE, 1.3f);
            }
        }
    }

    void drawSideWing(bool leftSide) {
        float wingBottom = 2.0f;
        float wingTop = 14.0f;

        float x1 = leftSide ? -12.0f : 6.0f;
        float x2 = leftSide ? -6.0f : 12.0f;

        framedRect(x1, wingBottom, x2, wingTop, WALL, OUTLINE, 2.5f);

        filledRect(x1, wingBottom, x2, wingBottom + 0.25f, WALL_DARK);

        drawWindowGrid(x1, wingBottom, x2, wingTop, 10, 3, 0.7f, 0.5f);
    }

    void drawCenterBlock() {
        float bottom = 2.4f;
        float top = 14.5f;
        float left = -4.2f;
        float right = 4.2f;

        framedRect(left - 0.4f, bottom, right + 0.4f, top, WALL_DARK, OUTLINE, 2.5f);

        framedRect(left, bottom + 0.4f, right, top - 0.6f, WIN_BLUE, OUTLINE, 2.5f);

        // vertical highlights
        setColor({ 0.70f, 0.86f, 1.0f });
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex2f(left + (right - left) * 0.22f, bottom + 0.6f);
        glVertex2f(left + (right - left) * 0.22f, top - 0.8f);
        glVertex2f(left + (right - left) * 0.50f, bottom + 0.6f);
        glVertex2f(left + (right - left) * 0.50f, top - 0.8f);
        glVertex2f(left + (right - left) * 0.78f, bottom + 0.6f);
        glVertex2f(left + (right - left) * 0.78f, top - 0.8f);
        glEnd();
        glLineWidth(1.0f);

        // beige floor bands
        for (int i = 0; i < 6; ++i) {
            float y1 = bottom + 2.0f + i * 1.5f;
            float y2 = y1 + 0.35f;
            if (y2 < top - 0.8f) filledRect(left, y1, right, y2, WALL);
        }

        // side pillars
        framedRect(left - 1.6f, bottom - 0.2f, left, top + 0.2f, WALL, OUTLINE, 2.5f);
        framedRect(right, bottom - 0.2f, right + 1.6f, top + 0.2f, WALL, OUTLINE, 2.5f);

        // entrance + doors
        float doorBaseY = 2.0f;
        framedRect(-2.0f, doorBaseY, 2.0f, bottom + 0.6f, WALL, OUTLINE, 2.0f);
        framedRect(-1.1f, doorBaseY + 0.5f, -0.1f, bottom + 0.45f, WIN_BLUE, OUTLINE, 1.5f);
        framedRect(0.1f, doorBaseY + 0.5f, 1.1f, bottom + 0.45f, WIN_BLUE, OUTLINE, 1.5f);

        // step + path
        filledRect(-2.4f, 1.7f, 2.4f, 2.0f, PATH_COLOR);
        outlineRect(-2.4f, 1.7f, 2.4f, 2.0f, OUTLINE, 1.5f);
        filledRect(-1.3f, 1.0f, 1.3f, 1.7f, PATH_COLOR);
        outlineRect(-1.3f, 1.0f, 1.3f, 1.7f, OUTLINE, 1.5f);

        // roof
        float roofY1 = top + 0.2f, roofY2 = top + 0.8f;
        framedRect(-8.0f, roofY1, 8.0f, roofY2, ROOF, OUTLINE, 2.5f);

        // sign pedestal
        float pedY1 = roofY2, pedY2 = roofY2 + 0.6f;
        framedRect(-7.0f, pedY1, 7.0f, pedY2, ROOF, OUTLINE, 2.0f);

        // sign board
        float signY1 = pedY2, signY2 = pedY2 + 0.9f;
        framedRect(-6.5f, signY1, 6.5f, signY2, SIGN_BG, OUTLINE, 2.0f);

        // text
        setColor(SIGN_TEXT);
        drawCenteredText("Daffodil International University",
            0.0f, signY1 + 0.25f, 0.22f, GLUT_BITMAP_HELVETICA_18);
    }

    void drawGround() {
        filledRect(-13.5f, 0.0f, 13.5f, 2.0f, GROUND);
        filledRect(-11.5f, 2.0f, 11.5f, 2.6f, GROUND_DARK);

        drawBush(-10.0f, 2.0f, 2.0f, 0.9f);
        drawBush(-7.5f, 2.0f, 2.4f, 1.0f);
        drawBush(7.5f, 2.0f, 2.4f, 1.0f);
        drawBush(10.0f, 2.0f, 2.0f, 0.9f);
    }

    void drawScene() {
        drawGround();
        drawSideWing(true);
        drawSideWing(false);
        drawCenterBlock();
    }
} // namespace Diu

// -------------------------------------------------------------
// Global display / reshape / main
// -------------------------------------------------------------
void display() {
    // #region agent log
    {
        std::ostringstream json;
        json << "{\"winW\":" << winW << ",\"winH\":" << winH << "}";
        static int callCount = 0;
        if (callCount++ < 3) {  // Log only first few calls to avoid spam
            debugLog("project.cpp:565", "Display called", json.str(), "run2", "C");
        }
    }
    // #endregion
    
    // Night sky background
    glClearColor(NIGHT_SKY.r, NIGHT_SKY.g, NIGHT_SKY.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Full green ground at the bottom - covering entire bottom area
    const Color GREEN_GROUND = {0.26f, 0.66f, 0.35f};
    filledRect(-40.0f, 0.0f, 40.0f, 5.0f, GREEN_GROUND);  // Green ground covering full bottom area

    // Draw sky elements (moon, stars, clouds) before buildings
    drawMoon();
    drawStars();
    drawClouds();
    
    // Draw countdown timer (10 to 1) before rocket launch - disappears at 0
    if (countdownValue > 0 && !missileAttackActive) {
        float countdownY = viewportTopY * 0.6f;  // 60% up from bottom (center area)
        setColor({1.0f, 1.0f, 0.0f});  // Bright yellow color
        glLineWidth(4.0f);  // Thick line for visibility
        std::string countdownStr = std::to_string(countdownValue);
        drawLargeStrokeText(countdownStr, 0.0f, countdownY, 0.15f);  // Large, centered text
        glLineWidth(1.0f);  // Reset line width
    }
    
    // Draw road and environment elements
    drawRoad();
    drawTrees();
    drawStreetLights();

    // LEFT: City University, shifted left
    glPushMatrix();
    glTranslatef(-22.0f, 0.0f, 0.0f);      // center around x ≈ -22
    City::drawScene();
    glPopMatrix();

    // RIGHT: DIU, shifted right
    glPushMatrix();
    glTranslatef(22.0f, 0.0f, 0.0f);       // center around x ≈ +22
    Diu::drawScene();
    glPopMatrix();

    // Draw animated rocket (only if missile attacks are active - after 10 second delay)
    if (missileAttackActive) {
        float rocketX, rocketY, rocketAngle;
        calculateRocketPosition(rocketAnimTime, rocketX, rocketY, rocketAngle);
        drawRocket(rocketX, rocketY, rocketAngle);
    }

    // Draw animated drone (loops DIU ↔ City University)
    float droneX, droneY;
    calculateDronePosition(droneAnimTime, droneDirection, droneX, droneY);
    drawDrone(droneX, droneY);
    
    // Draw flags on buildings
    drawFlags();
    
    // Draw people walking on the road (after buildings so they appear in front)
    drawPeople();

    // Draw destruction effects (explosion, debris) if building is destroyed
    if (cityBuildingDestroyed) {
        float impactX = -22.0f;  // City University X position (world coordinates)
        float impactY = 10.9f;   // City University roof Y position
        
        // Draw explosion effect
        if (explosionTime < 2.0f) {
            drawExplosion(impactX, impactY, explosionTime);
        }
        
        // Draw debris particles
        drawDebris();
    }
    
    // Draw firecracker jubilation animation
    if (firecrackersActive) {
        for (int i = 0; i < MAX_FIRECRACKERS; ++i) {
            if (!firecrackers[i].exploded) {
                // Draw firecracker body with its assigned color
                drawFirecracker(firecrackers[i].x, firecrackers[i].y, firecrackers[i].color);
            } else if (firecrackers[i].explosionTime < 2.0f) {
                // Draw firecracker explosion with its assigned color
                drawFirecrackerExplosion(firecrackers[i].x, firecrackers[i].y, firecrackers[i].explosionTime, firecrackers[i].color);
            }
        }
    }
    
    // Draw victory message when building is destroyed
    if (cityBuildingDestroyed) {
        // Position in top middle area (around Y = 16-17, centered at X = 0)
        float textY = viewportTopY * 0.85f;  // 85% up from bottom (top middle area)
        float textX = 0.0f;  // Center horizontally
        
        // Draw victory text with bright colors (smaller size to fit screen)
        setColor(FIRECRACKER_YELLOW);  // Bright yellow/gold color
        glLineWidth(1.5f);  // Thinner line width
        drawLargeStrokeText("Happy Victory Day DIU", textX, textY, 0.035f);
        glLineWidth(1.0f);  // Reset line width
    }

    glFlush();
    glutSwapBuffers();
}

void reshape(int w, int h) {
    winW = w;
    winH = h;
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Big world: x [-40, 40], y [0, 20]
    float worldW = 80.0f;
    float worldH = 20.0f;
    float aspect = static_cast<float>(w) / static_cast<float>(h);

    // #region agent log
    {
        std::ostringstream json;
        json << "{\"w\":" << w << ",\"h\":" << h << ",\"aspect\":" << std::fixed << std::setprecision(3) << aspect << ",\"worldW\":" << worldW << ",\"worldH\":" << worldH << "}";
        debugLog("project.cpp:602", "Reshape: window dimensions and aspect", json.str(), "run2", "A");
    }
    // #endregion

    float topY = worldH;
    if (aspect >= worldW / worldH) {
        float halfW = 0.5f * worldH * aspect;
        glOrtho(-halfW, halfW, 0.0f, worldH, -1.0f, 1.0f);
        topY = worldH;
    }
    else {
        float halfH = 0.5f * worldW / aspect;
        topY = 2.0f * halfH;
        glOrtho(-0.5f * worldW, 0.5f * worldW,
            0.0f, topY, -1.0f, 1.0f);
    }
    
    // Update global viewport top Y for dynamic positioning
    viewportTopY = topY;

    // #region agent log
    {
        std::ostringstream json;
        json << "{\"topY\":" << std::fixed << std::setprecision(3) << topY << ",\"viewportTopY\":" << viewportTopY << ",\"branch\":\"" << (aspect >= worldW / worldH ? "if" : "else") << "\",\"halfH\":" << (aspect < worldW / worldH ? (0.5f * worldW / aspect) : 0.0f) << "}";
        debugLog("project.cpp:664", "Reshape: calculated top Y value", json.str(), "run3", "D");
    }
    // #endregion

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void keyboard(unsigned char key, int, int) {
    if (key == 27 || key == 'q' || key == 'Q') {
        exit(0);
    }
    
    // Restart animation with 'r' or 'R' key
    if (key == 'r' || key == 'R') {
        // Reset all animation states
        rocketAnimTime = 0.0f;
        missileHitCount = 0;
        missileAttackActive = false;
        firstMissileHitTime = -1.0f;  // Reset first missile hit time
        cityBuildingDestroyed = false;
        explosionTime = 0.0f;
        debrisInitialized = false;
        
        // Reset drone animation (optional - remove if you want drone to continue)
        // droneAnimTime = 0.0f;
        // droneDirection = true;
        
        // Reset firecracker jubilation
        firecrackersInitialized = false;
        firecrackersActive = false;
        firecrackerJubilationTime = 0.0f;
        buildingDestroyedTime = -1.0f;  // Reset building destruction time
        // Reset all firecracker structs
        for (int i = 0; i < MAX_FIRECRACKERS; ++i) {
            firecrackers[i].exploded = true;
            firecrackers[i].explosionTime = 2.1f;
        }
        
        // Reset people animation
        peopleInitialized = false;  // Will re-initialize on next update
        
        // Reset cloud animation
        cloudOffsetX = 0.0f;
        cloudYInitialized = false;  // Re-initialize with new random positions
        
        // Reset flag animation
        flagWaveTime = 0.0f;
        
        // Reset timer for 10 second delay
        timeInitialized = false;  // Reset timer so it re-initializes in idle()
        startTime = 0;
        lastFrameTime = 0;  // Reset frame timing
        deltaTime = 0.0167f;  // Reset to default
        countdownValue = 10;  // Reset countdown to 10
        
        // Reset sound flags
        rocketSoundPlayed = false;
        explosionSoundPlayed = false;
        firecrackerSoundPlayed = false;
        stopSound();  // Stop any currently playing sounds
        
        glutPostRedisplay();
    }
}

void idle() {
    // Calculate elapsed time since program start (in seconds)
    if (!timeInitialized) {
        startTime = glutGet(GLUT_ELAPSED_TIME);  // Get time in milliseconds
        lastFrameTime = startTime;  // Initialize last frame time
        timeInitialized = true;
    }
    
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    float elapsedTime = (currentTime - startTime) / 1000.0f;  // Convert to seconds
    
    // Calculate actual deltaTime based on frame rate (frame-rate independent)
    deltaTime = (currentTime - lastFrameTime) / 1000.0f;  // Actual time since last frame
    if (deltaTime > 0.1f) deltaTime = 0.0167f;  // Cap deltaTime to prevent huge jumps (max 0.1s = 10 FPS min)
    lastFrameTime = currentTime;  // Update last frame time
    
    // Calculate countdown value (10 to 0)
    float remainingTime = missileDelay - elapsedTime;
    if (remainingTime > 0.0f) {
        countdownValue = (int)ceil(remainingTime);
        if (countdownValue < 0) countdownValue = 0;
        if (countdownValue > 10) countdownValue = 10;
    } else {
        countdownValue = 0;
    }
    
    // Activate missile attacks after 10 second delay (for first missile)
    static bool justActivated = false;
    if (!missileAttackActive && elapsedTime >= missileDelay && missileHitCount < 2) {
        // Check if we need to wait for second missile delay
        if (missileHitCount == 0) {
            // First missile: launch immediately after 10 second delay
            missileAttackActive = true;
            rocketSoundPlayed = false;  // Reset flag so sound can play
            justActivated = true;  // Mark that we just activated
        } else if (missileHitCount == 1 && firstMissileHitTime >= 0.0f) {
            // Second missile: launch 4 seconds after first hit
            float timeSinceFirstHit = elapsedTime - firstMissileHitTime;
            if (timeSinceFirstHit >= secondMissileDelay) {
                missileAttackActive = true;
                rocketSoundPlayed = false;  // Reset flag so sound can play
                justActivated = true;  // Mark that we just activated
            }
        }
    }
    
    // Update rocket animation (one-way: DIU to City University)
    // Only update if missile attacks are active (after 10 second delay)
    float prevRocketTime = rocketAnimTime;
    
    if (missileAttackActive) {
        // Play rocket sound immediately when attack activates or when rocket resets
        if (missileAttackActive && !rocketSoundPlayed && (justActivated || rocketAnimTime < 0.05f)) {
            playSound("Rocket-launcher.wav");
            rocketSoundPlayed = true;
            justActivated = false;  // Clear the flag
        }
        
        rocketAnimTime += rocketSpeed * (deltaTime / 0.0167f);  // Scale by actual frame time (normalized to 60 FPS)
        
        // Detect rocket impact on City University
        if (rocketAnimTime >= 1.0f && prevRocketTime < 1.0f) {
            missileHitCount++;
            
            // First hit: no destruction, just count and record time
            if (missileHitCount == 1) {
                firstMissileHitTime = elapsedTime;  // Record time of first hit
                missileAttackActive = false;  // Stop launching until delay passes
                rocketAnimTime = 0.0f;  // Reset animation
                rocketSoundPlayed = false;  // Reset flag for next rocket sound
            }
            // Second hit: destroy the building
            else if (missileHitCount == 2 && !cityBuildingDestroyed) {
                // Rocket just reached target - trigger destruction
                cityBuildingDestroyed = true;
                explosionTime = 0.0f;
                float impactX = -22.0f;  // City University X position (world coordinates)
                float impactY = 10.9f;  // City University roof Y position
                initializeDebris(impactX, impactY);
                
                // Record the time when building was destroyed
                buildingDestroyedTime = elapsedTime;
                
                // Play explosion sound
                if (!explosionSoundPlayed) {
                    playSound("explosion.wav");
                    explosionSoundPlayed = true;
                }
                
                // After second hit, stop launching new missiles
                missileAttackActive = false;
            }
        }
    }
    
    // Update drone animation (loop: DIU ↔ City University)
    droneAnimTime += droneSpeed * (deltaTime / 0.0167f);  // Scale by actual frame time
    if (droneAnimTime >= 1.0f) {
        droneAnimTime = 0.0f;
        droneDirection = !droneDirection;  // Toggle direction for continuous loop
    }
    
    // Update cloud animation (move left to right, looping)
    cloudOffsetX += cloudSpeed * (deltaTime / 0.0167f);  // Scale by actual frame time
    if (cloudOffsetX >= cloudLoopWidth) {
        cloudOffsetX -= cloudLoopWidth;  // Loop back to start
    }
    
    // Update flag animation (waving effect)
    flagWaveTime += flagWaveSpeed * (deltaTime / 0.0167f);  // Scale by actual frame time
    if (flagWaveTime > 6.28f) {  // Reset after full wave cycle (2*PI)
        flagWaveTime -= 6.28f;
    }
    
    // Update people animation (walking from DIU to City University and back)
    updatePeople();
    
    // Update destruction effects
    if (cityBuildingDestroyed) {
        explosionTime += explosionSpeed * (deltaTime / 0.0167f);  // Scale by actual frame time
        updateDebris();
        
        // Start firecracker jubilation 10 seconds after building is destroyed
        if (missileHitCount == 2 && !firecrackersActive && buildingDestroyedTime >= 0.0f) {
            float timeSinceDestruction = elapsedTime - buildingDestroyedTime;
            if (timeSinceDestruction >= firecrackerDelay) {
                initializeFirecrackers();
                firecrackersInitialized = true;
                firecrackersActive = true;
                firecrackerJubilationTime = 0.0f;
                
                // Play firecracker celebration sound
                if (!firecrackerSoundPlayed) {
                    playSound("firecrackers.wav");
                    firecrackerSoundPlayed = true;
                }
            }
        }
    }
    
    // Update firecracker animation (continuous loop)
    if (firecrackersActive) {
        updateFirecrackers();  // This will automatically launch new batches when current ones finish
        firecrackerJubilationTime += deltaTime;  // Use actual frame time
    }
    
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    // Seed random number generator for debris effects
    srand(static_cast<unsigned int>(time(nullptr)));
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("DIU and City University Scene");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutIdleFunc(idle);

    glutMainLoop();
    return 0;
}


