#include <windows.h>
#include <objidl.h>
#include <GL/gl.h>
#include <mmsystem.h>
#include <gdiplus.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <queue>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE2 0x84C2
#define GL_TEXTURE3 0x84C3
#define GL_TEXTURE4 0x84C4
#define GL_TEXTURE5 0x84C5
#define GL_TEXTURE6 0x84C6
#define GL_TEXTURE7 0x84C7
#define GL_TEXTURE8 0x84C8
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#endif
#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER 0x812D
#define GL_TEXTURE_BORDER_COLOR 0x1004
#endif
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_DEPTH_COMPONENT24 0x81A6
#endif
#ifndef GL_NONE
#define GL_NONE 0
#endif
#ifndef GL_R8
#define GL_R8 0x8229
#define GL_RED 0x1903
#endif
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif

using GLchar = char;
using GLsizeiptr = std::ptrdiff_t;
using CreateShader = GLuint(APIENTRY*)(GLenum);
using ShaderSource = void(APIENTRY*)(GLuint, GLsizei, const GLchar* const*, const GLint*);
using CompileShader = void(APIENTRY*)(GLuint);
using GetShaderiv = void(APIENTRY*)(GLuint, GLenum, GLint*);
using GetShaderInfoLog = void(APIENTRY*)(GLuint, GLsizei, GLsizei*, GLchar*);
using DeleteShader = void(APIENTRY*)(GLuint);
using CreateProgram = GLuint(APIENTRY*)();
using AttachShader = void(APIENTRY*)(GLuint, GLuint);
using LinkProgram = void(APIENTRY*)(GLuint);
using GetProgramiv = void(APIENTRY*)(GLuint, GLenum, GLint*);
using GetProgramInfoLog = void(APIENTRY*)(GLuint, GLsizei, GLsizei*, GLchar*);
using UseProgram = void(APIENTRY*)(GLuint);
using DeleteProgram = void(APIENTRY*)(GLuint);
using GetUniformLocation = GLint(APIENTRY*)(GLuint, const GLchar*);
using UniformMatrix4fv = void(APIENTRY*)(GLint, GLsizei, GLboolean, const GLfloat*);
using Uniform3f = void(APIENTRY*)(GLint, GLfloat, GLfloat, GLfloat);
using Uniform1i = void(APIENTRY*)(GLint, GLint);
using Uniform1f = void(APIENTRY*)(GLint, GLfloat);
using ActiveTexture = void(APIENTRY*)(GLenum);
using GenerateMipmap = void(APIENTRY*)(GLenum);
using GenVertexArrays = void(APIENTRY*)(GLsizei, GLuint*);
using BindVertexArray = void(APIENTRY*)(GLuint);
using GenBuffers = void(APIENTRY*)(GLsizei, GLuint*);
using BindBuffer = void(APIENTRY*)(GLenum, GLuint);
using BufferData = void(APIENTRY*)(GLenum, GLsizeiptr, const void*, GLenum);
using EnableVertexAttribArray = void(APIENTRY*)(GLuint);
using VertexAttribPointer = void(APIENTRY*)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
using GenFramebuffers = void(APIENTRY*)(GLsizei, GLuint*);
using BindFramebuffer = void(APIENTRY*)(GLenum, GLuint);
using FramebufferTexture2D = void(APIENTRY*)(GLenum, GLenum, GLenum, GLuint, GLint);
using CheckFramebufferStatus = GLenum(APIENTRY*)(GLenum);

CreateShader glCreateShader;
ShaderSource glShaderSource;
CompileShader glCompileShader;
GetShaderiv glGetShaderiv;
GetShaderInfoLog glGetShaderInfoLog;
DeleteShader glDeleteShader;
CreateProgram glCreateProgram;
AttachShader glAttachShader;
LinkProgram glLinkProgram;
GetProgramiv glGetProgramiv;
GetProgramInfoLog glGetProgramInfoLog;
UseProgram glUseProgram;
DeleteProgram glDeleteProgram;
GetUniformLocation glGetUniformLocation;
UniformMatrix4fv glUniformMatrix4fv;
Uniform3f glUniform3f;
Uniform1i glUniform1i;
Uniform1f glUniform1f;
ActiveTexture glActiveTexture;
GenerateMipmap glGenerateMipmap;
GenVertexArrays glGenVertexArrays;
BindVertexArray glBindVertexArray;
GenBuffers glGenBuffers;
BindBuffer glBindBuffer;
BufferData glBufferData;
EnableVertexAttribArray glEnableVertexAttribArray;
VertexAttribPointer glVertexAttribPointer;
GenFramebuffers glGenFramebuffers;
BindFramebuffer glBindFramebuffer;
FramebufferTexture2D glFramebufferTexture2D;
CheckFramebufferStatus glCheckFramebufferStatus;

constexpr int MapSize = 560;
constexpr float Cell = 0.0625f;
constexpr float HalfMaze = MapSize * Cell * 0.5f;
constexpr float Pi = 3.14159265f;
constexpr float ExitAngle = 95.625f * Pi / 180.0f; // Center of the 32-sector outer ring.
constexpr int RingCount = 6;
constexpr float CenterRadius = 2.75f, RadialStep = 2.05f;
constexpr float WallHalfThickness = 0.10f, DoorHalfWidth = 0.90f;

int sectorsForRing(int ring) {
    // Doubling ring counts keeps each child doorway away from parent wall spokes.
    if (ring == 0) return 1;
    if (ring == 1) return 8;
    if (ring <= 3) return 16;
    return 32;
}

struct Vertex {
    float x, y, z;
    float nx, ny, nz;
    float r, g, b, emission;
    float u, v, material;
};
struct SkyVertex { float x, y, z, u, v; };
struct UiVertex { float x, y, r, g, b, a; };
struct Vec3 { float x, y, z; };
enum class RunOmen : uint8_t { VeiledMoon, FailingWick, DistantBells, HowlingWind, None };
enum class TraceKind : uint8_t { ExitClue, BatteryCache, Remnant };
struct TrailPoint { float x=0.0f, z=0.0f, yaw=0.0f; };
struct BatteryPickup { float x=0.0f, z=0.0f; bool collected=false; };
struct RuinTrace {
    float x=0.0f,z=0.0f,hintX=0.0f,hintZ=0.0f;
    TraceKind kind=TraceKind::Remnant;
    bool found=false;
    uint8_t note=0;
};
struct PastRun {
    std::string timestamp;
    float elapsed=0.0f;
    int routeUses=0;
    uint32_t seed=0;
    RunOmen omen=RunOmen::None;
};
struct ModelPart { GLsizei first = 0, count = 0; GLuint texture = 0; std::string material; };
struct ModelObject { GLuint vao = 0, vbo = 0; std::vector<ModelPart> parts; bool loaded = false; };
struct WallArc {
    int boundary;
    float startAngle, endAngle;
    bool capStart, capEnd;
    float holeCenterAngle = 0.0f, holeHalfWidth = 0.0f;
    float holeBottom = 0.0f, holeTop = 0.0f;
    float crumbleStartLength = 0.0f, crumbleStartHeight = 0.0f;
    float crumbleEndLength = 0.0f, crumbleEndHeight = 0.0f;
};
struct RadialWall { float angle, startRadius, endRadius; bool capStart, capEnd; };

struct SpatialAudio {
    static constexpr int SampleRate=44100, FramesPerBuffer=1024, BufferCount=4;
    struct Buffer { WAVEHDR header{}; std::array<int16_t,FramesPerBuffer*2> samples{}; };
    HWAVEOUT device=nullptr;
    std::array<Buffer,BufferCount> buffers{};
    std::atomic<bool> active{false},enabled{true};
    std::atomic<float> cuePan{0.0f},cueVolume{0.0f};
    std::atomic<float> windLevel{1.0f},bellPan{0.0f},bellLevel{0.0f};
    uint32_t noiseState=0x8f31a2c7u;
    float filteredNoise=0.0f;
    float bellEnvelope=0.0f,bellCountdown=18.0f;
    double cuePhase=0.0,windPhase=0.0,bellPhase=0.0;

    static void CALLBACK callback(HWAVEOUT,UINT message,DWORD_PTR instance,DWORD_PTR parameter,DWORD_PTR) {
        if (message!=WOM_DONE || !instance) return;
        auto* audio=reinterpret_cast<SpatialAudio*>(instance);
        if (audio->active.load(std::memory_order_relaxed))
            audio->submit(reinterpret_cast<WAVEHDR*>(parameter));
    }

    void fill(Buffer& buffer) {
        const bool play=enabled.load(std::memory_order_relaxed);
        const float pan=std::clamp(cuePan.load(std::memory_order_relaxed),-1.0f,1.0f);
        const float volume=std::clamp(cueVolume.load(std::memory_order_relaxed),0.0f,0.3f);
        const float wind=std::clamp(windLevel.load(std::memory_order_relaxed),0.0f,8.0f);
        const float bell=std::clamp(bellLevel.load(std::memory_order_relaxed),0.0f,0.15f);
        const float bellDirection=std::clamp(bellPan.load(std::memory_order_relaxed),-1.0f,1.0f);
        const float leftGain=std::sqrt((1.0f-pan)*0.5f),rightGain=std::sqrt((1.0f+pan)*0.5f);
        const float leftBellGain=std::sqrt((1.0f-bellDirection)*0.5f),rightBellGain=std::sqrt((1.0f+bellDirection)*0.5f);
        for (int frame=0;frame<FramesPerBuffer;++frame) {
            if (!play) { buffer.samples[frame*2]=buffer.samples[frame*2+1]=0; continue; }
            noiseState^=noiseState<<13; noiseState^=noiseState>>17; noiseState^=noiseState<<5;
            const float noise=static_cast<float>(noiseState&0xffffu)/32767.5f-1.0f;
            filteredNoise+=0.0035f*(noise-filteredNoise);
            const float windSample=filteredNoise*(0.0028f+0.0012f*std::sin(windPhase))*wind;
            const float swell=0.66f+0.34f*std::sin(cuePhase*0.19);
            const float cue=(std::sin(cuePhase)+0.24f*std::sin(cuePhase*1.5))*volume*swell;
            float bellSample=0.0f;
            if (bell>0.0f) {
                bellCountdown-=1.0f/SampleRate;
                if (bellCountdown<=0.0f) {
                    bellEnvelope=1.0f;
                    const float variation=static_cast<float>((noiseState>>8)&0xffffu)/65535.0f;
                    bellCountdown=30.0f+variation*28.0f;
                }
                bellSample=(std::sin(bellPhase)+0.32f*std::sin(bellPhase*2.76)+
                            0.14f*std::sin(bellPhase*4.23))*bellEnvelope*bell;
                bellEnvelope*=0.999985f;
                bellPhase+=2.0*Pi*587.33/SampleRate;
                if (bellPhase>2.0*Pi) bellPhase-=2.0*Pi;
            }
            const float left=std::clamp(windSample+cue*leftGain+bellSample*leftBellGain,-0.95f,0.95f);
            const float right=std::clamp(windSample+cue*rightGain+bellSample*rightBellGain,-0.95f,0.95f);
            buffer.samples[frame*2]=static_cast<int16_t>(left*32767.0f);
            buffer.samples[frame*2+1]=static_cast<int16_t>(right*32767.0f);
            cuePhase+=2.0*Pi*212.0/SampleRate;
            windPhase+=2.0*Pi*0.11/SampleRate;
            if (cuePhase>2.0*Pi) cuePhase-=2.0*Pi;
            if (windPhase>2.0*Pi) windPhase-=2.0*Pi;
        }
    }

    void submit(WAVEHDR* header) {
        if (!active.load(std::memory_order_relaxed) || !device || !header) return;
        auto* buffer=reinterpret_cast<Buffer*>(reinterpret_cast<char*>(header)-offsetof(Buffer,header));
        fill(*buffer);
        if (active.load(std::memory_order_relaxed)) waveOutWrite(device,header,sizeof(WAVEHDR));
    }

    bool start() {
        WAVEFORMATEX format{};
        format.wFormatTag=WAVE_FORMAT_PCM; format.nChannels=2; format.nSamplesPerSec=SampleRate;
        format.wBitsPerSample=16; format.nBlockAlign=4; format.nAvgBytesPerSec=SampleRate*format.nBlockAlign;
        if (waveOutOpen(&device,WAVE_MAPPER,&format,reinterpret_cast<DWORD_PTR>(&SpatialAudio::callback),
                        reinterpret_cast<DWORD_PTR>(this),CALLBACK_FUNCTION)!=MMSYSERR_NOERROR) return false;
        active.store(true,std::memory_order_relaxed);
        for (Buffer& buffer:buffers) {
            buffer.header.lpData=reinterpret_cast<LPSTR>(buffer.samples.data());
            buffer.header.dwBufferLength=static_cast<DWORD>(sizeof(buffer.samples));
            if (waveOutPrepareHeader(device,&buffer.header,sizeof(WAVEHDR))!=MMSYSERR_NOERROR) { stop(); return false; }
        }
        for (Buffer& buffer:buffers) submit(&buffer.header);
        return true;
    }

    void stop() {
        active.store(false,std::memory_order_relaxed);
        if (!device) return;
        waveOutReset(device);
        for (Buffer& buffer:buffers)
            if (buffer.header.dwFlags&WHDR_PREPARED) waveOutUnprepareHeader(device,&buffer.header,sizeof(WAVEHDR));
        waveOutClose(device);
        device=nullptr;
    }
};

struct Game {
    HWND window = nullptr;
    HDC dc = nullptr;
    HGLRC gl = nullptr;
    int width = 1280, height = 800;
    std::vector<std::string> map;
    std::vector<WallArc> wallArcs;
    std::vector<RadialWall> radialWalls;
    std::vector<BatteryPickup> batteryPickups;
    std::vector<RuinTrace> ruinTraces;
    std::vector<PastRun> pastRuns;
    std::vector<TrailPoint> previousTrail,currentTrail;
    float mazePhaseA = 0.0f, mazePhaseB = 0.0f;
    uint32_t mazeSeed=0, runSeed=0, requestedSeed=0, completedSeed=0, bestSeed=0, runtimeRandom=1;
    uint32_t previousTrailSeed=0;
    RunOmen runOmen=RunOmen::None;
    bool hasRequestedSeed=false, newRecord=false, flashlightOn=true, headBobEnabled=true;
    std::filesystem::path executableDirectory;
    std::filesystem::path settingsPath;
    std::filesystem::path pastRunsPath;
    std::filesystem::path trailPath;
    GLsizei worldVertexCount = 0;
    GLsizei skyVertexCount = 0;
    GLsizei routeVertexCount = 0;
    float routeVisible = 0.0f, routeCooldown = 0.0f;
    float stepDistance = 0.0f;
    float x = 0.0f, z = 0.0f;
    float yaw = ExitAngle, pitch = 0.0f;
    float elapsed = 0.0f, bestTime=0.0f, batteryCharge=100.0f;
    int batteryCount=0, routeUses=0;
    bool started = false, paused = false, won = false, mouseCaptured = false, running = true;
    bool showSettings = false, showPastRuns=false, soundEnabled = true;
    int pastRunsPage=0;
    std::string noticeMessage;
    float bobPhase = 0.0f, movementBob = 0.0f, mouseSensitivity=1.0f;
    float flickerTimer=0.0f, nextFlicker=18.0f, weatherTime=0.0f, cloudCoverage=0.05f;
    float noticeTimer=0.0f,trailProgress=0.0f;
    int settingsIndex = 0, resolutionIndex = 1, difficultyIndex=1;
    float brightness = 1.0f, contrast = 1.0f;
    bool alternateFootstep = false;
    SpatialAudio spatialAudio;
    GLuint worldProgram = 0, skyProgram = 0, uiProgram = 0;
    GLuint shadowProgram = 0, shadowFramebuffer = 0, shadowTexture = 0;
    GLuint worldVao = 0, worldVbo = 0, routeVao = 0, routeVbo = 0;
    GLuint skyVao = 0, skyVbo = 0, uiVao = 0, uiVbo = 0;
    ModelObject flashlight;
    std::unordered_map<std::wstring,GLuint> flashlightTextures;
    GLuint grassTexture = 0, stoneTexture = 0, skyTexture = 0;
    GLuint grassNormalTexture = 0, stoneNormalTexture = 0;
    GLuint grassRoughnessTexture = 0, stoneRoughnessTexture = 0;
    GLuint grassAoTexture = 0, stoneAoTexture = 0;
    GLint viewLoc = -1, projectionLoc = -1, modelLoc = -1, eyeLoc = -1, flashLoc = -1, lightPosLoc = -1;
    GLint brightnessLoc = -1, contrastLoc = -1;
    GLint moonlightLoc=-1, skyWeatherLoc=-1, skyCloudLoc=-1, skyMoonLoc=-1, skyMoonlightLoc=-1, flashIntensityLoc=-1;
    GLint shadowMatrixLoc = -1, shadowMapLoc = -1;
    GLint depthMatrixLoc = -1, depthModelLoc = -1;
    GLint skyViewLoc = -1, skyProjectionLoc = -1, skyEyeLoc = -1, skySamplerLoc = -1;
    GLint skyBrightnessLoc = -1, skyContrastLoc = -1;
};

void playSound(const Game& game, const wchar_t* filename) {
    if (!game.soundEnabled) return;
    const auto path=game.executableDirectory/L"assets"/L"sounds"/filename;
    PlaySoundW(path.c_str(),nullptr,SND_ASYNC|SND_FILENAME|SND_NODEFAULT);
}

template<class T>
bool loadProc(T& target, const char* name) {
    PROC proc = wglGetProcAddress(name);
    if (!proc || proc == reinterpret_cast<PROC>(1) || proc == reinterpret_cast<PROC>(2) ||
        proc == reinterpret_cast<PROC>(3) || proc == reinterpret_cast<PROC>(-1)) {
        proc = GetProcAddress(GetModuleHandleW(L"opengl32.dll"), name);
    }
    static_assert(sizeof(target) == sizeof(proc));
    std::memcpy(&target, &proc, sizeof(proc));
    return target != nullptr;
}

bool loadGl() {
#define LOAD_GL(name) if (!loadProc(gl##name, "gl" #name)) return false
    LOAD_GL(CreateShader); LOAD_GL(ShaderSource); LOAD_GL(CompileShader);
    LOAD_GL(GetShaderiv); LOAD_GL(GetShaderInfoLog); LOAD_GL(DeleteShader);
    LOAD_GL(CreateProgram); LOAD_GL(AttachShader); LOAD_GL(LinkProgram);
    LOAD_GL(GetProgramiv); LOAD_GL(GetProgramInfoLog); LOAD_GL(UseProgram);
    LOAD_GL(DeleteProgram); LOAD_GL(GetUniformLocation); LOAD_GL(UniformMatrix4fv);
    LOAD_GL(Uniform3f); LOAD_GL(Uniform1i); LOAD_GL(Uniform1f); LOAD_GL(ActiveTexture); LOAD_GL(GenerateMipmap);
    LOAD_GL(GenVertexArrays); LOAD_GL(BindVertexArray);
    LOAD_GL(GenBuffers); LOAD_GL(BindBuffer); LOAD_GL(BufferData);
    LOAD_GL(EnableVertexAttribArray); LOAD_GL(VertexAttribPointer);
    LOAD_GL(GenFramebuffers); LOAD_GL(BindFramebuffer); LOAD_GL(FramebufferTexture2D); LOAD_GL(CheckFramebufferStatus);
#undef LOAD_GL
    return true;
}

float boundaryRadius(const Game& game, int boundary, float angle) {
    const float base=CenterRadius+RadialStep*boundary;
    if (boundary==0) return base;
    const float progress=static_cast<float>(boundary)/RingCount;
    const float shape=0.68f*std::sin(angle*3.0f+game.mazePhaseA)
                     +0.32f*std::sin(angle*5.0f+game.mazePhaseB);
    return base+0.82f*progress*shape;
}

std::array<float,16> multiplyMatrix(const std::array<float,16>& a,const std::array<float,16>& b) {
    std::array<float,16> result{};
    for (int column=0;column<4;++column) for (int row=0;row<4;++row)
        for (int inner=0;inner<4;++inner)
            result[column*4+row]+=a[inner*4+row]*b[column*4+inner];
    return result;
}

GLuint compileShader(GLenum kind, const char* source) {
    GLuint shader = glCreateShader(kind);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048]{};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        MessageBoxA(nullptr, log, "Maze Escape shader error", MB_OK | MB_ICONERROR);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint makeProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertex = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!vertex || !fragment) return 0;
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048]{};
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        MessageBoxA(nullptr, log, "Maze Escape shader link error", MB_OK | MB_ICONERROR);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

Vec3 normalize(Vec3 v) {
    const float n = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    return n > 0.0f ? Vec3{v.x/n, v.y/n, v.z/n} : Vec3{};
}
float dot(Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}

std::array<float, 16> perspective(float fov, float aspect, float nearPlane, float farPlane) {
    const float f = 1.0f / std::tan(fov * 0.5f);
    std::array<float, 16> m{};
    m[0] = f / aspect; m[5] = f;
    m[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
    m[11] = -1.0f;
    m[14] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
    return m;
}

std::array<float, 16> viewMatrix(Vec3 eye, float yaw, float pitch) {
    const float cp = std::cos(pitch);
    const Vec3 forward = normalize({std::cos(yaw)*cp, std::sin(pitch), std::sin(yaw)*cp});
    const Vec3 side = normalize(cross(forward, {0.0f, 1.0f, 0.0f}));
    const Vec3 up = cross(side, forward);
    std::array<float, 16> m{};
    m[0]=side.x; m[4]=side.y; m[8]=side.z; m[12]=-dot(side,eye);
    m[1]=up.x; m[5]=up.y; m[9]=up.z; m[13]=-dot(up,eye);
    m[2]=-forward.x; m[6]=-forward.y; m[10]=-forward.z; m[14]=dot(forward,eye);
    m[15]=1.0f;
    return m;
}

void addQuad(std::vector<Vertex>& out, Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 normal,
             float r, float g, float blue, float emission = 0.0f, float material = 1.0f) {
    const Vec3 edge1{b.x-a.x,b.y-a.y,b.z-a.z};
    const Vec3 edge2{c.x-a.x,c.y-a.y,c.z-a.z};
    if (dot(cross(edge1,edge2),normal)<0.0f) std::swap(b,d);
    auto uv = [normal,material](Vec3 p) {
        if (material<0.5f) return std::array<float,2>{p.x*0.72f,p.z*0.72f};
        if (material<1.5f) {
            if (std::abs(normal.y)>0.5f) return std::array<float,2>{p.x*0.72f,p.z*0.72f};
            if (std::abs(normal.x)>0.5f) return std::array<float,2>{p.z*0.72f,p.y*0.72f};
            return std::array<float,2>{p.x*0.72f,p.y*0.72f};
        }
        return std::array<float,2>{0.0f,0.0f};
    };
    const auto ua=uv(a), ub=uv(b), uc=uv(c), ud=uv(d);
    const Vertex v[4] = {
        {a.x,a.y,a.z,normal.x,normal.y,normal.z,r,g,blue,emission,ua[0],ua[1],material},
        {b.x,b.y,b.z,normal.x,normal.y,normal.z,r,g,blue,emission,ub[0],ub[1],material},
        {c.x,c.y,c.z,normal.x,normal.y,normal.z,r,g,blue,emission,uc[0],uc[1],material},
        {d.x,d.y,d.z,normal.x,normal.y,normal.z,r,g,blue,emission,ud[0],ud[1],material}
    };
    out.insert(out.end(), {v[0],v[1],v[2],v[0],v[2],v[3]});
}

void addQuadWithUV(std::vector<Vertex>& out, std::array<Vec3,4> points,
                   std::array<std::array<float,2>,4> uv, Vec3 normal,
                   float r, float g, float b) {
    const Vec3 edge1{points[1].x-points[0].x,points[1].y-points[0].y,points[1].z-points[0].z};
    const Vec3 edge2{points[2].x-points[0].x,points[2].y-points[0].y,points[2].z-points[0].z};
    if (dot(cross(edge1,edge2),normal)<0.0f) {
        std::swap(points[1],points[3]);
        std::swap(uv[1],uv[3]);
    }
    Vertex v[4]{};
    for (int i=0;i<4;++i)
        v[i]={points[i].x,points[i].y,points[i].z,normal.x,normal.y,normal.z,
              r,g,b,0.0f,uv[i][0],uv[i][1],1.0f};
    out.insert(out.end(),{v[0],v[1],v[2],v[0],v[2],v[3]});
}

void addStoneTriangle(std::vector<Vertex>& out,Vec3 a,Vec3 b,Vec3 c,Vec3 expectedNormal,
                      float r,float g,float blue) {
    Vec3 normal=normalize(cross({b.x-a.x,b.y-a.y,b.z-a.z},{c.x-a.x,c.y-a.y,c.z-a.z}));
    if (dot(normal,expectedNormal)<0.0f) { std::swap(b,c); normal={-normal.x,-normal.y,-normal.z}; }
    const Vec3 points[]={a,b,c};
    for (Vec3 p:points)
        out.push_back({p.x,p.y,p.z,normal.x,normal.y,normal.z,r,g,blue,0.0f,p.x*0.72f,p.y*0.72f,1.0f});
}

void addRubbleChunk(std::vector<Vertex>& out,Vec3 center,float yaw,float radiusX,float radiusZ,float height) {
    constexpr int sides=5;
    std::array<Vec3,sides> bottom{},middle{},upper{};
    for (int i=0;i<sides;++i) {
        const float angle=yaw+2.0f*Pi*i/sides;
        const float shape=0.80f+0.18f*std::sin(angle*3.7f+center.x*9.0f+center.z*4.0f);
        const float midScale=0.60f+0.12f*std::cos(angle*2.9f+center.z*7.0f);
        const float topScale=0.30f+0.09f*std::sin(angle*4.3f+center.x*5.0f);
        bottom[i]={center.x+std::cos(angle)*radiusX*shape,center.y,center.z+std::sin(angle)*radiusZ*shape};
        middle[i]={center.x+std::cos(angle)*radiusX*midScale,center.y+height*(0.42f+0.12f*std::sin(angle*5.0f)),center.z+std::sin(angle)*radiusZ*midScale};
        upper[i]={center.x+std::cos(angle)*radiusX*topScale,center.y+height*0.82f,center.z+std::sin(angle)*radiusZ*topScale};
    }
    const Vec3 top{center.x+radiusX*0.08f,center.y+height,center.z-radiusZ*0.06f};
    const std::array<float,3> tint{0.78f,0.82f,0.79f};
    for (int i=0;i<sides;++i) {
        const int next=(i+1)%sides;
        const Vec3 outward{(middle[i].x+middle[next].x)*0.5f-center.x,0.0f,
                           (middle[i].z+middle[next].z)*0.5f-center.z};
        addStoneTriangle(out,bottom[i],bottom[next],middle[next],outward,tint[0],tint[1],tint[2]);
        addStoneTriangle(out,bottom[i],middle[next],middle[i],outward,tint[0],tint[1],tint[2]);
        addStoneTriangle(out,middle[i],middle[next],upper[next],outward,tint[0],tint[1],tint[2]);
        addStoneTriangle(out,middle[i],upper[next],upper[i],outward,tint[0],tint[1],tint[2]);
        addStoneTriangle(out,upper[i],upper[next],top,{0.0f,1.0f,0.0f},tint[0],tint[1],tint[2]);
    }
}

void addBox(std::vector<Vertex>& out, float x0, float x1, float y0, float y1,
            float z0, float z1, float r, float g, float b, float emission, float material = 1.0f) {
    addQuad(out,{x0,y1,z0},{x1,y1,z0},{x1,y1,z1},{x0,y1,z1},{0,1,0},r,g,b,emission,material);
    addQuad(out,{x0,y0,z0},{x0,y0,z1},{x1,y0,z1},{x1,y0,z0},{0,-1,0},r,g,b,emission,material);
    addQuad(out,{x0,y0,z1},{x0,y1,z1},{x1,y1,z1},{x1,y0,z1},{0,0,1},r,g,b,emission,material);
    addQuad(out,{x0,y0,z0},{x1,y0,z0},{x1,y1,z0},{x0,y1,z0},{0,0,-1},r,g,b,emission,material);
    addQuad(out,{x1,y0,z0},{x1,y0,z1},{x1,y1,z1},{x1,y1,z0},{1,0,0},r,g,b,emission,material);
    addQuad(out,{x0,y0,z0},{x0,y1,z0},{x0,y1,z1},{x0,y0,z1},{-1,0,0},r,g,b,emission,material);
}

std::vector<Vertex> buildMaze(const Game& game) {
    std::vector<Vertex> out;
    out.reserve(160000);
    const float wallHeight = 4.0f;
    const float floorEdge = HalfMaze + 0.2f;
    addQuad(out,{-floorEdge,0,-floorEdge},{floorEdge,0,-floorEdge},
            {floorEdge,0,floorEdge},{-floorEdge,0,floorEdge},{0,1,0},0.92f,0.96f,0.88f,0.0f,0.0f);
    const float wallHalf=WallHalfThickness;
    const float r=0.86f, g=0.88f, b=0.86f;
    auto at=[&](float radius,float angle,float y) {
        return Vec3{std::cos(angle)*radius,y,std::sin(angle)*radius};
    };
    auto edgeDamage=[](const WallArc& arc,float angle,float radius) {
        auto chip=[](float distance,float length,float height,float phase) {
            if (length<=0.0f || distance>=length) return 0.0f;
            const float remaining=1.0f-distance/length;
            const float jagged=0.68f+0.32f*std::sin(distance*17.0f+phase);
            return height*remaining*jagged;
        };
        const float fromStart=(angle-arc.startAngle)*radius;
        const float fromEnd=(arc.endAngle-angle)*radius;
        return std::max(chip(fromStart,arc.crumbleStartLength,arc.crumbleStartHeight,arc.startAngle*31.0f),
                        chip(fromEnd,arc.crumbleEndLength,arc.crumbleEndHeight,arc.endAngle*27.0f));
    };
    for (const WallArc& arc:game.wallArcs) {
        const float middleRadius=boundaryRadius(game,arc.boundary,(arc.startAngle+arc.endAngle)*0.5f);
        const int segments=std::max(1,static_cast<int>(std::ceil((arc.endAngle-arc.startAngle)*middleRadius/0.08f)));
        const float holeHalfAngle=arc.holeHalfWidth>0.0f?arc.holeHalfWidth/middleRadius:0.0f;
        const float holeStart=arc.holeCenterAngle-holeHalfAngle, holeEnd=arc.holeCenterAngle+holeHalfAngle;
        std::vector<float> angles;
        angles.reserve(static_cast<size_t>(segments)+3);
        for (int i=0;i<=segments;++i)
            angles.push_back(arc.startAngle+(arc.endAngle-arc.startAngle)*i/segments);
        if (arc.holeHalfWidth>0.0f) { angles.push_back(holeStart); angles.push_back(holeEnd); }
        std::sort(angles.begin(),angles.end());
        angles.erase(std::unique(angles.begin(),angles.end(),[](float a,float c){return std::abs(a-c)<1.0e-6f;}),angles.end());
        float along=0.0f;
        for (size_t i=0;i+1<angles.size();++i) {
            const float a0=angles[i], a1=angles[i+1], middle=(a0+a1)*0.5f;
            if (a0<arc.startAngle || a1>arc.endAngle) continue;
            const float inner0=boundaryRadius(game,arc.boundary,a0)-wallHalf;
            const float inner1=boundaryRadius(game,arc.boundary,a1)-wallHalf;
            const float outer0=boundaryRadius(game,arc.boundary,a0)+wallHalf;
            const float outer1=boundaryRadius(game,arc.boundary,a1)+wallHalf;
            const float segmentLength=middleRadius*(a1-a0);
            const float u0=along*0.72f, u1=(along+segmentLength)*0.72f;
            const bool inHole=arc.holeHalfWidth>0.0f && a0>=holeStart-1.0e-5f && a1<=holeEnd+1.0e-5f;
            const float top0=wallHeight-edgeDamage(arc,a0,middleRadius);
            const float top1=wallHeight-edgeDamage(arc,a1,middleRadius);
            const Vec3 outward{std::cos(middle),0.0f,std::sin(middle)};
            auto addSide=[&](float sign,Vec3 normal,float bottom0,float bottom1,float topStart,float topEnd) {
                const float radius0=boundaryRadius(game,arc.boundary,a0)+sign*wallHalf;
                const float radius1=boundaryRadius(game,arc.boundary,a1)+sign*wallHalf;
                addQuadWithUV(out,{at(radius0,a0,bottom0),at(radius1,a1,bottom1),at(radius1,a1,topEnd),at(radius0,a0,topStart)},
                              {{{u0,bottom0*0.72f},{u1,bottom1*0.72f},{u1,topEnd*0.72f},{u0,topStart*0.72f}}},normal,r,g,b);
            };
            if (inHole) {
                addSide(1.0f,outward,0.0f,0.0f,arc.holeBottom,arc.holeBottom);
                addSide(1.0f,outward,arc.holeTop,arc.holeTop,top0,top1);
                addSide(-1.0f,{-outward.x,0.0f,-outward.z},0.0f,0.0f,arc.holeBottom,arc.holeBottom);
                addSide(-1.0f,{-outward.x,0.0f,-outward.z},arc.holeTop,arc.holeTop,top0,top1);
            } else {
                addSide(1.0f,outward,0.0f,0.0f,top0,top1);
                addSide(-1.0f,{-outward.x,0.0f,-outward.z},0.0f,0.0f,top0,top1);
            }
            addQuad(out,at(inner0,a0,top0),at(outer0,a0,top0),
                    at(outer1,a1,top1),at(inner1,a1,top1),{0,1,0},r,g,b);
            along+=segmentLength;
        }
        if (arc.holeHalfWidth>0.0f) {
            const float innerStart=boundaryRadius(game,arc.boundary,holeStart)-wallHalf;
            const float outerStart=boundaryRadius(game,arc.boundary,holeStart)+wallHalf;
            const float innerEnd=boundaryRadius(game,arc.boundary,holeEnd)-wallHalf;
            const float outerEnd=boundaryRadius(game,arc.boundary,holeEnd)+wallHalf;
            addQuad(out,at(innerStart,holeStart,arc.holeBottom),at(outerStart,holeStart,arc.holeBottom),
                    at(outerEnd,holeEnd,arc.holeBottom),at(innerEnd,holeEnd,arc.holeBottom),{0,1,0},r,g,b);
            addQuad(out,at(innerStart,holeStart,arc.holeTop),at(outerStart,holeStart,arc.holeTop),
                    at(outerEnd,holeEnd,arc.holeTop),at(innerEnd,holeEnd,arc.holeTop),{0,-1,0},r,g,b);
            const Vec3 tangentStart{-std::sin(holeStart),0.0f,std::cos(holeStart)};
            const Vec3 tangentEnd{-std::sin(holeEnd),0.0f,std::cos(holeEnd)};
            addQuad(out,at(innerStart,holeStart,arc.holeBottom),at(outerStart,holeStart,arc.holeBottom),
                    at(outerStart,holeStart,arc.holeTop),at(innerStart,holeStart,arc.holeTop),
                    {-tangentStart.x,0.0f,-tangentStart.z},r,g,b);
            addQuad(out,at(outerEnd,holeEnd,arc.holeBottom),at(innerEnd,holeEnd,arc.holeBottom),
                    at(innerEnd,holeEnd,arc.holeTop),at(outerEnd,holeEnd,arc.holeTop),
                    tangentEnd,r,g,b);
        }
        auto addCap=[&](float angle,bool end) {
            const Vec3 tangent{-std::sin(angle),0.0f,std::cos(angle)};
            const float sign=end?1.0f:-1.0f;
            const float innerRadius=boundaryRadius(game,arc.boundary,angle)-wallHalf;
            const float outerRadius=boundaryRadius(game,arc.boundary,angle)+wallHalf;
            const float top=wallHeight-edgeDamage(arc,angle,middleRadius);
            addQuad(out,at(innerRadius,angle,0),at(outerRadius,angle,0),
                    at(outerRadius,angle,top),at(innerRadius,angle,top),
                    {tangent.x*sign,0.0f,tangent.z*sign},r,g,b);
        };
        if (arc.capStart) addCap(arc.startAngle,false);
        if (arc.capEnd) addCap(arc.endAngle,true);
    }
    for (const WallArc& arc:game.wallArcs) {
        if (arc.crumbleStartLength<=0.0f && arc.crumbleEndLength<=0.0f) continue;
        const float middleRadius=boundaryRadius(game,arc.boundary,(arc.startAngle+arc.endAngle)*0.5f);
        const float side=((arc.boundary*17+static_cast<int>(arc.startAngle*100.0f))&1)?1.0f:-1.0f;
        auto addDebrisAt=[&](float edgeAngle,float distance,float height,float direction) {
            if (height<=0.0f) return;
            for (int piece=0;piece<3;++piece) {
                const float along=distance+0.13f*piece;
                const float angle=edgeAngle+direction*along/middleRadius;
                const float radius=boundaryRadius(game,arc.boundary,angle)+side*(wallHalf+0.14f);
                const float size=piece==0?0.24f:0.13f+0.035f*piece;
                addRubbleChunk(out,at(radius,angle,0.004f),angle+piece*0.83f,size,
                               size*(0.72f+0.08f*piece),std::min(0.34f,height*(piece==0?0.32f:0.18f)));
            }
        };
        addDebrisAt(arc.startAngle,arc.crumbleStartLength*0.22f,arc.crumbleStartHeight,1.0f);
        addDebrisAt(arc.endAngle,arc.crumbleEndLength*0.22f,arc.crumbleEndHeight,-1.0f);
    }
    for (const RadialWall& wall:game.radialWalls) {
        const Vec3 direction{std::cos(wall.angle),0.0f,std::sin(wall.angle)};
        const Vec3 tangent{-direction.z,0.0f,direction.x};
        auto atSide=[&](float radius,float side,float y) {
            return Vec3{direction.x*radius+tangent.x*side,y,direction.z*radius+tangent.z*side};
        };
        const float half=WallHalfThickness;
        addQuad(out,atSide(wall.startRadius,half,0),atSide(wall.endRadius,half,0),
                atSide(wall.endRadius,half,wallHeight),atSide(wall.startRadius,half,wallHeight),
                tangent,r,g,b);
        addQuad(out,atSide(wall.endRadius,-half,0),atSide(wall.startRadius,-half,0),
                atSide(wall.startRadius,-half,wallHeight),atSide(wall.endRadius,-half,wallHeight),
                {-tangent.x,0.0f,-tangent.z},r,g,b);
        addQuad(out,atSide(wall.startRadius,-half,wallHeight),atSide(wall.endRadius,-half,wallHeight),
                atSide(wall.endRadius,half,wallHeight),atSide(wall.startRadius,half,wallHeight),
                {0,1,0},r,g,b);
        if (wall.capStart)
            addQuad(out,atSide(wall.startRadius,half,0),atSide(wall.startRadius,-half,0),
                    atSide(wall.startRadius,-half,wallHeight),atSide(wall.startRadius,half,wallHeight),
                    {-direction.x,0.0f,-direction.z},r,g,b);
        if (wall.capEnd)
            addQuad(out,atSide(wall.endRadius,-half,0),atSide(wall.endRadius,half,0),
                    atSide(wall.endRadius,half,wallHeight),atSide(wall.endRadius,-half,wallHeight),
                    direction,r,g,b);
    }
    for (const BatteryPickup& pickup:game.batteryPickups) if (!pickup.collected) {
        addBox(out,pickup.x-0.12f,pickup.x+0.12f,0.045f,0.19f,pickup.z-0.07f,pickup.z+0.07f,
               0.24f,0.28f,0.30f,0.0f,1.0f);
        addBox(out,pickup.x-0.075f,pickup.x+0.075f,0.19f,0.225f,pickup.z-0.073f,pickup.z+0.073f,
               0.15f,0.78f,0.48f,1.8f,2.0f);
    }
    for (const RuinTrace& trace:game.ruinTraces) if (!trace.found) {
        if (trace.kind==TraceKind::ExitClue) {
            addBox(out,trace.x-0.12f,trace.x+0.12f,0.015f,0.39f,trace.z-0.055f,trace.z+0.055f,
                   0.28f,0.31f,0.29f,0.0f,1.0f);
            addBox(out,trace.x-0.075f,trace.x+0.075f,0.18f,0.205f,trace.z-0.064f,trace.z-0.059f,
                   0.18f,0.46f,0.38f,0.45f,2.0f);
            addBox(out,trace.x-0.015f,trace.x+0.015f,0.09f,0.30f,trace.z-0.065f,trace.z-0.060f,
                   0.18f,0.46f,0.38f,0.38f,2.0f);
        } else if (trace.kind==TraceKind::BatteryCache) {
            addBox(out,trace.x-0.18f,trace.x+0.18f,0.015f,0.23f,trace.z-0.14f,trace.z+0.14f,
                   0.24f,0.19f,0.13f,0.0f,1.0f);
            addBox(out,trace.x-0.19f,trace.x+0.19f,0.15f,0.19f,trace.z-0.15f,trace.z+0.15f,
                   0.33f,0.26f,0.17f,0.0f,1.0f);
            addBox(out,trace.x-0.055f,trace.x+0.055f,0.23f,0.27f,trace.z-0.025f,trace.z+0.025f,
                   0.14f,0.72f,0.42f,1.3f,2.0f);
        } else {
            addRubbleChunk(out,{trace.x-0.08f,0.012f,trace.z+0.035f},0.4f,0.16f,0.11f,0.08f);
            addRubbleChunk(out,{trace.x+0.10f,0.012f,trace.z-0.025f},1.7f,0.12f,0.10f,0.065f);
            addBox(out,trace.x-0.13f,trace.x+0.13f,0.025f,0.06f,trace.z-0.10f,trace.z+0.10f,
                   0.25f,0.21f,0.15f,0.0f,1.0f);
        }
    }
    if (!game.previousTrail.empty() && game.previousTrailSeed!=game.mazeSeed) {
        Vec3 lastMark{};
        bool haveLastMark=false;
        for (size_t i=0;i<game.previousTrail.size();++i) {
            const TrailPoint& point=game.previousTrail[i];
            const int sourceCol=std::clamp(static_cast<int>(std::floor((HalfMaze-point.x)/Cell)),0,MapSize-1);
            const int sourceRow=std::clamp(static_cast<int>(std::floor((HalfMaze-point.z)/Cell)),0,MapSize-1);
            int bestCol=-1,bestRow=-1;
            float bestDistance=1.0f;
            for (int row=std::max(0,sourceRow-18);row<=std::min(MapSize-1,sourceRow+18);++row)
                for (int col=std::max(0,sourceCol-18);col<=std::min(MapSize-1,sourceCol+18);++col) {
                    if (game.map[row][col]!='.') continue;
                    const float x=HalfMaze-(col+0.5f)*Cell,z=HalfMaze-(row+0.5f)*Cell;
                    const float dx=x-point.x,dz=z-point.z,distance=dx*dx+dz*dz;
                    if (distance<bestDistance) { bestDistance=distance; bestCol=col; bestRow=row; }
                }
            if (bestCol<0) continue;
            const float x=HalfMaze-(bestCol+0.5f)*Cell,z=HalfMaze-(bestRow+0.5f)*Cell;
            if (haveLastMark && std::hypot(x-lastMark.x,z-lastMark.z)<0.18f) continue;
            const Vec3 forward{std::cos(point.yaw),0.0f,std::sin(point.yaw)};
            const Vec3 right{std::sin(point.yaw),0.0f,-std::cos(point.yaw)};
            for (float side:{-0.075f,0.075f}) {
                const float cx=x+right.x*side,cz=z+right.z*side;
                const Vec3 center{cx,0.012f,cz};
                const Vec3 f{forward.x*0.075f,0.0f,forward.z*0.075f};
                const Vec3 r{right.x*0.035f,0.0f,right.z*0.035f};
                addQuad(out,{center.x-f.x-r.x,center.y,center.z-f.z-r.z},
                        {center.x+f.x-r.x,center.y,center.z+f.z-r.z},
                        {center.x+f.x+r.x,center.y,center.z+f.z+r.z},
                        {center.x-f.x+r.x,center.y,center.z-f.z+r.z},
                        {0,1,0},0.12f,0.44f,0.52f,0.24f,2.0f);
            }
            lastMark={x,0.012f,z};
            haveLastMark=true;
        }
    }
    const float gateRadius=boundaryRadius(game,RingCount,ExitAngle)+0.30f;
    const float gateX=std::cos(ExitAngle)*gateRadius, gateZ=std::sin(ExitAngle)*gateRadius;
    addBox(out,gateX-1.08f,gateX-0.95f,0,1.85f,gateZ-0.12f,gateZ+0.12f,0.10f,0.88f,0.72f,1.35f,2.0f);
    addBox(out,gateX+0.95f,gateX+1.08f,0,1.85f,gateZ-0.12f,gateZ+0.12f,0.10f,0.88f,0.72f,1.35f,2.0f);
    addBox(out,gateX-1.08f,gateX+1.08f,1.72f,1.88f,gateZ-0.12f,gateZ+0.12f,0.10f,0.88f,0.72f,1.35f,2.0f);
    return out;
}

void uploadMazeGeometry(Game& game) {
    if (!game.worldVao || !game.worldVbo) return;
    const auto vertices=buildMaze(game);
    game.worldVertexCount=static_cast<GLsizei>(vertices.size());
    glBindVertexArray(game.worldVao);
    glBindBuffer(GL_ARRAY_BUFFER,game.worldVbo);
    glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(vertices.size()*sizeof(Vertex)),vertices.data(),GL_DYNAMIC_DRAW);
}

bool generateMaze(Game& game) {
    struct CellNode { int ring, sector; };
    std::vector<CellNode> nodes{{0,0}};
    std::vector<std::vector<int>> rings(RingCount+1);
    rings[0].push_back(0);
    for (int ring=1;ring<=RingCount;++ring) {
        const int sectors=sectorsForRing(ring);
        rings[ring].reserve(sectors);
        for (int sector=0;sector<sectors;++sector) {
            rings[ring].push_back(static_cast<int>(nodes.size()));
            nodes.push_back({ring,sector});
        }
    }
    std::vector<std::vector<int>> graph(nodes.size());
    auto connect=[&](int a,int b) { graph[a].push_back(b); graph[b].push_back(a); };
    for (int ring=1;ring<=RingCount;++ring) {
        const int sectors=sectorsForRing(ring);
        for (int sector=0;sector<sectors;++sector) {
            connect(rings[ring][sector],rings[ring][(sector+1)%sectors]);
            if (ring==1) connect(rings[0][0],rings[ring][sector]);
            else {
                const int innerSectors=sectorsForRing(ring-1);
                const float center=(static_cast<float>(sector)+0.5f)/sectors;
                const int parent=std::min(innerSectors-1,static_cast<int>(center*innerSectors));
                connect(rings[ring][sector],rings[ring-1][parent]);
            }
        }
    }

    std::random_device randomDevice;
    const uint32_t seed=game.hasRequestedSeed?game.requestedSeed:randomDevice();
    game.hasRequestedSeed=false;
    game.mazeSeed=seed;
    const uint32_t omenBits=seed^(seed>>11)^(seed>>23);
    game.runOmen=static_cast<RunOmen>(omenBits%4u);
    game.runtimeRandom=seed^0x9e3779b9u;
    if (!game.runtimeRandom) game.runtimeRandom=1;
    std::mt19937 random(seed);
    std::uniform_real_distribution<float> phase(0.0f,2.0f*Pi);
    game.mazePhaseA=phase(random);
    game.mazePhaseB=phase(random);
    std::vector<std::vector<int>> passages(nodes.size());
    std::vector<uint8_t> visited(nodes.size(),0);
    std::vector<int> stack{0};
    visited[0]=1;
    while (!stack.empty()) {
        const int current=stack.back();
        std::vector<int> choices;
        for (int next:graph[current]) if (!visited[next]) choices.push_back(next);
        if (choices.empty()) { stack.pop_back(); continue; }
        std::shuffle(choices.begin(),choices.end(),random);
        const int next=choices.back();
        visited[next]=1;
        passages[current].push_back(next);
        passages[next].push_back(current);
        stack.push_back(next);
    }
    if (std::find(visited.begin(),visited.end(),uint8_t{0})!=visited.end()) return false;

    game.batteryPickups.clear();
    game.ruinTraces.clear();
    std::vector<int> deadEnds;
    const int exitNode=rings[RingCount][std::min(sectorsForRing(RingCount)-1,
        static_cast<int>(ExitAngle/(2.0f*Pi)*sectorsForRing(RingCount)))];
    for (int node=1;node<static_cast<int>(nodes.size());++node)
        if (node!=exitNode && passages[node].size()==1) deadEnds.push_back(node);
    std::shuffle(deadEnds.begin(),deadEnds.end(),random);
    const size_t pickupCount=std::min<size_t>(4,deadEnds.size());
    std::vector<int> batteryNodes;
    for (size_t i=0;i<pickupCount;++i) {
        batteryNodes.push_back(deadEnds[i]);
        const CellNode node=nodes[deadEnds[i]];
        const float angle=(node.sector+0.5f)*2.0f*Pi/sectorsForRing(node.ring);
        const float inner=boundaryRadius(game,node.ring-1,angle),outer=boundaryRadius(game,node.ring,angle);
        const float radius=(inner+outer)*0.5f;
        game.batteryPickups.push_back({std::cos(angle)*radius,std::sin(angle)*radius,false});
    }

    std::vector<int> nextTowardExit(nodes.size(),-1);
    std::queue<int> toExit;
    nextTowardExit[exitNode]=exitNode;
    toExit.push(exitNode);
    while (!toExit.empty()) {
        const int current=toExit.front();
        toExit.pop();
        for (int next:passages[current]) if (nextTowardExit[next]<0) {
            nextTowardExit[next]=current;
            toExit.push(next);
        }
    }
    auto nodePosition=[&](int id) {
        const CellNode node=nodes[id];
        if (node.ring==0) return std::pair<float,float>{0.0f,0.0f};
        const float angle=(node.sector+0.5f)*2.0f*Pi/sectorsForRing(node.ring);
        const float radius=(boundaryRadius(game,node.ring-1,angle)+boundaryRadius(game,node.ring,angle))*0.5f;
        return std::pair<float,float>{std::cos(angle)*radius,std::sin(angle)*radius};
    };
    std::vector<int> traceCells;
    for (int node=1;node<static_cast<int>(nodes.size());++node) {
        if (node==exitNode || std::find(batteryNodes.begin(),batteryNodes.end(),node)!=batteryNodes.end()) continue;
        traceCells.push_back(node);
    }
    std::shuffle(traceCells.begin(),traceCells.end(),random);
    std::array<TraceKind,5> traceKinds={TraceKind::ExitClue,TraceKind::ExitClue,
                                        TraceKind::BatteryCache,TraceKind::Remnant,TraceKind::Remnant};
    std::shuffle(traceKinds.begin(),traceKinds.end(),random);
    const size_t traceCount=std::min(traceCells.size(),traceKinds.size());
    for (size_t i=0;i<traceCount;++i) {
        const int node=traceCells[i], next=nextTowardExit[node];
        if (next<0) continue;
        const auto [x,z]=nodePosition(node);
        const auto [hintX,hintZ]=nodePosition(next);
        game.ruinTraces.push_back({x,z,hintX,hintZ,traceKinds[i],false,
                                   static_cast<uint8_t>(random()%4u)});
    }

    auto wrap=[&](float angle) { return std::atan2(std::sin(angle),std::cos(angle)); };
    auto hasPassage=[&](int a,int b) {
        const auto& edges=passages[a];
        return std::find(edges.begin(),edges.end(),b)!=edges.end();
    };
    const int outerSectors=sectorsForRing(RingCount);
    const int exitSector=std::min(outerSectors-1,static_cast<int>(ExitAngle/(2.0f*Pi)*outerSectors));
    const float outerPortalAngle=(exitSector+0.5f)*2.0f*Pi/outerSectors;
    std::uniform_real_distribution<float> holeChance(0.0f,1.0f);
    auto addWallArc=[&](int boundary,float start,float end,bool capStart,bool capEnd) {
        WallArc arc{boundary,start,end,capStart,capEnd};
        const float localRadius=boundaryRadius(game,boundary,(start+end)*0.5f);
        const float arcLength=(end-start)*localRadius;
        if (boundary>0 && boundary<RingCount && arcLength>1.0f && holeChance(random)<0.16f) {
            arc.holeCenterAngle=(start+end)*0.5f;
            arc.holeHalfWidth=std::min(0.25f,arcLength*0.12f);
            arc.holeBottom=1.16f;
            arc.holeTop=1.62f;
        } else if (boundary>0 && boundary<RingCount && arcLength>1.8f && holeChance(random)<0.12f) {
            std::uniform_real_distribution<float> crumbleLength(0.5f,1.0f);
            std::uniform_real_distribution<float> crumbleHeight(0.55f,1.25f);
            if (holeChance(random)<0.78f) {
                arc.crumbleStartLength=crumbleLength(random);
                arc.crumbleStartHeight=crumbleHeight(random);
            }
            if (holeChance(random)<0.78f) {
                arc.crumbleEndLength=crumbleLength(random);
                arc.crumbleEndHeight=crumbleHeight(random);
            }
        }
        game.wallArcs.push_back(arc);
    };

    game.wallArcs.clear();
    game.radialWalls.clear();
    for (int boundary=0;boundary<RingCount;++boundary) {
        const int outerRing=boundary+1, outerSectors=sectorsForRing(outerRing);
        const int innerSectors=sectorsForRing(boundary);
        const float sectorArc=2.0f*Pi/outerSectors;
        for (int sector=0;sector<outerSectors;++sector) {
            const float start=sector*sectorArc, end=start+sectorArc, center=(start+end)*0.5f;
            const float radius=boundaryRadius(game,boundary,center);
            const int parent=std::min(innerSectors-1,static_cast<int>((sector+0.5f)*innerSectors/outerSectors));
            const bool open=hasPassage(rings[outerRing][sector],rings[boundary][parent]);
            if (!open) {
                addWallArc(boundary,start,end,false,false);
                continue;
            }
            const float opening=std::min(DoorHalfWidth+WallHalfThickness,0.46f*sectorArc*radius);
            const float halfAngle=opening/radius;
            addWallArc(boundary,start,center-halfAngle,false,true);
            addWallArc(boundary,center+halfAngle,end,true,false);
        }
    }
    for (int ring=1;ring<=RingCount;++ring) {
        const int sectors=sectorsForRing(ring);
        for (int boundary=0;boundary<sectors;++boundary) {
            const float angle=2.0f*Pi*boundary/sectors;
            const float innerRadius=boundaryRadius(game,ring-1,angle);
            const float outerRadiusForRing=boundaryRadius(game,ring,angle);
            const float middleRadius=(innerRadius+outerRadiusForRing)*0.5f;
            const int before=(boundary+sectors-1)%sectors, after=boundary;
            const bool open=hasPassage(rings[ring][before],rings[ring][after]);
            if (!open) {
                game.radialWalls.push_back({angle,innerRadius,outerRadiusForRing,false,false});
                continue;
            }
            game.radialWalls.push_back({angle,innerRadius,
                                        middleRadius-DoorHalfWidth,false,true});
            game.radialWalls.push_back({angle,middleRadius+DoorHalfWidth,
                                        outerRadiusForRing,true,false});
        }
    }
    const float exitRadius=boundaryRadius(game,RingCount,outerPortalAngle);
    const float exitHalfAngle=(DoorHalfWidth+WallHalfThickness)/exitRadius;
    addWallArc(RingCount,0.0f,outerPortalAngle-exitHalfAngle,false,true);
    addWallArc(RingCount,outerPortalAngle+exitHalfAngle,2.0f*Pi,true,false);

    game.map.assign(MapSize,std::string(MapSize,'#'));
    for (int row=0;row<MapSize;++row) for (int col=0;col<MapSize;++col) {
        const float x=HalfMaze-(col+0.5f)*Cell;
        const float z=HalfMaze-(row+0.5f)*Cell;
        const float radius=std::hypot(x,z), angle=std::atan2(z,x);
        const float outerRadius=boundaryRadius(game,RingCount,angle);
        bool floor=false;
        if (radius>=outerRadius) {
            floor=std::abs(wrap(angle-ExitAngle))*outerRadius<DoorHalfWidth && radius<outerRadius+0.95f;
        } else if (radius<CenterRadius) {
            floor=true;
        } else {
            int ring=1;
            while (ring<RingCount && radius>=boundaryRadius(game,ring,angle)) ++ring;
            const int sectors=sectorsForRing(ring);
            float turn=angle/(2.0f*Pi);
            if (turn<0.0f) turn+=1.0f;
            const float sectorPosition=turn*sectors;
            const int sector=std::min(sectors-1,static_cast<int>(sectorPosition));
            floor=true;

            for (int boundary=0;boundary<RingCount;++boundary) {
                const float localBoundaryRadius=boundaryRadius(game,boundary,angle);
                if (std::abs(radius-localBoundaryRadius)>=WallHalfThickness) continue;
                const int outerRing=boundary+1;
                const int outerSectors=sectorsForRing(outerRing);
                const int outerSector=std::min(outerSectors-1,static_cast<int>(turn*outerSectors));
                const int outerId=rings[outerRing][outerSector];
                const float outerCenter=(outerSector+0.5f)*2.0f*Pi/outerSectors;
                const int innerSectors=sectorsForRing(boundary);
                const int parentSector=std::min(innerSectors-1,static_cast<int>((outerSector+0.5f)*innerSectors/outerSectors));
                const int innerId=rings[boundary][parentSector];
                const float opening=std::min(DoorHalfWidth,0.46f*(2.0f*Pi*boundaryRadius(game,boundary,outerCenter)/outerSectors));
                if (!hasPassage(outerId,innerId) || std::abs(wrap(angle-outerCenter))*localBoundaryRadius>=opening)
                    floor=false;
                break;
            }

            const float fraction=sectorPosition-std::floor(sectorPosition);
            const float angularDistance=std::min(fraction,1.0f-fraction)*(2.0f*Pi*radius/sectors);
            if (ring>0 && angularDistance<WallHalfThickness) {
                const int edgeBefore=(fraction<0.5f?(sector+sectors-1)%sectors:sector);
                const int other=(edgeBefore+1)%sectors;
                const int edgeA=rings[ring][edgeBefore], edgeB=rings[ring][other];
                const float middleRadius=(boundaryRadius(game,ring-1,angle)+boundaryRadius(game,ring,angle))*0.5f;
                if (!hasPassage(edgeA,edgeB) || std::abs(radius-middleRadius)>DoorHalfWidth)
                    floor=false;
            }
        }
        if (radius>=outerRadius-WallHalfThickness && radius<outerRadius+WallHalfThickness) {
            if (std::abs(wrap(angle-outerPortalAngle))*outerRadius>=DoorHalfWidth) floor=false;
        }
        game.map[row][col]=floor?'.':'#';
    }
    return game.map.size()==MapSize && std::all_of(game.map.begin(),game.map.end(),[](const std::string& row){return row.size()==MapSize;});
}

bool loadMap(Game& game) {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr,path,MAX_PATH);
    game.executableDirectory=std::filesystem::path(path).parent_path();
    game.settingsPath=game.executableDirectory/L"settings.ini";
    game.pastRunsPath=game.executableDirectory/L"past_runs.csv";
    game.trailPath=game.executableDirectory/L"last_run_trail.csv";
    game.resolutionIndex=std::clamp(static_cast<int>(GetPrivateProfileIntW(L"Video",L"Resolution",1,game.settingsPath.c_str())),0,3);
    game.brightness=std::clamp(GetPrivateProfileIntW(L"Video",L"Brightness",100,game.settingsPath.c_str())/100.0f,0.5f,1.5f);
    game.contrast=std::clamp(GetPrivateProfileIntW(L"Video",L"Contrast",100,game.settingsPath.c_str())/100.0f,0.5f,1.5f);
    game.soundEnabled=GetPrivateProfileIntW(L"Audio",L"Sound",1,game.settingsPath.c_str())!=0;
    game.difficultyIndex=std::clamp(static_cast<int>(GetPrivateProfileIntW(L"Game",L"Difficulty",1,game.settingsPath.c_str())),0,2);
    game.mouseSensitivity=std::clamp(GetPrivateProfileIntW(L"Controls",L"MouseSensitivity",100,game.settingsPath.c_str())/100.0f,0.5f,2.0f);
    game.headBobEnabled=GetPrivateProfileIntW(L"Controls",L"HeadBob",1,game.settingsPath.c_str())!=0;
    const int bestTimeMs=GetPrivateProfileIntW(L"Records",L"BestTimeMs",0,game.settingsPath.c_str());
    game.bestTime=bestTimeMs>0?bestTimeMs/1000.0f:0.0f;
    wchar_t bestSeedText[32]{};
    GetPrivateProfileStringW(L"Records",L"BestSeed",L"0",bestSeedText,32,game.settingsPath.c_str());
    wchar_t* seedEnd=nullptr;
    const unsigned long bestSeed=std::wcstoul(bestSeedText,&seedEnd,10);
    game.bestSeed=seedEnd!=bestSeedText && bestSeed<=std::numeric_limits<uint32_t>::max()
        ?static_cast<uint32_t>(bestSeed):0u;
    constexpr int widths[]={960,1280,1600}, heights[]={600,800,900};
    game.width=game.resolutionIndex==3?GetSystemMetrics(SM_CXSCREEN):widths[game.resolutionIndex];
    game.height=game.resolutionIndex==3?GetSystemMetrics(SM_CYSCREEN):heights[game.resolutionIndex];
    return generateMaze(game);
}

void regenerateMaze(Game& game) {
    if (!generateMaze(game)) return;
    game.routeVertexCount=0;
    uploadMazeGeometry(game);
}

bool atFloor(const Game& game, float x, float z) {
    const int col=static_cast<int>(std::floor((HalfMaze-x)/Cell));
    const int row=static_cast<int>(std::floor((HalfMaze-z)/Cell));
    if (col>=0 && row>=0 && col<MapSize && row<MapSize) return game.map[row][col]=='.';
    const float radius=std::hypot(x,z);
    const float angle=std::atan2(z,x);
    const float delta=std::atan2(std::sin(angle-ExitAngle),std::cos(angle-ExitAngle));
    return radius<boundaryRadius(game,RingCount,angle)+0.95f && std::abs(delta)<0.11f;
}

bool canStand(const Game& game, float x, float z) {
    constexpr float radius=0.35f;
    if (!atFloor(game,x,z)) return false;
    const int minCol=std::max(0,static_cast<int>(std::floor((HalfMaze-(x+radius))/Cell)));
    const int maxCol=std::min(MapSize-1,static_cast<int>(std::floor((HalfMaze-(x-radius))/Cell)));
    const int minRow=std::max(0,static_cast<int>(std::floor((HalfMaze-(z+radius))/Cell)));
    const int maxRow=std::min(MapSize-1,static_cast<int>(std::floor((HalfMaze-(z-radius))/Cell)));
    for (int row=minRow;row<=maxRow;++row) for (int col=minCol;col<=maxCol;++col) {
        if (game.map[row][col]!='#') continue;
        const float right=HalfMaze-col*Cell, left=right-Cell;
        const float top=HalfMaze-row*Cell, bottom=top-Cell;
        const float dx=x-std::clamp(x,left,right), dz=z-std::clamp(z,bottom,top);
        if (dx*dx+dz*dz<radius*radius) return false;
    }
    for (int i=0;i<16;++i) {
        const float a=2.0f*Pi*i/16.0f;
        if (!atFloor(game,x+std::cos(a)*radius,z+std::sin(a)*radius)) return false;
    }
    return true;
}

void formatTime(float elapsed, char* out) {
    const int seconds=static_cast<int>(elapsed);
    wsprintfA(out,"%02d:%02d",seconds/60,seconds%60);
}

const char* runOmenName(RunOmen omen) {
    switch (omen) {
    case RunOmen::VeiledMoon: return "VEILED MOON";
    case RunOmen::FailingWick: return "FAILING WICK";
    case RunOmen::DistantBells: return "DISTANT BELLS";
    case RunOmen::HowlingWind: return "HOWLING WIND";
    default: return "OLD RUN";
    }
}

void captureMouse(Game& game, bool capture) {
    game.mouseCaptured=capture;
    if (capture) {
        RECT rect{};
        GetWindowRect(game.window,&rect);
        ClipCursor(&rect);
        SetCursor(nullptr);
    } else {
        ClipCursor(nullptr);
        SetCursor(LoadCursorW(nullptr,IDC_ARROW));
    }
}

float randomUnit(Game& game) {
    uint32_t& value=game.runtimeRandom;
    value^=value<<13; value^=value>>17; value^=value<<5;
    return static_cast<float>(value&0x00ffffffu)/16777215.0f;
}

float initialBattery(const Game& game) {
    return game.difficultyIndex==0?100.0f:(game.difficultyIndex==1?80.0f:65.0f);
}

float routeCooldownLength(const Game& game) {
    return game.difficultyIndex==0?10.0f:(game.difficultyIndex==1?15.0f:25.0f);
}

void resetRunState(Game& game) {
    game.runSeed=game.mazeSeed;
    game.batteryCharge=initialBattery(game);
    game.batteryCount=0;
    game.routeUses=0;
    game.flashlightOn=true;
    game.flickerTimer=0.0f;
    game.nextFlicker=26.0f+randomUnit(game)*22.0f;
    if (game.runOmen==RunOmen::FailingWick) game.nextFlicker*=0.58f;
    game.newRecord=false;
}

void startTrail(Game& game) {
    game.currentTrail.clear();
    game.currentTrail.push_back({game.x,game.z,game.yaw});
    game.trailProgress=0.0f;
    game.noticeMessage.clear();
    game.noticeTimer=0.0f;
}

void saveRunRecord(Game& game) {
    const std::wstring time=std::to_wstring(static_cast<int>(std::lround(game.bestTime*1000.0f)));
    const std::wstring seed=std::to_wstring(game.bestSeed);
    WritePrivateProfileStringW(L"Records",L"BestTimeMs",time.c_str(),game.settingsPath.c_str());
    WritePrivateProfileStringW(L"Records",L"BestSeed",seed.c_str(),game.settingsPath.c_str());
    WritePrivateProfileStringW(nullptr,nullptr,nullptr,game.settingsPath.c_str());
}

void loadPastRuns(Game& game) {
    std::ifstream file(game.pastRunsPath);
    std::string line;
    while (std::getline(file,line)) {
        std::istringstream row(line);
        std::string timestamp,elapsedText,routeText,seedText;
        if (!std::getline(row,timestamp,',') || !std::getline(row,elapsedText,',') ||
            !std::getline(row,routeText,',') || !std::getline(row,seedText,',')) continue;
        try {
            const unsigned long seed=std::stoul(seedText);
            if (seed>std::numeric_limits<uint32_t>::max()) continue;
            RunOmen omen=RunOmen::None;
            std::string omenText;
            if (std::getline(row,omenText)) {
                const int savedOmen=std::stoi(omenText);
                if (savedOmen>=0 && savedOmen<4) omen=static_cast<RunOmen>(savedOmen);
            }
            game.pastRuns.push_back({timestamp,std::stof(elapsedText),std::stoi(routeText),
                                     static_cast<uint32_t>(seed),omen});
        } catch (const std::exception&) {
            continue;
        }
    }
}

void recordPastRun(Game& game) {
    SYSTEMTIME localTime{};
    GetLocalTime(&localTime);
    char timestamp[32]{};
    wsprintfA(timestamp,"%04u %02u %02u %02u:%02u:%02u",localTime.wYear,localTime.wMonth,
              localTime.wDay,localTime.wHour,localTime.wMinute,localTime.wSecond);
    PastRun run{timestamp,game.elapsed,game.routeUses,game.completedSeed,game.runOmen};
    std::ofstream file(game.pastRunsPath,std::ios::app);
    if (file) file<<run.timestamp<<','<<run.elapsed<<','<<run.routeUses<<','<<run.seed<<','
                  <<static_cast<int>(run.omen)<<'\n';
    game.pastRuns.push_back(std::move(run));
}

void loadPreviousTrail(Game& game) {
    std::ifstream file(game.trailPath);
    std::string line;
    while (std::getline(file,line)) {
        if (line.rfind("SEED,",0)==0) {
            try {
                const unsigned long seed=std::stoul(line.substr(5));
                if (seed<=std::numeric_limits<uint32_t>::max()) game.previousTrailSeed=static_cast<uint32_t>(seed);
            } catch (const std::exception&) {}
            continue;
        }
        std::istringstream row(line);
        std::string xText,zText,yawText;
        if (!std::getline(row,xText,',') || !std::getline(row,zText,',') || !std::getline(row,yawText)) continue;
        try {
            TrailPoint point{std::stof(xText),std::stof(zText),std::stof(yawText)};
            if (std::isfinite(point.x) && std::isfinite(point.z) && std::isfinite(point.yaw) &&
                game.previousTrail.size()<2400) game.previousTrail.push_back(point);
        } catch (const std::exception&) {
            continue;
        }
    }
}

void savePreviousTrail(Game& game) {
    if (game.currentTrail.empty()) game.currentTrail.push_back({game.x,game.z,game.yaw});
    else {
        const TrailPoint& last=game.currentTrail.back();
        if (std::hypot(game.x-last.x,game.z-last.z)>0.18f)
            game.currentTrail.push_back({game.x,game.z,game.yaw});
    }
    game.previousTrail=game.currentTrail;
    game.previousTrailSeed=game.runSeed;
    std::ofstream file(game.trailPath,std::ios::trunc);
    if (!file) return;
    file<<"SEED,"<<game.previousTrailSeed<<'\n';
    for (const TrailPoint& point:game.previousTrail)
        file<<point.x<<','<<point.z<<','<<point.yaw<<'\n';
}

void collectNearbyFinds(Game& game) {
    bool changed=false;
    for (BatteryPickup& pickup:game.batteryPickups) {
        if (pickup.collected) continue;
        const float dx=game.x-pickup.x,dz=game.z-pickup.z;
        if (dx*dx+dz*dz>0.52f*0.52f) continue;
        pickup.collected=true;
        ++game.batteryCount;
        playSound(game,L"battery_pickup.wav");
        changed=true;
    }
    for (RuinTrace& trace:game.ruinTraces) {
        if (trace.found) continue;
        const float dx=game.x-trace.x,dz=game.z-trace.z;
        if (dx*dx+dz*dz>0.68f*0.68f) continue;
        trace.found=true;
        changed=true;
        game.noticeTimer=5.0f;
        if (trace.kind==TraceKind::ExitClue) {
            const float angle=std::atan2(trace.hintZ-game.z,trace.hintX-game.x);
            const float relative=std::atan2(std::sin(angle-game.yaw),std::cos(angle-game.yaw));
            const char* direction=std::abs(relative)<0.52f?"AHEAD":
                (std::abs(relative)>2.62f?"BEHIND":(relative>0.0f?"RIGHT":"LEFT"));
            game.noticeMessage=std::string("SCRATCHED ARROW: EXIT PATH IS TO YOUR ")+direction;
        } else if (trace.kind==TraceKind::BatteryCache) {
            ++game.batteryCount;
            game.noticeMessage="ABANDONED PACK: ONE BATTERY CELL ADDED";
            playSound(game,L"battery_pickup.wav");
        } else {
            constexpr const char* notes[]={
                "A NAME IS CUT INTO THE STONE. NO ONE ANSWERS",
                "A COLD CAMPFIRE STILL SMELLS OF SMOKE",
                "THREE SCRATCHES MARK THE WALL. NOTHING ELSE",
                "A TORN MAP ENDS BEFORE THE EXIT"
            };
            game.noticeMessage=notes[trace.note%4];
        }
    }
    if (changed) uploadMazeGeometry(game);
}

void updateFlashlight(Game& game,float dt) {
    game.flickerTimer=std::max(0.0f,game.flickerTimer-dt);
    if (!game.flashlightOn || game.batteryCharge<=0.0f) return;
    const float drainPerMinute=game.difficultyIndex==0?3.0f:(game.difficultyIndex==1?7.5f:14.0f);
    game.batteryCharge=std::max(0.0f,game.batteryCharge-drainPerMinute*dt/60.0f);
    if (game.batteryCharge<=0.0f) {
        game.flashlightOn=false;
        game.flickerTimer=0.0f;
        playSound(game,L"flashlight_flicker.wav");
        return;
    }
    game.nextFlicker-=dt;
    if (game.nextFlicker<=0.0f) {
        game.flickerTimer=0.42f+randomUnit(game)*0.22f;
        if (game.runOmen==RunOmen::FailingWick) game.flickerTimer+=0.20f;
        float delay=game.difficultyIndex==0?35.0f:(game.difficultyIndex==1?24.0f:15.0f);
        delay+=randomUnit(game)*(game.difficultyIndex==0?24.0f:(game.difficultyIndex==1?19.0f:12.0f));
        if (game.batteryCharge<20.0f) delay*=0.42f;
        if (game.runOmen==RunOmen::FailingWick) delay*=0.58f;
        game.nextFlicker=delay;
        playSound(game,L"flashlight_flicker.wav");
    }
}

void updateAtmosphere(Game& game,float dt) {
    game.weatherTime+=dt;
    if (game.weatherTime>7200.0f) game.weatherTime=std::fmod(game.weatherTime,7200.0f);
    const float cloudWave=0.5f+0.48f*std::sin(game.weatherTime*0.026f-Pi*0.5f)
                         +0.035f*std::sin(game.weatherTime*0.071f+0.8f);
    game.cloudCoverage=std::clamp(cloudWave,0.04f,0.96f);
    if (game.runOmen==RunOmen::VeiledMoon) game.cloudCoverage=std::max(game.cloudCoverage,0.84f);
    game.spatialAudio.enabled.store(game.soundEnabled,std::memory_order_relaxed);
    const bool activeRun=game.started && !game.paused && !game.won;
    game.spatialAudio.windLevel.store(activeRun && game.runOmen==RunOmen::HowlingWind?6.5f:1.0f,
                                      std::memory_order_relaxed);
    game.spatialAudio.bellLevel.store(activeRun && game.soundEnabled && game.runOmen==RunOmen::DistantBells?0.052f:0.0f,
                                      std::memory_order_relaxed);
    if (!game.soundEnabled || !game.started || game.paused || game.won || game.showSettings) {
        game.spatialAudio.cueVolume.store(0.0f,std::memory_order_relaxed);
        return;
    }
    const float exitRadius=boundaryRadius(game,RingCount,ExitAngle)+0.45f;
    const float dx=std::cos(ExitAngle)*exitRadius-game.x;
    const float dz=std::sin(ExitAngle)*exitRadius-game.z;
    const float distance=std::hypot(dx,dz);
    const float pan=(std::sin(game.yaw)*dx-std::cos(game.yaw)*dz)/std::max(distance,0.001f);
    game.spatialAudio.cuePan.store(pan,std::memory_order_relaxed);
    game.spatialAudio.bellPan.store(pan,std::memory_order_relaxed);
    constexpr float fadeStart=4.0f, cutoffRadius=7.0f;
    if (distance>=cutoffRadius) {
        game.spatialAudio.cueVolume.store(0.0f,std::memory_order_relaxed);
        return;
    }
    const float fadeT=std::clamp((cutoffRadius-distance)/(cutoffRadius-fadeStart),0.0f,1.0f);
    const float radiusFade=fadeT*fadeT*(3.0f-2.0f*fadeT);
    const float attenuation=1.0f/(1.0f+0.06f*distance+0.005f*distance*distance);
    game.spatialAudio.cueVolume.store(0.12f*attenuation*radiusFade,std::memory_order_relaxed);
}

void restart(Game& game) {
    regenerateMaze(game);
    game.x=game.z=game.pitch=game.elapsed=0.0f;
    game.yaw=ExitAngle;
    game.started=true;
    game.won=game.paused=false;
    game.routeVisible=game.routeCooldown=0.0f;
    game.stepDistance=0.0f;
    game.bobPhase=game.movementBob=0.0f;
    resetRunState(game);
    startTrail(game);
    captureMouse(game,true);
    SetFocus(game.window);
}

void beginGame(Game& game) {
    game.started=true;
    game.paused=false;
    game.movementBob=0.0f;
    resetRunState(game);
    startTrail(game);
    playSound(game,L"menu.wav");
    captureMouse(game,true);
    SetFocus(game.window);
}

void returnToMenu(Game& game) {
    regenerateMaze(game);
    game.x=game.z=game.pitch=game.elapsed=0.0f;
    game.yaw=ExitAngle;
    game.started=game.paused=game.won=false;
    game.routeVisible=game.routeCooldown=0.0f;
    game.stepDistance=0.0f;
    game.bobPhase=game.movementBob=0.0f;
    game.noticeMessage.clear();
    game.noticeTimer=0.0f;
    game.currentTrail.clear();
    game.trailProgress=0.0f;
    captureMouse(game,false);
    playSound(game,L"menu.wav");
}

void activateRoute(Game& game) {
    if (game.routeCooldown>0.0f || !game.started || game.paused || game.won) return;
    const int startX=static_cast<int>(std::floor((HalfMaze-game.x)/Cell));
    const int startY=static_cast<int>(std::floor((HalfMaze-game.z)/Cell));
    if (startX<0 || startY<0 || startX>=MapSize || startY>=MapSize || game.map[startY][startX]!='.') return;
    const int count=MapSize*MapSize;
    auto center=[&](int cell) {
        const int x=cell%MapSize, y=cell/MapSize;
        return Vec3{HalfMaze-(x+0.5f)*Cell,0.025f,HalfMaze-(y+0.5f)*Cell};
    };
    auto clearLine=[&](Vec3 a,Vec3 b) {
        const float length=std::hypot(b.x-a.x,b.z-a.z);
        const int steps=std::max(1,static_cast<int>(std::ceil(length/(Cell*0.5f))));
        for (int i=0;i<=steps;++i) {
            const float t=static_cast<float>(i)/steps;
            if (!canStand(game,a.x+(b.x-a.x)*t,a.z+(b.z-a.z)*t)) return false;
        }
        return true;
    };
    std::vector<uint8_t> safe(count,0);
    for (int cell=0;cell<count;++cell) if (game.map[cell/MapSize][cell%MapSize]=='.') {
        const Vec3 p=center(cell);
        safe[cell]=canStand(game,p.x,p.z);
    }
    int start=startY*MapSize+startX;
    if (!safe[start]) {
        int nearest=-1;
        float nearestDistance=0.75f*0.75f;
        const int radius=static_cast<int>(std::ceil(0.75f/Cell));
        for (int y=std::max(0,startY-radius);y<=std::min(MapSize-1,startY+radius);++y)
            for (int x=std::max(0,startX-radius);x<=std::min(MapSize-1,startX+radius);++x) {
                const int cell=y*MapSize+x;
                if (!safe[cell]) continue;
                const Vec3 p=center(cell);
                const float dx=p.x-game.x, dz=p.z-game.z, distance=dx*dx+dz*dz;
                if (distance<nearestDistance) { nearestDistance=distance; nearest=cell; }
            }
        if (nearest<0) return;
        start=nearest;
    }
    const float exitRadius=boundaryRadius(game,RingCount,ExitAngle);
    const float goalRadius=exitRadius-0.45f;
    int goal=-1;
    float goalDistance=std::numeric_limits<float>::max();
    for (int cell=0;cell<count;++cell) if (safe[cell]) {
        const Vec3 p=center(cell);
        const float wx=p.x, wz=p.z;
        const float radius=std::hypot(wx,wz), angle=std::atan2(wz,wx);
        const float angleDelta=std::atan2(std::sin(angle-ExitAngle),std::cos(angle-ExitAngle));
        if (radius<exitRadius-0.8f || std::abs(angleDelta)*radius>DoorHalfWidth-0.4f) continue;
        const float distance=(radius-goalRadius)*(radius-goalRadius)+angleDelta*angleDelta*goalRadius*goalRadius;
        if (distance<goalDistance) { goalDistance=distance; goal=cell; }
    }
    if (goal<0) return;
    auto heuristic=[&](int cell) {
        return static_cast<float>(std::abs(cell%MapSize-goal%MapSize)+std::abs(cell/MapSize-goal/MapSize));
    };
    std::vector<float> cost(count,std::numeric_limits<float>::infinity());
    std::vector<int> parent(count,-1);
    using OpenEntry=std::pair<float,int>;
    std::priority_queue<OpenEntry,std::vector<OpenEntry>,std::greater<OpenEntry>> open;
    cost[start]=0.0f; parent[start]=start; open.push({heuristic(start),start});
    constexpr int dx[4]={-1,1,0,0}, dy[4]={0,0,-1,1};
    while (!open.empty()) {
        const int cell=open.top().second;
        const float score=open.top().first;
        open.pop();
        if (cell==goal) break;
        if (score>cost[cell]+heuristic(cell)+0.001f) continue;
        const int x=cell%MapSize, y=cell/MapSize;
        for (int d=0;d<4;++d) {
            const int nx=x+dx[d], ny=y+dy[d];
            if (nx<0 || ny<0 || nx>=MapSize || ny>=MapSize) continue;
            const int next=ny*MapSize+nx;
            if (!safe[next]) continue;
            if (!clearLine(center(cell),center(next))) continue;
            const float nextCost=cost[cell]+1.0f;
            if (nextCost<cost[next]) {
                cost[next]=nextCost; parent[next]=cell;
                open.push({nextCost+heuristic(next),next});
            }
        }
    }
    if (parent[goal]<0) return;
    std::vector<int> path;
    for (int cell=goal;cell!=start;cell=parent[cell]) path.push_back(cell);
    path.push_back(start);
    std::reverse(path.begin(),path.end());
    std::vector<Vertex> vertices;
    vertices.reserve(path.size()*6);
    auto addRibbon=[&](Vec3 a,Vec3 b) {
        const float dx=b.x-a.x, dz=b.z-a.z, len=std::hypot(dx,dz);
        if (len<0.0001f) return;
        const float ox=-dz/len*0.04f, oz=dx/len*0.04f;
        addQuad(vertices,{a.x+ox,a.y,a.z+oz},{b.x+ox,b.y,b.z+oz},
                {b.x-ox,b.y,b.z-oz},{a.x-ox,a.y,a.z-oz},{0,1,0},0.06f,0.72f,0.92f,1.5f,3.0f);
    };
    const Vec3 playerPosition{game.x,0.025f,game.z}, startPosition=center(start);
    if (clearLine(playerPosition,startPosition)) addRibbon(playerPosition,startPosition);
    for (size_t i=1;i<path.size();++i) addRibbon(center(path[i-1]),center(path[i]));
    const Vec3 exit{std::cos(ExitAngle)*(exitRadius+0.45f),0.025f,std::sin(ExitAngle)*(exitRadius+0.45f)};
    if (clearLine(center(path.back()),exit)) addRibbon(center(path.back()),exit);
    glBindVertexArray(game.routeVao); glBindBuffer(GL_ARRAY_BUFFER,game.routeVbo);
    glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(vertices.size()*sizeof(Vertex)),vertices.data(),GL_DYNAMIC_DRAW);
    game.routeVertexCount=static_cast<GLsizei>(vertices.size());
    game.routeVisible=1.0f; game.routeCooldown=routeCooldownLength(game);
    ++game.routeUses;
    playSound(game,L"route.wav");
}

void toggleFlashlight(Game& game) {
    if (!game.started || game.paused || game.won) return;
    if (game.flashlightOn) game.flashlightOn=false;
    else if (game.batteryCharge>0.0f) game.flashlightOn=true;
    game.flickerTimer=0.0f;
    playSound(game,L"flashlight_switch.wav");
}

void useBattery(Game& game) {
    if (!game.started || game.paused || game.won || game.batteryCount<=0 || game.batteryCharge>=100.0f) return;
    --game.batteryCount;
    const bool wasEmpty=game.batteryCharge<=0.0f;
    game.batteryCharge=std::min(100.0f,game.batteryCharge+50.0f);
    if (wasEmpty) {
        game.flashlightOn=true;
        game.nextFlicker=8.0f+randomUnit(game)*8.0f;
    }
    playSound(game,L"battery_recharge.wav");
}

void saveSettings(const Game& game) {
    auto write=[&](const wchar_t* section,const wchar_t* key,int value) {
        const std::wstring text=std::to_wstring(value);
        WritePrivateProfileStringW(section,key,text.c_str(),game.settingsPath.c_str());
    };
    write(L"Video",L"Resolution",game.resolutionIndex);
    write(L"Video",L"Brightness",static_cast<int>(std::lround(game.brightness*100.0f)));
    write(L"Video",L"Contrast",static_cast<int>(std::lround(game.contrast*100.0f)));
    write(L"Audio",L"Sound",game.soundEnabled?1:0);
    write(L"Game",L"Difficulty",game.difficultyIndex);
    write(L"Controls",L"MouseSensitivity",static_cast<int>(std::lround(game.mouseSensitivity*100.0f)));
    write(L"Controls",L"HeadBob",game.headBobEnabled?1:0);
    WritePrivateProfileStringW(nullptr,nullptr,nullptr,game.settingsPath.c_str());
}

void setResolution(Game& game) {
    constexpr int widths[]={960,1280,1600}, heights[]={600,800,900};
    game.resolutionIndex=std::clamp(game.resolutionIndex,0,3);
    if (game.resolutionIndex==3) {
        SetWindowLongPtrW(game.window,GWL_STYLE,WS_POPUP|WS_VISIBLE);
        SetWindowPos(game.window,HWND_TOP,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),SWP_FRAMECHANGED|SWP_SHOWWINDOW);
        return;
    }
    SetWindowLongPtrW(game.window,GWL_STYLE,WS_OVERLAPPEDWINDOW|WS_VISIBLE);
    RECT rect{0,0,widths[game.resolutionIndex],heights[game.resolutionIndex]};
    AdjustWindowRect(&rect,WS_OVERLAPPEDWINDOW,FALSE);
    const int windowWidth=rect.right-rect.left, windowHeight=rect.bottom-rect.top;
    SetWindowPos(game.window,HWND_TOP,(GetSystemMetrics(SM_CXSCREEN)-windowWidth)/2,
                 (GetSystemMetrics(SM_CYSCREEN)-windowHeight)/2,windowWidth,windowHeight,
                 SWP_FRAMECHANGED|SWP_SHOWWINDOW);
}

void openSettings(Game& game) {
    game.showSettings=true;
    game.settingsIndex=0;
    captureMouse(game,false);
}

void closeSettings(Game& game) {
    game.showSettings=false;
    saveSettings(game);
}

void adjustSetting(Game& game,int direction) {
    switch (game.settingsIndex) {
    case 0:
        game.resolutionIndex=std::clamp(game.resolutionIndex+direction,0,3);
        setResolution(game);
        break;
    case 1:
        game.brightness=std::clamp(game.brightness+direction*0.1f,0.5f,1.5f);
        game.brightness=std::round(game.brightness*10.0f)/10.0f;
        break;
    case 2:
        game.contrast=std::clamp(game.contrast+direction*0.1f,0.5f,1.5f);
        game.contrast=std::round(game.contrast*10.0f)/10.0f;
        break;
    case 3: game.soundEnabled=direction>0; break;
    case 4: game.difficultyIndex=(game.difficultyIndex+direction+3)%3; break;
    case 5:
        game.mouseSensitivity=std::clamp(game.mouseSensitivity+direction*0.1f,0.5f,2.0f);
        game.mouseSensitivity=std::round(game.mouseSensitivity*10.0f)/10.0f;
        break;
    case 6: game.headBobEnabled=!game.headBobEnabled; break;
    case 7: closeSettings(game); return;
    }
    saveSettings(game);
}

LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* game=reinterpret_cast<Game*>(GetWindowLongPtrW(window,GWLP_USERDATA));
    if (message==WM_NCCREATE) {
        const auto* create=reinterpret_cast<CREATESTRUCTW*>(lParam);
        game=static_cast<Game*>(create->lpCreateParams);
        SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(game));
        if (game) game->window=window;
    }
    if (game) switch (message) {
    case WM_SIZE:
        game->width=std::max(1,static_cast<int>(LOWORD(lParam)));
        game->height=std::max(1,static_cast<int>(HIWORD(lParam)));
        return 0;
    case WM_ERASEBKGND: return 1;
    case WM_SETCURSOR:
        if (game->mouseCaptured) { SetCursor(nullptr); return TRUE; }
        break;
    case WM_LBUTTONDOWN:
        if (game->showSettings) return 0;
        if (game->showPastRuns) return 0;
        if (!game->started) beginGame(*game);
        else if (game->paused && !game->won) { game->paused=false; captureMouse(*game,true); playSound(*game,L"menu.wav"); }
        return 0;
    case WM_INPUT:
        if (game->mouseCaptured && !game->paused && !game->won) {
            RAWINPUT input{};
            UINT size=sizeof(input);
            if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam),RID_INPUT,&input,&size,sizeof(RAWINPUTHEADER))==size && input.header.dwType==RIM_TYPEMOUSE) {
                game->yaw += input.data.mouse.lLastX*0.0025f*game->mouseSensitivity;
                game->pitch=std::clamp(game->pitch-input.data.mouse.lLastY*0.0025f*game->mouseSensitivity,-1.35f,1.35f);
            }
        }
        return 0;
    case WM_KEYDOWN:
        if (!(lParam & (1LL<<30))) {
            if (game->showSettings) {
                if (wParam==VK_ESCAPE) closeSettings(*game);
                else if (wParam==VK_UP) game->settingsIndex=(game->settingsIndex+7)%8;
                else if (wParam==VK_DOWN) game->settingsIndex=(game->settingsIndex+1)%8;
                else if (wParam==VK_LEFT) adjustSetting(*game,-1);
                else if (wParam==VK_RIGHT) adjustSetting(*game,1);
                else if (wParam==VK_RETURN || wParam==VK_SPACE) {
                    if (game->settingsIndex==3) { game->soundEnabled=!game->soundEnabled; saveSettings(*game); }
                    else if (game->settingsIndex==6) { game->headBobEnabled=!game->headBobEnabled; saveSettings(*game); }
                    else if (game->settingsIndex==7) closeSettings(*game);
                }
            } else if (game->showPastRuns) {
                if (wParam==VK_ESCAPE) game->showPastRuns=false;
                else if (wParam==VK_LEFT || wParam==VK_RIGHT) {
                    const int pageCount=std::max(1,static_cast<int>((game->pastRuns.size()+9)/10));
                    const int direction=wParam==VK_RIGHT?1:-1;
                    game->pastRunsPage=(game->pastRunsPage+direction+pageCount)%pageCount;
                } else if (wParam=='S') openSettings(*game);
            } else if (wParam==VK_ESCAPE) {
                if (!game->started) DestroyWindow(window);
                else if (game->paused || game->won) returnToMenu(*game);
                else { game->paused=true; captureMouse(*game,false); playSound(*game,L"menu.wav"); }
            } else if (wParam=='S' && (!game->started || game->paused)) {
                openSettings(*game);
            } else if (wParam=='P' && !game->started) {
                game->showPastRuns=true;
                game->pastRunsPage=0;
                captureMouse(*game,false);
            } else if (!game->started && (wParam==VK_LEFT || wParam==VK_RIGHT)) {
                const int direction=wParam==VK_RIGHT?1:-1;
                game->difficultyIndex=(game->difficultyIndex+direction+3)%3;
                saveSettings(*game);
            } else if (!game->started && (wParam==VK_RETURN || wParam==VK_SPACE)) {
                beginGame(*game);
            } else if (wParam=='R' && game->won) restart(*game);
            else if (wParam=='R') useBattery(*game);
            else if (wParam=='F') toggleFlashlight(*game);
            else if (wParam=='O') activateRoute(*game);
        }
        return 0;
    case WM_KILLFOCUS:
        if (game->started && !game->won) { game->paused=true; captureMouse(*game,false); }
        return 0;
    case WM_CLOSE:
        captureMouse(*game,false);
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        game->running=false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window,message,wParam,lParam);
}

constexpr int WglMajorVersion = 0x2091;
constexpr int WglMinorVersion = 0x2092;
constexpr int WglProfileMask = 0x9126;
constexpr int WglCoreProfile = 0x00000001;
using CreateContextAttribs = HGLRC(WINAPI*)(HDC,HGLRC,const int*);
using SwapInterval = BOOL(WINAPI*)(int);

bool setPixelFormat(HDC dc) {
    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize=sizeof(pfd); pfd.nVersion=1;
    pfd.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
    pfd.iPixelType=PFD_TYPE_RGBA; pfd.cColorBits=32; pfd.cDepthBits=24;
    pfd.cStencilBits=8; pfd.iLayerType=PFD_MAIN_PLANE;
    const int format=ChoosePixelFormat(dc,&pfd);
    return format && SetPixelFormat(dc,format,&pfd);
}

bool createWindowAndContext(Game& game, HINSTANCE instance) {
    WNDCLASSEXW wc{};
    wc.cbSize=sizeof(wc); wc.style=CS_OWNDC; wc.lpfnWndProc=windowProc;
    wc.hInstance=instance; wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    wc.lpszClassName=L"MazeEscapeWindow";
    if (!RegisterClassExW(&wc) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS) return false;

    HWND dummy=CreateWindowExW(0,wc.lpszClassName,L"",WS_POPUP,0,0,1,1,nullptr,nullptr,instance,nullptr);
    if (!dummy) return false;
    HDC dummyDc=GetDC(dummy);
    if (!setPixelFormat(dummyDc)) return false;
    HGLRC dummyGl=wglCreateContext(dummyDc);
    if (!dummyGl || !wglMakeCurrent(dummyDc,dummyGl)) return false;
    auto createContext=reinterpret_cast<CreateContextAttribs>(wglGetProcAddress("wglCreateContextAttribsARB"));

    const DWORD style=game.resolutionIndex==3?WS_POPUP:WS_OVERLAPPEDWINDOW;
    RECT rect{0,0,game.width,game.height};
    if (game.resolutionIndex!=3) AdjustWindowRect(&rect,style,FALSE);
    game.window=CreateWindowExW(0,wc.lpszClassName,L"Maze Escape | Find the exit",style,
        game.resolutionIndex==3?0:CW_USEDEFAULT,game.resolutionIndex==3?0:CW_USEDEFAULT,
        rect.right-rect.left,rect.bottom-rect.top,nullptr,nullptr,instance,&game);
    if (!game.window) return false;
    game.dc=GetDC(game.window);
    if (!setPixelFormat(game.dc) || !createContext) return false;
    const int attributes[]={WglMajorVersion,3,WglMinorVersion,3,WglProfileMask,WglCoreProfile,0};
    game.gl=createContext(game.dc,nullptr,attributes);
    wglMakeCurrent(nullptr,nullptr);
    wglDeleteContext(dummyGl); ReleaseDC(dummy,dummyDc); DestroyWindow(dummy);
    if (!game.gl || !wglMakeCurrent(game.dc,game.gl)) return false;
    SwapInterval setSwapInterval=nullptr;
    if (loadProc(setSwapInterval,"wglSwapIntervalEXT")) setSwapInterval(1);

    RAWINPUTDEVICE mouse{0x01,0x02,RIDEV_INPUTSINK,game.window};
    RegisterRawInputDevices(&mouse,1,sizeof(mouse));
    ShowWindow(game.window,SW_SHOW); UpdateWindow(game.window);
    return true;
}

constexpr char WorldVertexShader[] = R"GLSL(#version 330 core
layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec3 aColor;
layout(location=3) in float aEmission;
layout(location=4) in vec2 aUv;
layout(location=5) in float aMaterial;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
out vec3 vWorld;
out vec3 vNormal;
out vec3 vColor;
out float vEmission;
out vec2 vUv;
out float vMaterial;
out vec4 vLightPosition;
uniform mat4 uLightMatrix;
void main() {
    vec4 world = uModel * vec4(aPosition, 1.0);
    vWorld = world.xyz;
    vNormal = mat3(uModel) * aNormal;
    vColor = aColor;
    vEmission = aEmission;
    vUv = aUv;
    vMaterial = aMaterial;
    vLightPosition = uLightMatrix * world;
    gl_Position = uProjection * uView * world;
})GLSL";

constexpr char ShadowVertexShader[] = R"GLSL(#version 330 core
layout(location=0) in vec3 aPosition;
uniform mat4 uLightMatrix;
uniform mat4 uModel;
void main() { gl_Position=uLightMatrix*uModel*vec4(aPosition,1.0); }
)GLSL";

constexpr char ShadowFragmentShader[] = R"GLSL(#version 330 core
void main() { }
)GLSL";

constexpr char WorldFragmentShader[] = R"GLSL(#version 330 core
in vec3 vWorld;
in vec3 vNormal;
in vec3 vColor;
in float vEmission;
in vec2 vUv;
in float vMaterial;
in vec4 vLightPosition;
uniform vec3 uEye;
uniform vec3 uFlash;
uniform vec3 uLightPos;
uniform float uFlashIntensity;
uniform float uMoonlight;
uniform float uBrightness;
uniform float uContrast;
uniform sampler2D uGrass;
uniform sampler2D uStone;
uniform sampler2D uGrassNormal;
uniform sampler2D uStoneNormal;
uniform sampler2D uGrassRoughness;
uniform sampler2D uStoneRoughness;
uniform sampler2D uGrassAO;
uniform sampler2D uStoneAO;
uniform sampler2D uPropTexture;
uniform sampler2D uShadowMap;
out vec4 outColor;
vec3 srgbToLinear(vec3 color) { return pow(max(color,vec3(0.0)),vec3(2.2)); }
vec3 mappedNormal(vec3 normal,vec3 position,vec2 uv,vec3 sampleNormal) {
    vec3 dp1=dFdx(position), dp2=dFdy(position);
    vec2 duv1=dFdx(uv), duv2=dFdy(uv);
    vec3 tangent=normalize(dp1*duv2.y-dp2*duv1.y);
    vec3 bitangent=normalize(-dp1*duv2.x+dp2*duv1.x);
    float handedness=sign(dot(cross(normal,tangent),bitangent));
    bitangent=normalize(cross(normal,tangent))*handedness;
    return normalize(mat3(tangent,bitangent,normal)*(sampleNormal*2.0-1.0));
}
float shadowVisibility(vec3 normal,vec3 lightDirection) {
    vec3 projected=vLightPosition.xyz/vLightPosition.w;
    projected=projected*0.5+0.5;
    if (projected.z>1.0 || any(lessThan(projected.xy,vec2(0.0))) || any(greaterThan(projected.xy,vec2(1.0)))) return 1.0;
    float bias=max(0.0016*(1.0-dot(normal,lightDirection)),0.00055);
    vec2 texel=1.0/vec2(textureSize(uShadowMap,0));
    float visible=0.0;
    for (int x=-1;x<=1;++x) for (int y=-1;y<=1;++y) {
        float depth=texture(uShadowMap,projected.xy+vec2(x,y)*texel).r;
        visible+=projected.z-bias<=depth?1.0:0.0;
    }
    return visible/9.0;
}
void main() {
    vec3 toPoint = vWorld - uLightPos;
    float distanceToEye = length(vWorld - uEye);
    float distanceToLight = length(toPoint);
    float cone = dot(normalize(toPoint), normalize(uFlash));
    float spot = smoothstep(0.66, 0.90, cone);
    vec3 lightDirection=normalize(uLightPos-vWorld);
    vec3 normal=normalize(vNormal);
    vec3 albedo = vColor;
    float alpha=1.0;
    float roughness=0.88;
    float ambientOcclusion=1.0;
    if (vMaterial > 3.5) {
        vec4 texel=texture(uPropTexture,vUv);
        if (texel.a<0.18) discard;
        albedo*=srgbToLinear(texel.rgb);
        alpha=texel.a;
        roughness=0.52;
    } else if (vMaterial < 0.5) {
        vec2 uv=vUv*0.62;
        vec3 fineGrass=srgbToLinear(texture(uGrass,uv).rgb);
        vec3 broadGrass=srgbToLinear(texture(uGrass,uv*0.5+vec2(0.17,0.31)).rgb);
        albedo*=mix(fineGrass,broadGrass,0.18)*0.94;
        normal=mappedNormal(normal,vWorld,uv,texture(uGrassNormal,uv).rgb);
        roughness=clamp(texture(uGrassRoughness,uv).r,0.35,1.0);
        ambientOcclusion=texture(uGrassAO,uv).r;
    } else if (vMaterial < 1.5) {
        vec2 uv=vUv*0.78;
        vec3 detail=srgbToLinear(texture(uStone,uv).rgb);
        vec3 fineStone=srgbToLinear(texture(uStone,uv*1.65).rgb);
        vec3 source=mix(detail,fineStone,0.12);
        vec3 grayStone=pow(max(source,vec3(0.0)),vec3(0.76))*vec3(0.88,0.94,1.0);
        vec2 patchCell=floor(vUv*1.1);
        vec2 patchUv=fract(vUv*1.1);
        float seed = fract(sin(dot(patchCell,vec2(127.1,311.7)))*43758.5453);
        vec2 center = vec2(fract(seed*17.13),fract(seed*39.71));
        float grassCoverage=(1.0-smoothstep(0.12,0.31,length((patchUv-center)*vec2(1.0,0.72))))*step(0.70,seed);
        grassCoverage*=0.38+0.62*(1.0-smoothstep(0.18,1.35,vWorld.y));
        vec3 grass=srgbToLinear(texture(uGrass,vUv*0.62).rgb);
        vec3 moss=grass*vec3(0.38,0.72,0.24);
        albedo = mix(grayStone,moss,grassCoverage);
        normal=mappedNormal(normal,vWorld,uv*1.65,texture(uStoneNormal,uv*1.65).rgb);
        roughness=clamp(texture(uStoneRoughness,uv).r,0.25,1.0);
        ambientOcclusion=texture(uStoneAO,uv).r;
    }
    float diffuse=max(dot(normal,lightDirection),0.0);
    float attenuation=1.0/(1.0+0.055*distanceToLight+0.012*distanceToLight*distanceToLight);
    float shadow=vMaterial>3.5?1.0:shadowVisibility(normal,lightDirection);
    float contact=vMaterial>0.5 && vMaterial<1.5?mix(0.70,1.0,smoothstep(0.0,0.38,vWorld.y)):1.0;
    vec3 moonFill=vMaterial<0.5?vec3(0.019,0.026,0.018):vec3(0.024,0.034,0.052);
    moonFill*=mix(0.0,4.0,uMoonlight);
    moonFill*=contact*mix(0.58,1.0,ambientOcclusion);
    vec3 halfway=normalize(lightDirection+normalize(uEye-vWorld));
    float specular=pow(max(dot(normal,halfway),0.0),mix(96.0,5.0,roughness));
    vec3 specularColor=mix(vec3(0.10,0.095,0.075),vec3(0.24,0.28,0.34),1.0-roughness);
    vec3 direct=vec3(1.0,0.91,0.78)*spot*attenuation*shadow*uFlashIntensity;
    vec3 lit=albedo*(moonFill+direct*(0.14+1.12*diffuse))+specularColor*specular*direct*pow(1.0-roughness,2.0)*0.55+vColor*vEmission;
    float fog=smoothstep(14.0,27.0,distanceToEye);
    vec3 color=mix(lit,vec3(0.0015,0.003,0.009),fog);
    color=max((color-vec3(0.5))*uContrast+vec3(0.5),vec3(0.0))*uBrightness;
    color=pow(max(color,vec3(0.0)),vec3(1.0/2.2));
    outColor = vec4(color, alpha);
})GLSL";

constexpr char SkyVertexShader[] = R"GLSL(#version 330 core
layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aUv;
uniform mat4 uView;
uniform mat4 uProjection;
uniform vec3 uEye;
uniform float uBrightness;
uniform float uContrast;
out vec2 vUv;
void main() {
    vec4 clip = uProjection * uView * vec4(aPosition * 60.0 + uEye, 1.0);
    gl_Position = clip.xyww;
    vUv = aUv;
})GLSL";

constexpr char SkyFragmentShader[] = R"GLSL(#version 330 core
in vec2 vUv;
uniform sampler2D uSky;
uniform float uBrightness;
uniform float uContrast;
uniform float uWeatherTime;
uniform float uCloudCoverage;
uniform float uMoonU;
uniform float uMoonlight;
out vec4 outColor;
float hash(vec2 p) { return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453); }
float noise(vec2 p) {
    vec2 i=floor(p),f=fract(p); f=f*f*(3.0-2.0*f);
    return mix(mix(hash(i),hash(i+vec2(1.0,0.0)),f.x),
               mix(hash(i+vec2(0.0,1.0)),hash(i+vec2(1.0,1.0)),f.x),f.y);
}
float cloudNoise(vec2 p) {
    float value=0.0,amplitude=0.52;
    for (int octave=0;octave<4;++octave) {
        value+=amplitude*noise(p);
        p=p*2.03+vec2(19.17,7.31);
        amplitude*=0.5;
    }
    return value;
}
void main() {
    vec3 sampled=texture(uSky,vUv).rgb;
    vec3 night=vec3(0.001,0.002,0.008)+sampled*0.025;
    vec2 grid=vUv*vec2(480.0,240.0), cell=floor(grid), local=fract(grid);
    float seed=hash(cell);
    float star=(step(0.9975,seed))*(1.0-smoothstep(0.035,0.10,length(local-vec2(hash(cell+1.3),hash(cell+5.7)))));
    float deltaU=abs(vUv.x-uMoonU); deltaU=min(deltaU,1.0-deltaU);
    vec2 moonDelta=vec2(deltaU*1.25,vUv.y-0.755);
    float moonDistance=length(moonDelta);
    float moon=1.0-smoothstep(0.018,0.021,moonDistance);
    float halo=exp(-moonDistance*82.0)*0.075;
    vec2 cloudUv=vec2(vUv.x*9.0+uWeatherTime*0.0012,
                      vUv.y*5.7+sin(vUv.x*8.0+uWeatherTime*0.0007)*0.07);
    float density=cloudNoise(cloudUv);
    float threshold=mix(0.78,0.40,uCloudCoverage);
    float clouds=smoothstep(threshold,threshold+0.13,density)*smoothstep(0.38,0.55,vUv.y);
    night+=vec3(0.22,0.28,0.43)*star*(1.0-clouds)*(1.0-0.65*uCloudCoverage);
    float moonVisibility=1.0-max(clouds,1.0-uMoonlight);
    night+=vec3(0.65,0.72,0.88)*(moon*moonVisibility+halo*moonVisibility);
    vec3 cloudColor=mix(vec3(0.0025,0.0045,0.010),vec3(0.075,0.095,0.13),0.14+0.86*(1.0-uCloudCoverage));
    night=mix(night,cloudColor,clouds*0.88);
    vec3 color=max((night-vec3(0.5))*uContrast+vec3(0.5),vec3(0.0))*uBrightness;
    color=pow(max(color,vec3(0.0)),vec3(1.0/2.2));
    outColor=vec4(color,1.0);
}
)GLSL";

constexpr char UiVertexShader[] = R"GLSL(#version 330 core
layout(location=0) in vec2 aPosition;
layout(location=1) in vec4 aColor;
out vec4 vColor;
void main() { gl_Position=vec4(aPosition,0.0,1.0); vColor=aColor; }
)GLSL";

constexpr char UiFragmentShader[] = R"GLSL(#version 330 core
in vec4 vColor;
out vec4 outColor;
void main() { outColor=vColor; }
)GLSL";

void configureVertexArray(GLuint vao, GLuint vbo, bool ui) {
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    if (ui) {
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(UiVertex),reinterpret_cast<void*>(offsetof(UiVertex,x)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,sizeof(UiVertex),reinterpret_cast<void*>(offsetof(UiVertex,r)));
    } else {
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,x)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,nx)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,r)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3,1,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,emission)));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,u)));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5,1,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,material)));
    }
}

bool loadPpm(const std::filesystem::path& path, int& width, int& height, std::vector<uint8_t>& pixels) {
    std::ifstream file(path,std::ios::binary);
    std::string magic;
    int maxValue=0;
    if (!(file>>magic>>width>>height>>maxValue) || magic!="P6" || maxValue!=255 || width<=0 || height<=0) return false;
    file.get();
    pixels.resize(static_cast<size_t>(width)*height*3);
    return static_cast<bool>(file.read(reinterpret_cast<char*>(pixels.data()),static_cast<std::streamsize>(pixels.size())));
}

GLuint loadTexture(const std::filesystem::path& path, GLint wrapS, GLint wrapT) {
    int width=0,height=0;
    std::vector<uint8_t> pixels;
    if (!loadPpm(path,width,height,pixels)) return 0;
    GLuint texture=0;
    glGenTextures(1,&texture);
    glBindTexture(GL_TEXTURE_2D,texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,wrapS);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,wrapT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,width,height,0,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    return texture;
}

bool createShadowResources(Game& game) {
    constexpr int resolution=2048;
    glGenTextures(1,&game.shadowTexture);
    glBindTexture(GL_TEXTURE_2D,game.shadowTexture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24,resolution,resolution,0,GL_DEPTH_COMPONENT,GL_FLOAT,nullptr);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
    const float border[]={1.0f,1.0f,1.0f,1.0f};
    glTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_BORDER_COLOR,border);
    glGenFramebuffers(1,&game.shadowFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER,game.shadowFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,game.shadowTexture,0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    const bool complete=glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    glDrawBuffer(GL_BACK);
    return complete;
}

GLuint loadImageTexture(const std::filesystem::path& path,GLint wrapS=GL_REPEAT,GLint wrapT=GL_REPEAT,
                        int channels=4,bool flipRows=true) {
    Gdiplus::Bitmap image(path.c_str());
    if (image.GetLastStatus()!=Gdiplus::Ok) return 0;
    const int width=static_cast<int>(image.GetWidth()), height=static_cast<int>(image.GetHeight());
    if (width<=0 || height<=0) return 0;
    Gdiplus::Rect rect(0,0,width,height);
    Gdiplus::BitmapData data{};
    if (image.LockBits(&rect,Gdiplus::ImageLockModeRead,PixelFormat32bppARGB,&data)!=Gdiplus::Ok) return 0;
    const size_t rowBytes=static_cast<size_t>(width)*4;
    if ((channels!=1 && channels!=3 && channels!=4) || data.Stride==0 || static_cast<size_t>(std::abs(data.Stride))<rowBytes) {
        image.UnlockBits(&data);
        return 0;
    }
    std::vector<uint8_t> pixels(static_cast<size_t>(width)*height*channels);
    const auto* firstRow=static_cast<const uint8_t*>(data.Scan0);
    for (int y=0;y<height;++y) {
        const int sourceY=flipRows?height-1-y:y;
        const auto* source=firstRow+static_cast<ptrdiff_t>(sourceY)*data.Stride;
        for (int x=0;x<width;++x) {
            const size_t sourceIndex=static_cast<size_t>(x)*4;
            const size_t targetIndex=(static_cast<size_t>(y)*width+x)*channels;
            if (channels==1) pixels[targetIndex]=source[sourceIndex+2];
            else if (channels==3) {
                pixels[targetIndex]=source[sourceIndex+2];
                pixels[targetIndex+1]=source[sourceIndex+1];
                pixels[targetIndex+2]=source[sourceIndex];
            } else std::memcpy(pixels.data()+targetIndex,source+sourceIndex,4);
        }
    }
    GLuint texture=0;
    glGenTextures(1,&texture);
    glBindTexture(GL_TEXTURE_2D,texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,wrapS);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,wrapT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    const GLint internalFormat=channels==1?GL_R8:(channels==3?GL_RGB:GL_RGBA);
    const GLenum format=channels==1?GL_RED:(channels==3?GL_RGB:GL_BGRA);
    glTexImage2D(GL_TEXTURE_2D,0,internalFormat,width,height,0,format,GL_UNSIGNED_BYTE,pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    image.UnlockBits(&data);
    return texture;
}

bool loadObjModel(Game& game,const std::filesystem::path& path,ModelObject& output) {
    struct Index { int position=-1, uv=-1, normal=-1; };
    struct Group { std::vector<Vertex> vertices; };
    std::ifstream file(path);
    if (!file) return false;
    std::vector<Vec3> positions,normals;
    std::vector<std::array<float,2>> uvs;
    std::map<std::string,Group> groups;
    std::string material="default", materialLibrary;
    auto indexOf=[](int index,size_t size) -> int {
        const int result=index>0?index-1:static_cast<int>(size)+index;
        return result>=0 && result<static_cast<int>(size)?result:-1;
    };
    std::string line;
    while (std::getline(file,line)) {
        std::istringstream row(line);
        std::string key;
        row>>key;
        if (key=="v") {
            Vec3 p{}; row>>p.x>>p.y>>p.z; positions.push_back(p);
        } else if (key=="vt") {
            std::array<float,2> uv{}; row>>uv[0]>>uv[1]; uv[1]=1.0f-uv[1]; uvs.push_back(uv);
        } else if (key=="vn") {
            Vec3 n{}; row>>n.x>>n.y>>n.z; normals.push_back(normalize(n));
        } else if (key=="usemtl") {
            row>>material;
        } else if (key=="mtllib") {
            std::getline(row,materialLibrary);
            const size_t first=materialLibrary.find_first_not_of(" \t");
            if (first!=std::string::npos) materialLibrary.erase(0,first);
        } else if (key=="f") {
            std::vector<Index> face;
            std::string token;
            while (row>>token) {
                Index idx;
                const size_t first=token.find('/');
                const size_t second=first==std::string::npos?std::string::npos:token.find('/',first+1);
                try {
                    idx.position=indexOf(std::stoi(token.substr(0,first)),positions.size());
                    if (first!=std::string::npos && second!=first+1)
                        idx.uv=indexOf(std::stoi(token.substr(first+1,second==std::string::npos?std::string::npos:second-first-1)),uvs.size());
                    if (second!=std::string::npos && second+1<token.size())
                        idx.normal=indexOf(std::stoi(token.substr(second+1)),normals.size());
                } catch (...) { idx.position=-1; }
                if (idx.position>=0) face.push_back(idx);
            }
            for (size_t i=1;i+1<face.size();++i) {
                const Index tri[3]={face[0],face[i],face[i+1]};
                const Vec3 a=positions[tri[0].position], b=positions[tri[1].position], c=positions[tri[2].position];
                const Vec3 fallback=normalize(cross({b.x-a.x,b.y-a.y,b.z-a.z},{c.x-a.x,c.y-a.y,c.z-a.z}));
                constexpr std::array<float,3> tint{0.9f,0.94f,1.0f};
                auto& vertices=groups[material].vertices;
                for (const Index& idx:tri) {
                    const Vec3 p=positions[idx.position];
                    const Vec3 n=idx.normal>=0?normals[idx.normal]:fallback;
                    const auto uv=idx.uv>=0?uvs[idx.uv]:std::array<float,2>{0.0f,0.0f};
                    vertices.push_back({p.x,p.y,p.z,n.x,n.y,n.z,tint[0],tint[1],tint[2],0.04f,uv[0],uv[1],4.0f});
                }
            }
        }
    }
    if (positions.empty() || groups.empty()) return false;

    std::unordered_map<std::string,std::filesystem::path> materialTextures;
    if (!materialLibrary.empty()) {
        std::ifstream mtl(path.parent_path()/std::filesystem::path(materialLibrary));
        std::string current, key;
        while (mtl>>key) {
            if (key=="newmtl") mtl>>current;
            else if (key=="map_Kd") {
                std::string imageName; std::getline(mtl,imageName);
                const size_t first=imageName.find_first_not_of(" \t");
                if (first!=std::string::npos) {
                    imageName.erase(0,first);
                    materialTextures[current]=path.parent_path()/std::filesystem::path(imageName);
                }
            } else { std::string rest; std::getline(mtl,rest); }
        }
    }
    std::vector<Vertex> vertices;
    for (auto& entry:groups) {
        if (entry.second.vertices.empty()) continue;
        ModelPart part;
        part.first=static_cast<GLsizei>(vertices.size());
        part.count=static_cast<GLsizei>(entry.second.vertices.size());
        part.material=entry.first;
        const auto texturePath=materialTextures.find(entry.first);
        if (texturePath!=materialTextures.end() && std::filesystem::exists(texturePath->second)) {
            const auto key=texturePath->second.wstring();
            const auto cached=game.flashlightTextures.find(key);
            if (cached!=game.flashlightTextures.end()) part.texture=cached->second;
            else {
                // The OBJ loader flips vt above to compensate for top-down images. Keep its historical upload order.
                part.texture=loadImageTexture(texturePath->second,GL_REPEAT,GL_REPEAT,4,false);
                game.flashlightTextures.emplace(key,part.texture);
            }
        }
        vertices.insert(vertices.end(),entry.second.vertices.begin(),entry.second.vertices.end());
        output.parts.push_back(part);
    }
    if (vertices.empty()) return false;
    glGenVertexArrays(1,&output.vao); glGenBuffers(1,&output.vbo);
    configureVertexArray(output.vao,output.vbo,false);
    glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(vertices.size()*sizeof(Vertex)),vertices.data(),GL_STATIC_DRAW);
    output.loaded=true;
    return true;
}

std::vector<SkyVertex> buildSky() {
    constexpr int rings=32, slices=64;
    std::vector<SkyVertex> vertices;
    vertices.reserve(rings*slices*6);
    auto point=[](int ring,int slice) {
        const float latitude=-Pi*0.5f+Pi*ring/rings;
        const float longitude=2.0f*Pi*slice/slices;
        return SkyVertex{std::cos(latitude)*std::cos(longitude),std::sin(latitude),
                         std::cos(latitude)*std::sin(longitude),
                         static_cast<float>(slice)/slices,static_cast<float>(ring)/rings};
    };
    for (int ring=0;ring<rings;++ring) for (int slice=0;slice<slices;++slice) {
        const SkyVertex a=point(ring,slice), b=point(ring,slice+1);
        const SkyVertex c=point(ring+1,slice+1), d=point(ring+1,slice);
        vertices.insert(vertices.end(),{a,b,c,a,c,d});
    }
    return vertices;
}

bool initializeRenderer(Game& game) {
    if (!glGetString(GL_VERSION) || !loadGl()) return false;
    game.worldProgram=makeProgram(WorldVertexShader,WorldFragmentShader);
    game.skyProgram=makeProgram(SkyVertexShader,SkyFragmentShader);
    game.uiProgram=makeProgram(UiVertexShader,UiFragmentShader);
    game.shadowProgram=makeProgram(ShadowVertexShader,ShadowFragmentShader);
    if (!game.worldProgram || !game.skyProgram || !game.uiProgram || !game.shadowProgram) return false;
    game.viewLoc=glGetUniformLocation(game.worldProgram,"uView");
    game.projectionLoc=glGetUniformLocation(game.worldProgram,"uProjection");
    game.modelLoc=glGetUniformLocation(game.worldProgram,"uModel");
    game.eyeLoc=glGetUniformLocation(game.worldProgram,"uEye");
    game.flashLoc=glGetUniformLocation(game.worldProgram,"uFlash");
    game.lightPosLoc=glGetUniformLocation(game.worldProgram,"uLightPos");
    game.brightnessLoc=glGetUniformLocation(game.worldProgram,"uBrightness");
    game.contrastLoc=glGetUniformLocation(game.worldProgram,"uContrast");
    game.flashIntensityLoc=glGetUniformLocation(game.worldProgram,"uFlashIntensity");
    game.moonlightLoc=glGetUniformLocation(game.worldProgram,"uMoonlight");
    game.shadowMatrixLoc=glGetUniformLocation(game.worldProgram,"uLightMatrix");
    game.shadowMapLoc=glGetUniformLocation(game.worldProgram,"uShadowMap");

    game.depthMatrixLoc=glGetUniformLocation(game.shadowProgram,"uLightMatrix");
    game.depthModelLoc=glGetUniformLocation(game.shadowProgram,"uModel");

    game.skyViewLoc=glGetUniformLocation(game.skyProgram,"uView");
    game.skyProjectionLoc=glGetUniformLocation(game.skyProgram,"uProjection");
    game.skyEyeLoc=glGetUniformLocation(game.skyProgram,"uEye");
    game.skySamplerLoc=glGetUniformLocation(game.skyProgram,"uSky");
    game.skyBrightnessLoc=glGetUniformLocation(game.skyProgram,"uBrightness");
    game.skyContrastLoc=glGetUniformLocation(game.skyProgram,"uContrast");
    game.skyWeatherLoc=glGetUniformLocation(game.skyProgram,"uWeatherTime");
    game.skyCloudLoc=glGetUniformLocation(game.skyProgram,"uCloudCoverage");
    game.skyMoonLoc=glGetUniformLocation(game.skyProgram,"uMoonU");
    game.skyMoonlightLoc=glGetUniformLocation(game.skyProgram,"uMoonlight");

    wchar_t executable[MAX_PATH]{};
    GetModuleFileNameW(nullptr,executable,MAX_PATH);
    const auto textureDir=std::filesystem::path(executable).parent_path()/L"assets"/L"textures";
    const auto pbrDir=textureDir/L"pbr";
    game.grassTexture=loadImageTexture(pbrDir/L"forest_ground_diffuse_2k.jpg",GL_REPEAT,GL_REPEAT,3);
    game.grassNormalTexture=loadImageTexture(pbrDir/L"forest_ground_normal_2k.jpg",GL_REPEAT,GL_REPEAT,3);
    game.grassRoughnessTexture=loadImageTexture(pbrDir/L"forest_ground_roughness_2k.jpg",GL_REPEAT,GL_REPEAT,1);
    game.grassAoTexture=loadImageTexture(pbrDir/L"forest_ground_ao_2k.jpg",GL_REPEAT,GL_REPEAT,1);
    game.stoneTexture=loadImageTexture(pbrDir/L"stone_wall_diffuse_2k.jpg",GL_REPEAT,GL_REPEAT,3);
    game.stoneNormalTexture=loadImageTexture(pbrDir/L"stone_wall_normal_2k.jpg",GL_REPEAT,GL_REPEAT,3);
    game.stoneRoughnessTexture=loadImageTexture(pbrDir/L"stone_wall_roughness_2k.jpg",GL_REPEAT,GL_REPEAT,1);
    game.stoneAoTexture=loadImageTexture(pbrDir/L"stone_wall_ao_2k.jpg",GL_REPEAT,GL_REPEAT,1);
    game.skyTexture=loadTexture(textureDir/L"night_sky.ppm",GL_REPEAT,GL_CLAMP_TO_EDGE);
    if (!game.grassTexture || !game.grassNormalTexture || !game.grassRoughnessTexture || !game.grassAoTexture ||
        !game.stoneTexture || !game.stoneNormalTexture || !game.stoneRoughnessTexture || !game.stoneAoTexture ||
        !game.skyTexture) return false;

    const auto vertices=buildMaze(game);
    game.worldVertexCount=static_cast<GLsizei>(vertices.size());
    glGenVertexArrays(1,&game.worldVao); glGenBuffers(1,&game.worldVbo);
    configureVertexArray(game.worldVao,game.worldVbo,false);
    glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(vertices.size()*sizeof(Vertex)),vertices.data(),GL_STATIC_DRAW);

    glGenVertexArrays(1,&game.routeVao); glGenBuffers(1,&game.routeVbo);
    configureVertexArray(game.routeVao,game.routeVbo,false);

    const auto skyVertices=buildSky();
    game.skyVertexCount=static_cast<GLsizei>(skyVertices.size());
    glGenVertexArrays(1,&game.skyVao); glGenBuffers(1,&game.skyVbo);
    glBindVertexArray(game.skyVao); glBindBuffer(GL_ARRAY_BUFFER,game.skyVbo);
    glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(skyVertices.size()*sizeof(SkyVertex)),skyVertices.data(),GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(SkyVertex),reinterpret_cast<void*>(offsetof(SkyVertex,x)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(SkyVertex),reinterpret_cast<void*>(offsetof(SkyVertex,u)));

    glGenVertexArrays(1,&game.uiVao); glGenBuffers(1,&game.uiVbo);
    configureVertexArray(game.uiVao,game.uiVbo,true);

    glUseProgram(game.worldProgram);
    glUniform1i(glGetUniformLocation(game.worldProgram,"uGrass"),0);
    glUniform1i(glGetUniformLocation(game.worldProgram,"uStone"),1);
    glUniform1i(glGetUniformLocation(game.worldProgram,"uGrassNormal"),2);
    glUniform1i(glGetUniformLocation(game.worldProgram,"uStoneNormal"),3);
    glUniform1i(glGetUniformLocation(game.worldProgram,"uPropTexture"),3);
    glUniform1i(game.shadowMapLoc,4);
    glUniform1i(glGetUniformLocation(game.worldProgram,"uGrassRoughness"),5);
    glUniform1i(glGetUniformLocation(game.worldProgram,"uStoneRoughness"),6);
    glUniform1i(glGetUniformLocation(game.worldProgram,"uGrassAO"),7);
    glUniform1i(glGetUniformLocation(game.worldProgram,"uStoneAO"),8);
    glUseProgram(game.skyProgram);
    glUniform1i(game.skySamplerLoc,2);
    if (!createShadowResources(game)) return false;
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    const auto modelDir=std::filesystem::path(executable).parent_path()/L"assets"/L"models";
    loadObjModel(game,modelDir/L"flashlight"/L"Flashlight.obj",game.flashlight);
    return true;
}

void movePlayer(Game& game, float dt) {
    if (!game.started || game.paused || game.won) return;
    game.routeVisible=std::max(0.0f,game.routeVisible-dt);
    game.routeCooldown=std::max(0.0f,game.routeCooldown-dt);
    game.noticeTimer=std::max(0.0f,game.noticeTimer-dt);
    if (game.noticeTimer<=0.0f) game.noticeMessage.clear();
    game.elapsed+=dt;
    updateFlashlight(game,dt);
    const float forward=(GetAsyncKeyState('W')<0?1.0f:0.0f)-(GetAsyncKeyState('S')<0?1.0f:0.0f);
    const float side=(GetAsyncKeyState('D')<0?1.0f:0.0f)-(GetAsyncKeyState('A')<0?1.0f:0.0f);
    if (forward==0.0f && side==0.0f) {
        game.stepDistance=0.0f;
        game.movementBob=std::max(0.0f,game.movementBob-dt*7.0f);
        collectNearbyFinds(game);
        return;
    }
    const float length=std::sqrt(forward*forward+side*side);
    const float f=forward/length, s=side/length;
    const bool sprinting=GetAsyncKeyState(VK_SHIFT)<0;
    const float speed=sprinting?4.6f:2.35f;
    const float dx=(std::cos(game.yaw)*f-std::sin(game.yaw)*s)*speed*dt;
    const float dz=(std::sin(game.yaw)*f+std::cos(game.yaw)*s)*speed*dt;
    const float oldX=game.x, oldZ=game.z;
    const int steps=std::max(1,static_cast<int>(std::ceil(std::max(std::abs(dx),std::abs(dz))/(Cell*0.5f))));
    const float stepX=dx/steps, stepZ=dz/steps;
    for (int i=0;i<steps;++i) {
        if (canStand(game,game.x+stepX,game.z)) game.x+=stepX;
        if (canStand(game,game.x,game.z+stepZ)) game.z+=stepZ;
    }
    const float moved=std::hypot(game.x-oldX,game.z-oldZ);
    game.trailProgress+=moved;
    while (game.trailProgress>=0.55f && game.currentTrail.size()<2400) {
        game.currentTrail.push_back({game.x,game.z,game.yaw});
        game.trailProgress-=0.55f;
    }
    if (game.currentTrail.size()>=2400) game.trailProgress=0.0f;
    game.stepDistance+=moved;
    game.movementBob=std::clamp(game.movementBob+(moved>0.0001f?dt*8.0f:-dt*7.0f),0.0f,1.0f);
    if (moved>0.0001f) game.bobPhase=std::fmod(game.bobPhase+dt*(sprinting?13.0f:10.0f),2.0f*Pi);
    const float stepLength=sprinting?0.9f:0.68f;
    if (game.stepDistance>=stepLength) {
        game.stepDistance-=stepLength;
        if (game.routeVisible<=0.0f)
            playSound(game,game.alternateFootstep?L"step2.wav":L"step1.wav");
        game.alternateFootstep=!game.alternateFootstep;
    }
    collectNearbyFinds(game);

    const float radius=std::hypot(game.x,game.z);
    const float angle=std::atan2(game.z,game.x);
    const float delta=std::atan2(std::sin(angle-ExitAngle),std::cos(angle-ExitAngle));
    const float exitRadius=boundaryRadius(game,RingCount,ExitAngle);
    if (radius>=exitRadius+0.10f && std::abs(delta)*exitRadius<=DoorHalfWidth+0.45f) {
        game.won=true;
        game.completedSeed=game.runSeed;
        savePreviousTrail(game);
        recordPastRun(game);
        if (game.bestTime<=0.0f || game.elapsed<game.bestTime) {
            game.bestTime=game.elapsed;
            game.bestSeed=game.completedSeed;
            game.newRecord=true;
            saveRunRecord(game);
        }
        regenerateMaze(game);
        captureMouse(game,false);
        playSound(game,L"escape.wav");
    }
}

void uiRect(std::vector<UiVertex>& out, int screenW, int screenH, float x, float y,
            float w, float h, std::array<float,4> color) {
    const float x0=2.0f*x/screenW-1.0f, x1=2.0f*(x+w)/screenW-1.0f;
    const float y0=1.0f-2.0f*y/screenH, y1=1.0f-2.0f*(y+h)/screenH;
    const UiVertex a{x0,y0,color[0],color[1],color[2],color[3]}, b{x1,y0,color[0],color[1],color[2],color[3]};
    const UiVertex c{x1,y1,color[0],color[1],color[2],color[3]}, d{x0,y1,color[0],color[1],color[2],color[3]};
    out.insert(out.end(),{a,b,c,a,c,d});
}

constexpr std::array<std::array<uint8_t,5>,36> Font={{
    {{0x7e,0x11,0x11,0x11,0x7e}},{{0x7f,0x49,0x49,0x49,0x36}},{{0x3e,0x41,0x41,0x41,0x22}},
    {{0x7f,0x41,0x41,0x22,0x1c}},{{0x7f,0x49,0x49,0x49,0x41}},{{0x7f,0x09,0x09,0x09,0x01}},
    {{0x3e,0x41,0x49,0x49,0x7a}},{{0x7f,0x08,0x08,0x08,0x7f}},{{0x00,0x41,0x7f,0x41,0x00}},
    {{0x20,0x40,0x41,0x3f,0x01}},{{0x7f,0x08,0x14,0x22,0x41}},{{0x7f,0x40,0x40,0x40,0x40}},
    {{0x7f,0x02,0x0c,0x02,0x7f}},{{0x7f,0x04,0x08,0x10,0x7f}},{{0x3e,0x41,0x41,0x41,0x3e}},
    {{0x7f,0x09,0x09,0x09,0x06}},{{0x3e,0x41,0x51,0x21,0x5e}},{{0x7f,0x09,0x19,0x29,0x46}},
    {{0x26,0x49,0x49,0x49,0x32}},{{0x01,0x01,0x7f,0x01,0x01}},{{0x3f,0x40,0x40,0x40,0x3f}},
    {{0x1f,0x20,0x40,0x20,0x1f}},{{0x7f,0x20,0x18,0x20,0x7f}},{{0x63,0x14,0x08,0x14,0x63}},
    {{0x03,0x04,0x78,0x04,0x03}},{{0x61,0x51,0x49,0x45,0x43}},{{0x3e,0x45,0x49,0x51,0x3e}},
    {{0x00,0x42,0x7f,0x40,0x00}},{{0x42,0x61,0x51,0x49,0x46}},{{0x21,0x41,0x45,0x4b,0x31}},
    {{0x18,0x14,0x12,0x7f,0x10}},{{0x27,0x45,0x45,0x45,0x39}},{{0x3c,0x4a,0x49,0x49,0x30}},
    {{0x01,0x71,0x09,0x05,0x03}},{{0x36,0x49,0x49,0x49,0x36}},{{0x06,0x49,0x49,0x29,0x1e}}
}};

int glyphIndex(char c) {
    if (c>='A' && c<='Z') return c-'A';
    if (c>='0' && c<='9') return 26+c-'0';
    return -1;
}

void uiText(std::vector<UiVertex>& out, int screenW, int screenH, const std::string& text,
            float x, float y, float scale, std::array<float,4> color) {
    for (char c:text) {
        const int index=glyphIndex(c);
        if (index>=0) {
            for (int col=0;col<5;++col) for (int row=0;row<7;++row) {
                if (Font[index][col] & (1u<<row))
                    uiRect(out,screenW,screenH,x+col*scale,y+row*scale,scale,scale,color);
            }
        } else if (c==':') {
            uiRect(out,screenW,screenH,x+2*scale,y+2*scale,scale,scale,color);
            uiRect(out,screenW,screenH,x+2*scale,y+5*scale,scale,scale,color);
        } else if (c=='!') {
            uiRect(out,screenW,screenH,x+2*scale,y,scale,4*scale,color);
            uiRect(out,screenW,screenH,x+2*scale,y+6*scale,scale,scale,color);
        }
        x+=7.0f*scale;
    }
}

void centeredText(std::vector<UiVertex>& out, int w, int h, const std::string& text,
                  float y, float scale, std::array<float,4> color) {
    uiText(out,w,h,text,(w-text.size()*7.0f*scale+scale)/2.0f,y,scale,color);
}

void renderUi(Game& game) {
    std::vector<UiVertex> vertices;
    const int w=game.width, h=game.height;
    const std::array<float,4> white{0.82f,0.88f,0.90f,1.0f};
    const std::array<float,4> cyan{0.35f,0.85f,0.82f,1.0f};
    const std::array<float,4> green{0.35f,1.0f,0.72f,1.0f};
    if (game.showSettings) {
        const float cx=w*0.5f, cy=h*0.5f;
        uiRect(vertices,w,h,0,0,static_cast<float>(w),static_cast<float>(h),{0.002f,0.006f,0.010f,0.84f});
        uiRect(vertices,w,h,cx-300,cy-285,600,570,{0.012f,0.025f,0.030f,0.98f});
        uiRect(vertices,w,h,cx-300,cy-285,600,5,{0.25f,0.88f,0.67f,1.0f});
        centeredText(vertices,w,h,"SETTINGS",cy-195,3.0f,green);
        constexpr int widths[]={960,1280,1600}, heights[]={600,800,900};
        const std::string resolution=game.resolutionIndex==3?"FULLSCREEN":
            std::to_string(widths[game.resolutionIndex])+"X"+std::to_string(heights[game.resolutionIndex]);
        const char* difficultyNames[]={"EASY","NORMAL","HARD"};
        const std::array<std::string,8> rows={
            "RESOLUTION "+resolution,
            "BRIGHTNESS "+std::to_string(static_cast<int>(std::lround(game.brightness*100.0f))),
            "CONTRAST "+std::to_string(static_cast<int>(std::lround(game.contrast*100.0f))),
            std::string("SOUND ")+(game.soundEnabled?"ON":"OFF"),
            std::string("DIFFICULTY ")+difficultyNames[game.difficultyIndex],
            "MOUSE SENS "+std::to_string(static_cast<int>(std::lround(game.mouseSensitivity*100.0f))),
            std::string("HEAD BOB ")+(game.headBobEnabled?"ON":"OFF"),
            "BACK"
        };
        for (int i=0;i<static_cast<int>(rows.size());++i) {
            const float y=cy-166+i*38.0f;
            if (i==game.settingsIndex) uiRect(vertices,w,h,cx-265,y-6,530,31,{0.08f,0.32f,0.28f,0.95f});
            centeredText(vertices,w,h,rows[i],y,1.55f,i==game.settingsIndex?green:white);
        }
        centeredText(vertices,w,h,"UP DOWN SELECT",cy+155,1.25f,white);
        centeredText(vertices,w,h,"LEFT RIGHT ADJUST",cy+180,1.25f,cyan);
        centeredText(vertices,w,h,"ESC BACK",cy+207,1.1f,white);
    } else if (!game.started && game.showPastRuns) {
        const float cx=w*0.5f, cy=h*0.5f;
        const int pageCount=std::max(1,static_cast<int>((game.pastRuns.size()+9)/10));
        game.pastRunsPage=std::clamp(game.pastRunsPage,0,pageCount-1);
        uiRect(vertices,w,h,0,0,static_cast<float>(w),static_cast<float>(h),{0.002f,0.006f,0.010f,0.82f});
        uiRect(vertices,w,h,cx-470,cy-270,940,540,{0.012f,0.025f,0.030f,0.98f});
        uiRect(vertices,w,h,cx-470,cy-270,940,5,{0.25f,0.88f,0.67f,1.0f});
        centeredText(vertices,w,h,"PAST RUNS",cy-235,3.2f,green);
        centeredText(vertices,w,h,"COMPLETION TIME   RUN TIME   ROUTE FINDS   OMEN   MAZE SEED",cy-193,1.05f,cyan);
        if (game.pastRuns.empty()) {
            centeredText(vertices,w,h,"NO COMPLETED RUNS YET",cy-70,1.7f,white);
        } else {
            for (int i=0;i<10;++i) {
                const size_t reverseIndex=static_cast<size_t>(game.pastRunsPage*10+i);
                if (reverseIndex>=game.pastRuns.size()) break;
                const PastRun& run=game.pastRuns[game.pastRuns.size()-1-reverseIndex];
                char runTime[32]{}, routeCount[16]{};
                formatTime(run.elapsed,runTime);
                wsprintfA(routeCount,"%02d",run.routeUses);
                const std::string row=run.timestamp+"  TIME "+runTime+"  ROUTE "+routeCount+
                                      "  OMEN "+runOmenName(run.omen)+"  SEED "+std::to_string(run.seed);
                uiRect(vertices,w,h,cx-430,cy-168+i*29.0f,860,25,
                       {0.025f,0.060f,0.064f,(i%2==0)?0.70f:0.38f});
                uiText(vertices,w,h,row,cx-418,cy-162+i*29.0f,1.05f,white);
            }
        }
        char pageText[40]{};
        wsprintfA(pageText,"PAGE %02d OF %02d",game.pastRunsPage+1,pageCount);
        centeredText(vertices,w,h,pageText,cy+163,1.2f,green);
        centeredText(vertices,w,h,"LEFT RIGHT CHANGE PAGE",cy+190,1.05f,cyan);
        centeredText(vertices,w,h,"ESC BACK   S SETTINGS",cy+215,1.05f,white);
    } else if (!game.started) {
        const float cx=w*0.5f, cy=h*0.5f;
        uiRect(vertices,w,h,0,0,static_cast<float>(w),static_cast<float>(h),{0.002f,0.006f,0.010f,0.78f});
        uiRect(vertices,w,h,cx-270,cy-170,540,340,{0.012f,0.025f,0.030f,0.97f});
        uiRect(vertices,w,h,cx-270,cy-170,540,5,{0.25f,0.88f,0.67f,1.0f});
        centeredText(vertices,w,h,"MAZE ESCAPE",cy-129,3.5f,green);
        centeredText(vertices,w,h,"FIND THE EXIT",cy-83,1.8f,white);
        if (game.bestTime>0.0f) {
            char record[32]{}; formatTime(game.bestTime,record);
            centeredText(vertices,w,h,std::string("BEST ")+record,cy-58,1.0f,green);
        } else centeredText(vertices,w,h,"NO RECORD YET",cy-58,1.0f,white);
        centeredText(vertices,w,h,"SEED "+std::to_string(game.mazeSeed),cy-39,0.95f,cyan);
        centeredText(vertices,w,h,std::string("OMEN ")+runOmenName(game.runOmen),cy-20,0.95f,cyan);
        const char* difficultyNames[]={"EASY","NORMAL","HARD"};
        centeredText(vertices,w,h,std::string("DIFFICULTY ")+difficultyNames[game.difficultyIndex]+" LEFT RIGHT",
                     cy,0.90f,white);
        uiRect(vertices,w,h,cx-190,cy+12,380,52,{0.08f,0.32f,0.28f,1.0f});
        centeredText(vertices,w,h,"ENTER OR CLICK TO PLAY",cy+30,1.3f,green);
        centeredText(vertices,w,h,"P PAST RUNS",cy+83,1.15f,cyan);
        centeredText(vertices,w,h,"S SETTINGS",cy+111,1.15f,cyan);
        centeredText(vertices,w,h,"ESC TO CLOSE",cy+139,1.0f,cyan);
    } else if (game.won) {
        uiRect(vertices,w,h,w*0.5f-310,h*0.5f-152,620,304,{0.004f,0.012f,0.018f,0.92f});
        centeredText(vertices,w,h,"YOU ESCAPED!",h*0.5f-126,3.6f,green);
        char timeText[32]{}; formatTime(game.elapsed,timeText);
        centeredText(vertices,w,h,std::string("TIME ")+timeText,h*0.5f-73,1.8f,white);
        centeredText(vertices,w,h,"SEED "+std::to_string(game.completedSeed),h*0.5f-39,1.25f,cyan);
        if (game.newRecord) centeredText(vertices,w,h,"NEW PERSONAL BEST",h*0.5f-10,1.45f,green);
        char routeCount[32]{}; wsprintfA(routeCount,"ROUTE REVEALS %02d",game.routeUses);
        centeredText(vertices,w,h,routeCount,h*0.5f+20,1.25f,white);
        centeredText(vertices,w,h,"R NEW MAZE",h*0.5f+54,1.5f,cyan);
        centeredText(vertices,w,h,"ESC RETURN TO TITLE",h*0.5f+82,1.15f,white);
        centeredText(vertices,w,h,"P PAST RUNS FROM TITLE",h*0.5f+108,1.0f,cyan);
    } else if (game.paused) {
        uiRect(vertices,w,h,0,0,static_cast<float>(w),static_cast<float>(h),{0.002f,0.006f,0.010f,0.72f});
        centeredText(vertices,w,h,"PAUSED",h*0.5f-48,4.0f,white);
        centeredText(vertices,w,h,"CLICK TO RESUME",h*0.5f+12,1.8f,cyan);
        centeredText(vertices,w,h,"S SETTINGS",h*0.5f+46,1.5f,cyan);
        centeredText(vertices,w,h,"ESC TO TITLE",h*0.5f+78,1.35f,white);
    } else {
        uiRect(vertices,w,h,16,16,248,224,{0.004f,0.010f,0.016f,0.74f});
        uiText(vertices,w,h,"MAZE ESCAPE",28,26,1.7f,cyan);
        char timeText[32]{}; formatTime(game.elapsed,timeText);
        uiText(vertices,w,h,std::string("TIME ")+timeText,28,47,1.7f,white);
        uiText(vertices,w,h,"WASD MOVE",28,73,1.25f,white);
        uiText(vertices,w,h,"MOUSE LOOK",28,92,1.25f,white);
        uiText(vertices,w,h,"SHIFT SPRINT",28,109,1.25f,white);
        uiText(vertices,w,h,game.flashlightOn?"F LIGHT ON":"F LIGHT OFF",28,127,1.25f,game.flashlightOn?green:white);
        char batteryText[32]{};
        wsprintfA(batteryText,"CHARGE %03d",static_cast<int>(std::ceil(game.batteryCharge)));
        uiText(vertices,w,h,batteryText,28,146,1.2f,game.batteryCharge>20.0f?green:white);
        uiRect(vertices,w,h,28,166,170,7,{0.12f,0.16f,0.17f,1.0f});
        uiRect(vertices,w,h,28,166,170.0f*std::clamp(game.batteryCharge/100.0f,0.0f,1.0f),7,
               game.batteryCharge>20.0f?std::array<float,4>{0.15f,0.86f,0.55f,1.0f}:std::array<float,4>{0.95f,0.36f,0.18f,1.0f});
        char inventoryText[32]{}; wsprintfA(inventoryText,"CELLS %02d",game.batteryCount);
        uiText(vertices,w,h,inventoryText,28,181,1.15f,game.batteryCount>0?green:white);
        uiText(vertices,w,h,"R USE BATTERY",28,201,1.15f,game.batteryCount>0?cyan:white);
        const float skillX=std::max(12.0f,w-174.0f), skillY=static_cast<float>(h)-68.0f;
        uiRect(vertices,w,h,skillX,skillY,162,52,{0.004f,0.010f,0.016f,0.78f});
        uiText(vertices,w,h,"ROUTE",skillX+12,skillY+8,1.15f,cyan);
        char skillText[32]{};
        if (game.routeCooldown>0.0f) wsprintfA(skillText,"O %02dS",static_cast<int>(std::ceil(game.routeCooldown)));
        else lstrcpyA(skillText,"O READY");
        uiText(vertices,w,h,skillText,skillX+12,skillY+29,1.35f,game.routeCooldown>0.0f?white:green);
        if (game.noticeTimer>0.0f && !game.noticeMessage.empty()) {
            uiRect(vertices,w,h,w*0.5f-390,static_cast<float>(h)-124.0f,780,40,{0.004f,0.012f,0.018f,0.88f});
            centeredText(vertices,w,h,game.noticeMessage,static_cast<float>(h)-112.0f,0.95f,cyan);
        }
        uiRect(vertices,w,h,w*0.5f-5,h*0.5f-1,10,2,{0.42f,0.87f,0.82f,0.9f});
        uiRect(vertices,w,h,w*0.5f-1,h*0.5f-5,2,10,{0.42f,0.87f,0.82f,0.9f});
    }
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(game.uiProgram);
    glBindVertexArray(game.uiVao); glBindBuffer(GL_ARRAY_BUFFER,game.uiVbo);
    glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(vertices.size()*sizeof(UiVertex)),vertices.data(),GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(vertices.size()));
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

std::array<float,16> basisTransform(Vec3 right,Vec3 up,Vec3 forward,Vec3 origin,float scale) {
    return {right.x*scale,right.y*scale,right.z*scale,0.0f,
            up.x*scale,up.y*scale,up.z*scale,0.0f,
            forward.x*scale,forward.y*scale,forward.z*scale,0.0f,
            origin.x,origin.y,origin.z,1.0f};
}

void drawModel(Game& game,const ModelObject& object,const std::array<float,16>& transform) {
    if (!object.loaded || object.parts.empty()) return;
    glUniformMatrix4fv(game.modelLoc,1,GL_FALSE,transform.data());
    glBindVertexArray(object.vao);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE3);
    for (const ModelPart& part:object.parts) {
        glBindTexture(GL_TEXTURE_2D,part.texture);
        glDrawArrays(GL_TRIANGLES,part.first,part.count);
    }
    glDisable(GL_BLEND);
}

void render(Game& game) {
    const float bob=game.headBobEnabled?std::sin(game.bobPhase*2.0f)*0.025f*game.movementBob:0.0f;
    Vec3 eye{game.x,1.38f+bob,game.z};
    const float cp=std::cos(game.pitch);
    const Vec3 flash{std::cos(game.yaw)*cp,std::sin(game.pitch),std::sin(game.yaw)*cp};
    Vec3 heldRight{},heldUp{},heldForward{},heldOrigin{};
    float heldScale=0.085f;
    heldForward={std::cos(game.yaw)*cp,std::sin(game.pitch),std::sin(game.yaw)*cp};
    heldRight=normalize(cross(heldForward,{0.0f,1.0f,0.0f}));
    heldUp=cross(heldRight,heldForward);
    const Vec3 playerRight{std::sin(game.yaw),0.0f,-std::cos(game.yaw)};
    const Vec3 playerForward{std::cos(game.yaw),0.0f,std::sin(game.yaw)};
    const Vec3 grip{game.x-playerRight.x*0.134f+playerForward.x*0.236f,
                    1.063f+bob,
                    game.z-playerRight.z*0.134f+playerForward.z*0.236f};
    heldOrigin={grip.x+heldForward.x*0.169f,grip.y+heldForward.y*0.169f,grip.z+heldForward.z*0.169f};
    // Keep the flashlight beam aligned with the view when rotating the prop in the grip.
    heldRight={-heldRight.x,-heldRight.y,-heldRight.z};
    heldUp={-heldUp.x,-heldUp.y,-heldUp.z};
    const Vec3 lightPos{heldOrigin.x+heldForward.x*heldScale*2.871889f,
                        heldOrigin.y+heldForward.y*heldScale*2.871889f,
                        heldOrigin.z+heldForward.z*heldScale*2.871889f};
    const auto view=viewMatrix(eye,game.yaw,game.pitch);
    const auto projection=perspective(70.0f*Pi/180.0f,static_cast<float>(game.width)/game.height,0.05f,70.0f);
    const std::array<float,16> model{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    const auto lightView=viewMatrix(lightPos,game.yaw,game.pitch);
    const auto lightProjection=perspective(98.0f*Pi/180.0f,1.0f,0.12f,32.0f);
    const auto lightMatrix=multiplyMatrix(lightProjection,lightView);

    glBindFramebuffer(GL_FRAMEBUFFER,game.shadowFramebuffer);
    glViewport(0,0,2048,2048);
    glClear(GL_DEPTH_BUFFER_BIT);
    glUseProgram(game.shadowProgram);
    glUniformMatrix4fv(game.depthMatrixLoc,1,GL_FALSE,lightMatrix.data());
    glUniformMatrix4fv(game.depthModelLoc,1,GL_FALSE,model.data());
    glBindVertexArray(game.worldVao);
    glDrawArrays(GL_TRIANGLES,0,game.worldVertexCount);
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    glDrawBuffer(GL_BACK);

    glViewport(0,0,game.width,game.height);
    glClearColor(0.004f,0.007f,0.012f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    const float cloudPhase=std::clamp((game.cloudCoverage-0.25f)/0.71f,0.0f,1.0f);
    const float moonlight=1.0f-cloudPhase*cloudPhase*(3.0f-2.0f*cloudPhase);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(game.skyProgram);
    glUniformMatrix4fv(game.skyViewLoc,1,GL_FALSE,view.data());
    glUniformMatrix4fv(game.skyProjectionLoc,1,GL_FALSE,projection.data());
    glUniform3f(game.skyEyeLoc,eye.x,eye.y,eye.z);
    glUniform1f(game.skyBrightnessLoc,game.brightness);
    glUniform1f(game.skyContrastLoc,game.contrast);
    glUniform1f(game.skyWeatherLoc,game.weatherTime);
    glUniform1f(game.skyCloudLoc,game.cloudCoverage);
    glUniform1f(game.skyMoonLoc,std::fmod(0.72f+game.weatherTime*0.000035f,1.0f));
    glUniform1f(game.skyMoonlightLoc,moonlight);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D,game.skyTexture);
    glBindVertexArray(game.skyVao);
    glDrawArrays(GL_TRIANGLES,0,game.skyVertexCount);

    glEnable(GL_DEPTH_TEST);
    glUseProgram(game.worldProgram);
    glUniformMatrix4fv(game.viewLoc,1,GL_FALSE,view.data());
    glUniformMatrix4fv(game.projectionLoc,1,GL_FALSE,projection.data());
    glUniformMatrix4fv(game.modelLoc,1,GL_FALSE,model.data());
    glUniform3f(game.eyeLoc,eye.x,eye.y,eye.z);
    glUniform3f(game.flashLoc,flash.x,flash.y,flash.z);
    glUniform3f(game.lightPosLoc,lightPos.x,lightPos.y,lightPos.z);
    glUniform1f(game.brightnessLoc,game.brightness);
    glUniform1f(game.contrastLoc,game.contrast);
    float flickerIntensity=1.0f;
    if (game.flickerTimer>0.0f) {
        const float phase=std::fmod(game.flickerTimer,0.12f)/0.12f;
        flickerIntensity=phase<0.45f?0.018f:(phase<0.70f?0.34f:0.90f);
    }
    const float flashIntensity=game.flashlightOn && game.batteryCharge>0.0f?flickerIntensity:0.0f;
    glUniform1f(game.moonlightLoc,moonlight);
    glUniform1f(game.flashIntensityLoc,flashIntensity);
    glUniformMatrix4fv(game.shadowMatrixLoc,1,GL_FALSE,lightMatrix.data());
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,game.grassTexture);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D,game.stoneTexture);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D,game.grassNormalTexture);
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D,game.stoneNormalTexture);
    glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D,game.shadowTexture);
    glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D,game.grassRoughnessTexture);
    glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D,game.stoneRoughnessTexture);
    glActiveTexture(GL_TEXTURE7); glBindTexture(GL_TEXTURE_2D,game.grassAoTexture);
    glActiveTexture(GL_TEXTURE8); glBindTexture(GL_TEXTURE_2D,game.stoneAoTexture);
    glBindVertexArray(game.worldVao);
    glDrawArrays(GL_TRIANGLES,0,game.worldVertexCount);
    if (game.routeVisible>0.0f && game.routeVertexCount>0) {
        glBindVertexArray(game.routeVao);
        glDrawArrays(GL_TRIANGLES,0,game.routeVertexCount);
    }
    if (game.started && game.flashlight.loaded) {
        const auto heldLight=basisTransform(heldRight,heldUp,heldForward,heldOrigin,heldScale);
        drawModel(game,game.flashlight,heldLight);
    }
    renderUi(game);
    SwapBuffers(game.dc);
}

void readSeedArgument(Game& game) {
    const std::wstring command=GetCommandLineW();
    const size_t option=command.find(L"--seed");
    if (option==std::wstring::npos) return;
    size_t value=option+6;
    if (value<command.size() && command[value]!=L'=' && !std::iswspace(command[value])) return;
    if (value<command.size() && command[value]==L'=') ++value;
    while (value<command.size() && std::iswspace(command[value])) ++value;
    wchar_t* end=nullptr;
    const unsigned long parsed=std::wcstoul(command.c_str()+value,&end,10);
    if (end==command.c_str()+value || parsed>std::numeric_limits<uint32_t>::max()) return;
    game.requestedSeed=static_cast<uint32_t>(parsed);
    game.hasRequestedSeed=true;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int) {
    Gdiplus::GdiplusStartupInput gdiplusInput;
    ULONG_PTR gdiplusToken=0;
    if (Gdiplus::GdiplusStartup(&gdiplusToken,&gdiplusInput,nullptr)!=Gdiplus::Ok) {
        MessageBoxW(nullptr,L"Could not initialize image support.",L"Maze Escape",MB_OK|MB_ICONERROR);
        return 1;
    }
    Game game;
    readSeedArgument(game);
    if (!loadMap(game)) {
        MessageBoxW(nullptr,L"Could not generate the maze layout.",L"Maze Escape",MB_OK|MB_ICONERROR);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return 1;
    }
    loadPastRuns(game);
    loadPreviousTrail(game);
    if (!createWindowAndContext(game,instance)) {
        MessageBoxW(nullptr,L"Could not create an OpenGL 3.3 window/context.",L"Maze Escape",MB_OK|MB_ICONERROR);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return 1;
    }
    if (!initializeRenderer(game)) {
        MessageBoxW(game.window,L"OpenGL 3.3 entry points or shaders could not be initialized.",L"Maze Escape",MB_OK|MB_ICONERROR);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return 1;
    }
    game.spatialAudio.start();
    using Clock=std::chrono::steady_clock;
    auto previous=Clock::now();
    MSG message{};
    while (game.running) {
        while (PeekMessageW(&message,nullptr,0,0,PM_REMOVE)) {
            if (message.message==WM_QUIT) game.running=false;
            TranslateMessage(&message); DispatchMessageW(&message);
        }
        if (!game.running) break;
        const auto now=Clock::now();
        const float dt=std::min(std::chrono::duration<float>(now-previous).count(),0.05f);
        previous=now;
        updateAtmosphere(game,dt);
        movePlayer(game,dt);
        render(game);
    }
    game.spatialAudio.stop();
    PlaySoundW(nullptr,nullptr,0);
    captureMouse(game,false);
    if (wglGetCurrentContext()==game.gl) wglMakeCurrent(nullptr,nullptr);
    if (game.gl) wglDeleteContext(game.gl);
    if (game.dc && game.window) ReleaseDC(game.window,game.dc);
    if (game.window && IsWindow(game.window)) DestroyWindow(game.window);
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return 0;
}
