#include <windows.h>
#include <objidl.h>
#include <GL/gl.h>
#include <mmsystem.h>
#include <gdiplus.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
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
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
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

constexpr int MapSize = 488;
constexpr float Cell = 0.0625f;
constexpr float HalfMaze = MapSize * Cell * 0.5f;
constexpr float ExitAngle = 94.0f * 3.14159265f / 180.0f;
constexpr float Pi = 3.14159265f;

struct Vertex {
    float x, y, z;
    float nx, ny, nz;
    float r, g, b, emission;
    float u, v, material;
};
struct SkyVertex { float x, y, z, u, v; };
struct UiVertex { float x, y, r, g, b, a; };
struct Vec3 { float x, y, z; };
struct ModelPart { GLsizei first = 0, count = 0; GLuint texture = 0; std::string material; };
struct ModelObject { GLuint vao = 0, vbo = 0; std::vector<ModelPart> parts; bool loaded = false; };

struct Game {
    HWND window = nullptr;
    HDC dc = nullptr;
    HGLRC gl = nullptr;
    int width = 1280, height = 800;
    std::vector<std::string> map;
    std::filesystem::path executableDirectory;
    std::filesystem::path settingsPath;
    GLsizei worldVertexCount = 0;
    GLsizei skyVertexCount = 0;
    GLsizei routeVertexCount = 0;
    float routeVisible = 0.0f, routeCooldown = 0.0f;
    float stepDistance = 0.0f;
    float x = 0.0f, z = 0.0f;
    float yaw = ExitAngle, pitch = 0.0f;
    float elapsed = 0.0f;
    bool started = false, paused = false, won = false, mouseCaptured = false, running = true;
    bool showSettings = false, settingsFromPause = false, soundEnabled = true, thirdPerson = false;
    float walkPhase = 0.0f, walking = 0.0f;
    int settingsIndex = 0, resolutionIndex = 1;
    float brightness = 1.0f, contrast = 1.0f;
    bool alternateFootstep = false;
    GLuint worldProgram = 0, skyProgram = 0, uiProgram = 0;
    GLuint worldVao = 0, worldVbo = 0, routeVao = 0, routeVbo = 0;
    GLuint skyVao = 0, skyVbo = 0, uiVao = 0, uiVbo = 0;
    ModelObject character, flashlight;
    std::vector<ModelObject> characterWalk;
    std::unordered_map<std::wstring,GLuint> modelTextures;
    GLuint grassTexture = 0, stoneTexture = 0, skyTexture = 0;
    GLint viewLoc = -1, projectionLoc = -1, modelLoc = -1, eyeLoc = -1, flashLoc = -1, lightPosLoc = -1;
    GLint brightnessLoc = -1, contrastLoc = -1;
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
#undef LOAD_GL
    return true;
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

void addBox(std::vector<Vertex>& out, float x0, float x1, float y0, float y1,
            float z0, float z1, float r, float g, float b, float emission, float material = 1.0f) {
    addQuad(out,{x0,y1,z0},{x1,y1,z0},{x1,y1,z1},{x0,y1,z1},{0,1,0},r,g,b,emission,material);
    addQuad(out,{x0,y0,z0},{x0,y0,z1},{x1,y0,z1},{x1,y0,z0},{0,-1,0},r,g,b,emission,material);
    addQuad(out,{x0,y0,z1},{x0,y1,z1},{x1,y1,z1},{x1,y0,z1},{0,0,1},r,g,b,emission,material);
    addQuad(out,{x0,y0,z0},{x1,y0,z0},{x1,y1,z0},{x0,y1,z0},{0,0,-1},r,g,b,emission,material);
    addQuad(out,{x1,y0,z0},{x1,y0,z1},{x1,y1,z1},{x1,y1,z0},{1,0,0},r,g,b,emission,material);
    addQuad(out,{x0,y0,z0},{x0,y1,z0},{x0,y1,z1},{x0,y0,z1},{-1,0,0},r,g,b,emission,material);
}

bool wallAt(const Game& game, int x, int y) {
    return x < 0 || y < 0 || x >= MapSize || y >= MapSize || game.map[y][x] == '#';
}

std::vector<Vertex> buildMaze(const Game& game) {
    std::vector<Vertex> out;
    out.reserve(MapSize * MapSize * 5);
    const float wallHeight = 4.0f;
    const float floorEdge = HalfMaze + 0.2f;
    addQuad(out,{-floorEdge,0,-floorEdge},{floorEdge,0,-floorEdge},
            {floorEdge,0,floorEdge},{-floorEdge,0,floorEdge},{0,1,0},0.92f,0.96f,0.88f,0.0f,0.0f);

    for (int y=0; y<MapSize; ++y) for (int x=0; x<MapSize; ++x) {
        if (!wallAt(game,x,y)) continue;
        const float x0=(MapSize*0.5f-x-1)*Cell, x1=x0+Cell;
        const float z1=(MapSize*0.5f-y)*Cell, z0=z1-Cell;
        const float r=0.86f, g=0.88f, b=0.86f;
        addQuad(out,{x0,wallHeight,z0},{x1,wallHeight,z0},{x1,wallHeight,z1},{x0,wallHeight,z1},{0,1,0},r,g,b);
    }

    // Marching squares follows the source image's curved wall boundaries instead of stair-stepping each map cell.
    for (int y=0; y<MapSize-1; ++y) for (int x=0; x<MapSize-1; ++x) {
        const bool wall[4]={wallAt(game,x,y),wallAt(game,x+1,y),wallAt(game,x+1,y+1),wallAt(game,x,y+1)};
        int edge[4], crossings=0;
        constexpr float edgeU[4]={0.5f,1.0f,0.5f,0.0f};
        constexpr float edgeV[4]={0.0f,0.5f,1.0f,0.5f};
        for (int i=0;i<4;++i) if (wall[i]!=wall[(i+1)%4]) edge[crossings++]=i;
        auto makePoint=[&](int e) {
            const float u=edgeU[e], v=edgeV[e];
            return Vec3{HalfMaze-(x+0.5f+u)*Cell,0.0f,HalfMaze-(y+0.5f+v)*Cell};
        };
        auto addBoundary=[&](int e0,int e1) {
            const Vec3 a=makePoint(e0), b=makePoint(e1);
            const float u=(edgeU[e0]+edgeU[e1])*0.5f, v=(edgeV[e0]+edgeV[e1])*0.5f;
            const float q00=wall[0]?1.0f:0.0f, q10=wall[1]?1.0f:0.0f;
            const float q11=wall[2]?1.0f:0.0f, q01=wall[3]?1.0f:0.0f;
            const float du=(q10-q00)*(1-v)+(q11-q01)*v;
            const float dv=(q01-q00)*(1-u)+(q11-q10)*u;
            Vec3 normal=normalize({du,0.0f,dv});
            if (normal.x==0.0f && normal.z==0.0f) normal={1.0f,0.0f,0.0f};
            Vec3 c=b, d=a; c.y=d.y=wallHeight;
            addQuad(out,a,b,c,d,normal,0.86f,0.88f,0.86f);
        };
        if (crossings==2) addBoundary(edge[0],edge[1]);
        else if (crossings==4) {
            const bool centerWall=(static_cast<int>(wall[0])+static_cast<int>(wall[1])+
                                   static_cast<int>(wall[2])+static_cast<int>(wall[3]))>=2;
            for (int corner=0;corner<4;++corner) if (wall[corner]!=centerWall)
                addBoundary((corner+3)%4,corner);
        }
    }

    const float gateX=std::cos(ExitAngle)*15.35f, gateZ=std::sin(ExitAngle)*15.35f;
    addBox(out,gateX-0.67f,gateX-0.52f,0,1.85f,gateZ-0.12f,gateZ+0.12f,0.10f,0.88f,0.72f,1.35f,2.0f);
    addBox(out,gateX+0.52f,gateX+0.67f,0,1.85f,gateZ-0.12f,gateZ+0.12f,0.10f,0.88f,0.72f,1.35f,2.0f);
    addBox(out,gateX-0.67f,gateX+0.67f,1.72f,1.88f,gateZ-0.12f,gateZ+0.12f,0.10f,0.88f,0.72f,1.35f,2.0f);
    return out;
}

bool generateMaze(Game& game) {
    struct CellNode { int ring, sector; };
    constexpr int RingCount=15;
    auto sectorsForRing=[](int ring) {
        if (ring==0) return 1;
        if (ring<=2) return 8;
        if (ring<=4) return 16;
        if (ring<=8) return 32;
        return 64;
    };
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
    std::mt19937 random(randomDevice());
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

    constexpr float CenterRadius=1.15f, RadialStep=0.92f;
    constexpr float WallHalfThickness=0.055f, DoorHalfWidth=0.42f;
    const float outerRadius=CenterRadius+RadialStep*RingCount;
    auto wrap=[&](float angle) { return std::atan2(std::sin(angle),std::cos(angle)); };
    auto hasPassage=[&](int a,int b) {
        const auto& edges=passages[a];
        return std::find(edges.begin(),edges.end(),b)!=edges.end();
    };
    game.map.assign(MapSize,std::string(MapSize,'#'));
    for (int row=0;row<MapSize;++row) for (int col=0;col<MapSize;++col) {
        const float x=HalfMaze-(col+0.5f)*Cell;
        const float z=HalfMaze-(row+0.5f)*Cell;
        const float radius=std::hypot(x,z), angle=std::atan2(z,x);
        bool floor=false;
        if (radius>=outerRadius) {
            floor=std::abs(wrap(angle-ExitAngle))*outerRadius<DoorHalfWidth && radius<outerRadius+0.26f;
        } else if (radius<CenterRadius) {
            floor=true;
        } else {
            const int ring=std::clamp(static_cast<int>(std::ceil((radius-CenterRadius)/RadialStep)),1,RingCount);
            const int sectors=sectorsForRing(ring);
            float turn=angle/(2.0f*Pi);
            if (turn<0.0f) turn+=1.0f;
            const float sectorPosition=turn*sectors;
            const int sector=std::min(sectors-1,static_cast<int>(sectorPosition));
            const int cellId=rings[ring][sector];
            floor=true;

            for (int boundary=0;boundary<RingCount;++boundary) {
                const float boundaryRadius=CenterRadius+boundary*RadialStep;
                if (std::abs(radius-boundaryRadius)>=WallHalfThickness) continue;
                const int outerRing=boundary+1;
                const int outerSectors=sectorsForRing(outerRing);
                const int outerSector=std::min(outerSectors-1,static_cast<int>(turn*outerSectors));
                const int outerId=rings[outerRing][outerSector];
                const float outerCenter=(outerSector+0.5f)*2.0f*Pi/outerSectors;
                const int innerSectors=sectorsForRing(boundary);
                const int parentSector=std::min(innerSectors-1,static_cast<int>((outerSector+0.5f)*innerSectors/outerSectors));
                const int innerId=rings[boundary][parentSector];
                const float opening=std::min(DoorHalfWidth,0.42f*(2.0f*Pi*boundaryRadius/outerSectors));
                if (!hasPassage(outerId,innerId) || std::abs(wrap(angle-outerCenter))*boundaryRadius>=opening)
                    floor=false;
                break;
            }

            const float fraction=sectorPosition-std::floor(sectorPosition);
            const float angularDistance=std::min(fraction,1.0f-fraction)*(2.0f*Pi*radius/sectors);
            if (ring>0 && angularDistance<WallHalfThickness) {
                const int edgeBefore=(fraction<0.5f?(sector+sectors-1)%sectors:sector);
                const int other=(edgeBefore+1)%sectors;
                const int edgeA=rings[ring][edgeBefore], edgeB=rings[ring][other];
                const float middleRadius=CenterRadius+(ring-0.5f)*RadialStep;
                if (!hasPassage(edgeA,edgeB) || std::abs(radius-middleRadius)>DoorHalfWidth)
                    floor=false;
            }
        }
        if (radius>=outerRadius-WallHalfThickness && radius<outerRadius+WallHalfThickness) {
            if (std::abs(wrap(angle-ExitAngle))*outerRadius>=DoorHalfWidth) floor=false;
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
    game.resolutionIndex=std::clamp(static_cast<int>(GetPrivateProfileIntW(L"Video",L"Resolution",1,game.settingsPath.c_str())),0,3);
    game.brightness=std::clamp(GetPrivateProfileIntW(L"Video",L"Brightness",100,game.settingsPath.c_str())/100.0f,0.5f,1.5f);
    game.contrast=std::clamp(GetPrivateProfileIntW(L"Video",L"Contrast",100,game.settingsPath.c_str())/100.0f,0.5f,1.5f);
    game.soundEnabled=GetPrivateProfileIntW(L"Audio",L"Sound",1,game.settingsPath.c_str())!=0;
    game.thirdPerson=GetPrivateProfileIntW(L"Camera",L"ThirdPerson",0,game.settingsPath.c_str())!=0;
    constexpr int widths[]={960,1280,1600}, heights[]={600,800,900};
    game.width=game.resolutionIndex==3?GetSystemMetrics(SM_CXSCREEN):widths[game.resolutionIndex];
    game.height=game.resolutionIndex==3?GetSystemMetrics(SM_CYSCREEN):heights[game.resolutionIndex];
    return generateMaze(game);
}

bool atFloor(const Game& game, float x, float z) {
    const int col=static_cast<int>(std::floor((HalfMaze-x)/Cell));
    const int row=static_cast<int>(std::floor((HalfMaze-z)/Cell));
    if (col>=0 && row>=0 && col<MapSize && row<MapSize) return game.map[row][col]=='.';
    const float radius=std::hypot(x,z);
    const float angle=std::atan2(z,x);
    const float delta=std::atan2(std::sin(angle-ExitAngle),std::cos(angle-ExitAngle));
    return radius<16.0f && std::abs(delta)<0.11f;
}

bool canStand(const Game& game, float x, float z) {
    constexpr float radius=0.15f;
    if (!atFloor(game,x,z)) return false;
    for (int i=0;i<12;++i) {
        const float a=2.0f*Pi*i/12.0f;
        if (!atFloor(game,x+std::cos(a)*radius,z+std::sin(a)*radius)) return false;
    }
    return true;
}

void formatTime(float elapsed, char* out) {
    const int seconds=static_cast<int>(elapsed);
    wsprintfA(out,"%02d:%02d",seconds/60,seconds%60);
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

void restart(Game& game) {
    game.x=game.z=game.pitch=game.elapsed=0.0f;
    game.yaw=ExitAngle;
    game.started=true;
    game.won=game.paused=false;
    game.routeVisible=game.routeCooldown=0.0f;
    game.stepDistance=0.0f;
    game.walkPhase=game.walking=0.0f;
    captureMouse(game,true);
    SetFocus(game.window);
}

void beginGame(Game& game) {
    game.started=true;
    game.paused=false;
    game.walking=0.0f;
    playSound(game,L"menu.wav");
    captureMouse(game,true);
    SetFocus(game.window);
}

void returnToMenu(Game& game) {
    game.x=game.z=game.pitch=game.elapsed=0.0f;
    game.yaw=ExitAngle;
    game.started=game.paused=game.won=false;
    game.routeVisible=game.routeCooldown=0.0f;
    game.stepDistance=0.0f;
    game.walkPhase=game.walking=0.0f;
    captureMouse(game,false);
    playSound(game,L"menu.wav");
}

void activateRoute(Game& game) {
    if (game.routeCooldown>0.0f || !game.started || game.paused || game.won) return;
    const int startX=static_cast<int>(std::floor((HalfMaze-game.x)/Cell));
    const int startY=static_cast<int>(std::floor((HalfMaze-game.z)/Cell));
    if (startX<0 || startY<0 || startX>=MapSize || startY>=MapSize || game.map[startY][startX]!='.') return;
    const int count=MapSize*MapSize, start=startY*MapSize+startX;
    const float goalRadius=14.5f;
    int goal=-1;
    float goalDistance=std::numeric_limits<float>::max();
    for (int y=0;y<MapSize;++y) for (int x=0;x<MapSize;++x) if (game.map[y][x]=='.') {
        const float wx=HalfMaze-(x+0.5f)*Cell, wz=HalfMaze-(y+0.5f)*Cell;
        const float radius=std::hypot(wx,wz), angle=std::atan2(wz,wx);
        const float angleDelta=std::atan2(std::sin(angle-ExitAngle),std::cos(angle-ExitAngle));
        if (radius<13.8f || std::abs(angleDelta)>0.035f) continue;
        const float distance=(radius-goalRadius)*(radius-goalRadius)+angleDelta*angleDelta*goalRadius*goalRadius;
        if (distance<goalDistance) { goalDistance=distance; goal=y*MapSize+x; }
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
            if (nx<0 || ny<0 || nx>=MapSize || ny>=MapSize || game.map[ny][nx]!='.') continue;
            const int next=ny*MapSize+nx;
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
    auto center=[&](int cell) {
        const int x=cell%MapSize, y=cell/MapSize;
        return Vec3{HalfMaze-(x+0.5f)*Cell,0.025f,HalfMaze-(y+0.5f)*Cell};
    };
    auto addRibbon=[&](Vec3 a,Vec3 b) {
        const float dx=b.x-a.x, dz=b.z-a.z, len=std::hypot(dx,dz);
        if (len<0.0001f) return;
        const float ox=-dz/len*0.055f, oz=dx/len*0.055f;
        addQuad(vertices,{a.x+ox,a.y,a.z+oz},{b.x+ox,b.y,b.z+oz},
                {b.x-ox,b.y,b.z-oz},{a.x-ox,a.y,a.z-oz},{0,1,0},0.06f,0.72f,0.92f,1.5f,3.0f);
    };
    for (size_t i=1;i<path.size();++i) addRibbon(center(path[i-1]),center(path[i]));
    const Vec3 exit{std::cos(ExitAngle)*15.35f,0.025f,std::sin(ExitAngle)*15.35f};
    addRibbon(center(path.back()),exit);
    glBindVertexArray(game.routeVao); glBindBuffer(GL_ARRAY_BUFFER,game.routeVbo);
    glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(vertices.size()*sizeof(Vertex)),vertices.data(),GL_DYNAMIC_DRAW);
    game.routeVertexCount=static_cast<GLsizei>(vertices.size());
    game.routeVisible=1.0f; game.routeCooldown=15.0f;
    playSound(game,L"route.wav");
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
    write(L"Camera",L"ThirdPerson",game.thirdPerson?1:0);
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
    game.settingsFromPause=game.started && game.paused;
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
    case 4: game.thirdPerson=direction>0; break;
    case 5: closeSettings(game); return;
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
        if (!game->started) beginGame(*game);
        else if (game->paused && !game->won) { game->paused=false; captureMouse(*game,true); playSound(*game,L"menu.wav"); }
        return 0;
    case WM_INPUT:
        if (game->mouseCaptured && !game->paused && !game->won) {
            RAWINPUT input{};
            UINT size=sizeof(input);
            if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam),RID_INPUT,&input,&size,sizeof(RAWINPUTHEADER))==size && input.header.dwType==RIM_TYPEMOUSE) {
                game->yaw += input.data.mouse.lLastX*0.0025f;
                game->pitch=std::clamp(game->pitch-input.data.mouse.lLastY*0.0025f,-1.35f,1.35f);
            }
        }
        return 0;
    case WM_KEYDOWN:
        if (!(lParam & (1LL<<30))) {
            if (game->showSettings) {
                if (wParam==VK_ESCAPE) closeSettings(*game);
                else if (wParam==VK_UP) game->settingsIndex=(game->settingsIndex+5)%6;
                else if (wParam==VK_DOWN) game->settingsIndex=(game->settingsIndex+1)%6;
                else if (wParam==VK_LEFT) adjustSetting(*game,-1);
                else if (wParam==VK_RIGHT) adjustSetting(*game,1);
                else if (wParam==VK_RETURN || wParam==VK_SPACE) {
                    if (game->settingsIndex==3) { game->soundEnabled=!game->soundEnabled; saveSettings(*game); }
                    else if (game->settingsIndex==4) { game->thirdPerson=!game->thirdPerson; saveSettings(*game); }
                    else if (game->settingsIndex==5) closeSettings(*game);
                }
            } else if (wParam==VK_ESCAPE) {
                if (!game->started) DestroyWindow(window);
                else if (game->paused || game->won) returnToMenu(*game);
                else { game->paused=true; captureMouse(*game,false); playSound(*game,L"menu.wav"); }
            } else if (wParam=='S' && (!game->started || game->paused)) {
                openSettings(*game);
            } else if (!game->started && (wParam==VK_RETURN || wParam==VK_SPACE)) {
                beginGame(*game);
            } else if (wParam=='R' && game->won) restart(*game);
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
void main() {
    vec4 world = uModel * vec4(aPosition, 1.0);
    vWorld = world.xyz;
    vNormal = mat3(uModel) * aNormal;
    vColor = aColor;
    vEmission = aEmission;
    vUv = aUv;
    vMaterial = aMaterial;
    gl_Position = uProjection * uView * world;
})GLSL";

constexpr char WorldFragmentShader[] = R"GLSL(#version 330 core
in vec3 vWorld;
in vec3 vNormal;
in vec3 vColor;
in float vEmission;
in vec2 vUv;
in float vMaterial;
uniform vec3 uEye;
uniform vec3 uFlash;
uniform vec3 uLightPos;
uniform float uBrightness;
uniform float uContrast;
uniform sampler2D uGrass;
uniform sampler2D uStone;
uniform sampler2D uCharacter;
out vec4 outColor;
void main() {
    vec3 toPoint = vWorld - uLightPos;
    float distanceToEye = length(vWorld - uEye);
    float distanceToLight = length(toPoint);
    float cone = dot(normalize(toPoint), normalize(uFlash));
    float spot = smoothstep(0.76, 0.93, cone);
    float diffuse = max(dot(normalize(vNormal), normalize(uLightPos - vWorld)), 0.0);
    float attenuation = 1.0 / (1.0 + 0.09*distanceToLight + 0.035*distanceToLight*distanceToLight);
    vec3 albedo = vColor;
    float alpha=1.0;
    if (vMaterial > 3.5) {
        vec4 texel=texture(uCharacter,vUv);
        if (texel.a<0.18) discard;
        albedo*=texel.rgb;
        alpha=texel.a;
    }
    if (vMaterial < 0.5) albedo *= texture(uGrass, vUv).rgb;
    else if (vMaterial < 1.5) {
        vec3 source = texture(uStone, vUv).rgb;
        float stoneValue = dot(source, vec3(0.299,0.587,0.114));
        vec3 grayStone = mix(vec3(0.30,0.33,0.34), vec3(0.92,0.94,0.92), smoothstep(0.16,0.72,stoneValue));
        vec2 patchCell = floor(vUv*1.4);
        vec2 patchUv = fract(vUv*1.4);
        float seed = fract(sin(dot(patchCell,vec2(127.1,311.7)))*43758.5453);
        vec2 center = vec2(fract(seed*17.13),fract(seed*39.71));
        float grassCoverage = (1.0-smoothstep(0.13,0.34,length((patchUv-center)*vec2(1.0,0.72))))*step(0.68,seed);
        grassCoverage *= 0.45+0.55*(1.0-smoothstep(0.25,1.7,vWorld.y));
        vec3 grass = texture(uGrass,vUv*2.0).rgb;
        vec3 moss = vec3(0.07,0.24,0.045)*(0.8+0.6*grass.g);
        albedo = mix(grayStone,moss,grassCoverage);
    }
    float ambient = vMaterial > 3.5 ? 0.045 : (vMaterial < 0.5 ? 0.018 : 0.009);
    vec3 lit = albedo * (ambient + spot*(1.05 + 1.25*diffuse)*attenuation) + vColor*vEmission;
    float fog = smoothstep(10.0, 18.0, distanceToEye);
    vec3 color=mix(lit, vec3(0.001,0.002,0.006), fog);
    color=max((color-vec3(0.5))*uContrast+vec3(0.5),vec3(0.0))*uBrightness;
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
out vec4 outColor;
float hash(vec2 p) { return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453); }
void main() {
    vec3 sampled=texture(uSky,vUv).rgb;
    vec3 night=vec3(0.001,0.002,0.008)+sampled*0.025;
    vec2 grid=vUv*vec2(480.0,240.0), cell=floor(grid), local=fract(grid);
    float seed=hash(cell);
    float star=(step(0.9975,seed))*(1.0-smoothstep(0.035,0.10,length(local-vec2(hash(cell+1.3),hash(cell+5.7)))));
    night+=vec3(0.24,0.30,0.48)*star;
    vec3 color=max((night-vec3(0.5))*uContrast+vec3(0.5),vec3(0.0))*uBrightness;
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

GLuint loadPngTexture(const std::filesystem::path& path) {
    Gdiplus::Bitmap image(path.c_str());
    if (image.GetLastStatus()!=Gdiplus::Ok) return 0;
    const int width=static_cast<int>(image.GetWidth()), height=static_cast<int>(image.GetHeight());
    if (width<=0 || height<=0) return 0;
    Gdiplus::Rect rect(0,0,width,height);
    Gdiplus::BitmapData data{};
    if (image.LockBits(&rect,Gdiplus::ImageLockModeRead,PixelFormat32bppARGB,&data)!=Gdiplus::Ok) return 0;
    GLuint texture=0;
    glGenTextures(1,&texture);
    glBindTexture(GL_TEXTURE_2D,texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_BGRA,GL_UNSIGNED_BYTE,data.Scan0);
    glGenerateMipmap(GL_TEXTURE_2D);
    image.UnlockBits(&data);
    return texture;
}

std::array<float,3> modelTint(const std::string& name) {
    if (name.find("Face")!=std::string::npos) return {0.91f,0.78f,0.69f};
    if (name.find("Hair")!=std::string::npos || name.find("Bangs")!=std::string::npos) return {0.92f,0.92f,1.0f};
    if (name.find("Eye")!=std::string::npos) return {0.85f,0.9f,1.0f};
    if (name.find("Down")!=std::string::npos || name.find("Up")!=std::string::npos) return {0.94f,0.91f,0.87f};
    return {0.9f,0.94f,1.0f};
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
                const auto tint=modelTint(material);
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
            const auto cached=game.modelTextures.find(key);
            if (cached!=game.modelTextures.end()) part.texture=cached->second;
            else {
                part.texture=loadPngTexture(texturePath->second);
                game.modelTextures.emplace(key,part.texture);
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
    if (!game.worldProgram || !game.skyProgram || !game.uiProgram) return false;
    game.viewLoc=glGetUniformLocation(game.worldProgram,"uView");
    game.projectionLoc=glGetUniformLocation(game.worldProgram,"uProjection");
    game.modelLoc=glGetUniformLocation(game.worldProgram,"uModel");
    game.eyeLoc=glGetUniformLocation(game.worldProgram,"uEye");
    game.flashLoc=glGetUniformLocation(game.worldProgram,"uFlash");
    game.lightPosLoc=glGetUniformLocation(game.worldProgram,"uLightPos");
    game.brightnessLoc=glGetUniformLocation(game.worldProgram,"uBrightness");
    game.contrastLoc=glGetUniformLocation(game.worldProgram,"uContrast");

    game.skyViewLoc=glGetUniformLocation(game.skyProgram,"uView");
    game.skyProjectionLoc=glGetUniformLocation(game.skyProgram,"uProjection");
    game.skyEyeLoc=glGetUniformLocation(game.skyProgram,"uEye");
    game.skySamplerLoc=glGetUniformLocation(game.skyProgram,"uSky");
    game.skyBrightnessLoc=glGetUniformLocation(game.skyProgram,"uBrightness");
    game.skyContrastLoc=glGetUniformLocation(game.skyProgram,"uContrast");

    wchar_t executable[MAX_PATH]{};
    GetModuleFileNameW(nullptr,executable,MAX_PATH);
    const auto textureDir=std::filesystem::path(executable).parent_path()/L"assets"/L"textures";
    game.grassTexture=loadTexture(textureDir/L"grass.ppm",GL_REPEAT,GL_REPEAT);
    game.stoneTexture=loadTexture(textureDir/L"stone.ppm",GL_REPEAT,GL_REPEAT);
    game.skyTexture=loadTexture(textureDir/L"night_sky.ppm",GL_REPEAT,GL_CLAMP_TO_EDGE);
    if (!game.grassTexture || !game.stoneTexture || !game.skyTexture) return false;

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
    glUniform1i(glGetUniformLocation(game.worldProgram,"uCharacter"),3);
    glUseProgram(game.skyProgram);
    glUniform1i(game.skySamplerLoc,2);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    const auto modelDir=std::filesystem::path(executable).parent_path()/L"assets"/L"models";
    const auto characterDir=modelDir/L"chisa";
    loadObjModel(game,characterDir/L"Chisa.obj",game.character);
    game.characterWalk.resize(8);
    for (int i=0;i<static_cast<int>(game.characterWalk.size());++i)
        loadObjModel(game,characterDir/(L"ChisaWalk0"+std::to_wstring(i)+L".obj"),game.characterWalk[i]);
    loadObjModel(game,modelDir/L"flashlight"/L"Flashlight.obj",game.flashlight);
    return true;
}

void movePlayer(Game& game, float dt) {
    if (!game.started || game.paused || game.won) return;
    game.routeVisible=std::max(0.0f,game.routeVisible-dt);
    game.routeCooldown=std::max(0.0f,game.routeCooldown-dt);
    const float forward=(GetAsyncKeyState('W')<0?1.0f:0.0f)-(GetAsyncKeyState('S')<0?1.0f:0.0f);
    const float side=(GetAsyncKeyState('D')<0?1.0f:0.0f)-(GetAsyncKeyState('A')<0?1.0f:0.0f);
    if (forward==0.0f && side==0.0f) {
        game.stepDistance=0.0f;
        game.walking=std::max(0.0f,game.walking-dt*7.0f);
        game.elapsed+=dt;
        return;
    }
    const float length=std::sqrt(forward*forward+side*side);
    const float f=forward/length, s=side/length;
    const bool sprinting=GetAsyncKeyState(VK_SHIFT)<0;
    const float speed=sprinting?4.6f:2.35f;
    const float dx=(std::cos(game.yaw)*f-std::sin(game.yaw)*s)*speed*dt;
    const float dz=(std::sin(game.yaw)*f+std::cos(game.yaw)*s)*speed*dt;
    const float oldX=game.x, oldZ=game.z;
    if (canStand(game,game.x+dx,game.z)) game.x+=dx;
    if (canStand(game,game.x,game.z+dz)) game.z+=dz;
    const float moved=std::hypot(game.x-oldX,game.z-oldZ);
    game.stepDistance+=moved;
    game.walking=std::clamp(game.walking+(moved>0.0001f?dt*8.0f:-dt*7.0f),0.0f,1.0f);
    if (moved>0.0001f) game.walkPhase=std::fmod(game.walkPhase+dt*(sprinting?13.0f:10.0f),2.0f*Pi);
    const float stepLength=sprinting?0.9f:0.68f;
    if (game.stepDistance>=stepLength) {
        game.stepDistance-=stepLength;
        if (game.routeVisible<=0.0f)
            playSound(game,game.alternateFootstep?L"step2.wav":L"step1.wav");
        game.alternateFootstep=!game.alternateFootstep;
    }
    game.elapsed+=dt;

    const float radius=std::hypot(game.x,game.z);
    const float angle=std::atan2(game.z,game.x);
    const float delta=std::atan2(std::sin(angle-ExitAngle),std::cos(angle-ExitAngle));
    if (radius>=15.45f && std::abs(delta)<0.12f) {
        game.won=true;
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
        uiRect(vertices,w,h,cx-300,cy-235,600,470,{0.012f,0.025f,0.030f,0.98f});
        uiRect(vertices,w,h,cx-300,cy-235,600,5,{0.25f,0.88f,0.67f,1.0f});
        centeredText(vertices,w,h,"SETTINGS",cy-195,3.0f,green);
        constexpr int widths[]={960,1280,1600}, heights[]={600,800,900};
        const std::string resolution=game.resolutionIndex==3?"FULLSCREEN":
            std::to_string(widths[game.resolutionIndex])+"X"+std::to_string(heights[game.resolutionIndex]);
        const std::array<std::string,6> rows={
            "RESOLUTION "+resolution,
            "BRIGHTNESS "+std::to_string(static_cast<int>(std::lround(game.brightness*100.0f))),
            "CONTRAST "+std::to_string(static_cast<int>(std::lround(game.contrast*100.0f))),
            std::string("SOUND ")+(game.soundEnabled?"ON":"OFF"),
            std::string("CAMERA ")+(game.thirdPerson?"THIRD":"FIRST"),
            "BACK"
        };
        for (int i=0;i<static_cast<int>(rows.size());++i) {
            const float y=cy-132+i*46.0f;
            if (i==game.settingsIndex) uiRect(vertices,w,h,cx-250,y-8,500,34,{0.08f,0.32f,0.28f,0.95f});
            centeredText(vertices,w,h,rows[i],y,1.55f,i==game.settingsIndex?green:white);
        }
        centeredText(vertices,w,h,"UP DOWN SELECT",cy+165,1.25f,white);
        centeredText(vertices,w,h,"LEFT RIGHT ADJUST",cy+190,1.25f,cyan);
        centeredText(vertices,w,h,"ESC BACK",cy+215,1.1f,white);
    } else if (!game.started) {
        const float cx=w*0.5f, cy=h*0.5f;
        uiRect(vertices,w,h,0,0,static_cast<float>(w),static_cast<float>(h),{0.002f,0.006f,0.010f,0.78f});
        uiRect(vertices,w,h,cx-300,cy-230,600,460,{0.012f,0.025f,0.030f,0.97f});
        uiRect(vertices,w,h,cx-300,cy-230,600,5,{0.25f,0.88f,0.67f,1.0f});
        centeredText(vertices,w,h,"MAZE ESCAPE",cy-165,4.0f,green);
        centeredText(vertices,w,h,"FIND THE EXIT",cy-92,2.0f,white);
        centeredText(vertices,w,h,"WASD MOVE",cy-32,1.7f,white);
        centeredText(vertices,w,h,"MOUSE LOOK",cy+8,1.7f,white);
        centeredText(vertices,w,h,"SHIFT SPRINT",cy+48,1.7f,white);
        centeredText(vertices,w,h,"O SHOW ROUTE",cy+80,1.35f,cyan);
        uiRect(vertices,w,h,cx-190,cy+105,380,56,{0.08f,0.32f,0.28f,1.0f});
        centeredText(vertices,w,h,"ENTER OR CLICK TO START",cy+124,1.3f,green);
        centeredText(vertices,w,h,"S SETTINGS",cy+172,1.15f,cyan);
        centeredText(vertices,w,h,"ESC TO CLOSE",cy+196,1.0f,cyan);
    } else if (game.won) {
        uiRect(vertices,w,h,w*0.5f-220,h*0.5f-110,440,220,{0.004f,0.012f,0.018f,0.92f});
        centeredText(vertices,w,h,"YOU ESCAPED!",h*0.5f-60,4.0f,green);
        char timeText[32]{}; formatTime(game.elapsed,timeText);
        centeredText(vertices,w,h,std::string("TIME ")+timeText,h*0.5f+4,2.0f,white);
        centeredText(vertices,w,h,"R TO RESTART",h*0.5f+55,1.6f,cyan);
    } else if (game.paused) {
        uiRect(vertices,w,h,0,0,static_cast<float>(w),static_cast<float>(h),{0.002f,0.006f,0.010f,0.72f});
        centeredText(vertices,w,h,"PAUSED",h*0.5f-48,4.0f,white);
        centeredText(vertices,w,h,"CLICK TO RESUME",h*0.5f+12,1.8f,cyan);
        centeredText(vertices,w,h,"S SETTINGS",h*0.5f+46,1.5f,cyan);
        centeredText(vertices,w,h,"ESC TO TITLE",h*0.5f+78,1.35f,white);
    } else {
        uiRect(vertices,w,h,16,16,208,122,{0.004f,0.010f,0.016f,0.72f});
        uiText(vertices,w,h,"MAZE ESCAPE",28,26,1.7f,cyan);
        char timeText[32]{}; formatTime(game.elapsed,timeText);
        uiText(vertices,w,h,std::string("TIME ")+timeText,28,47,1.7f,white);
        uiText(vertices,w,h,"WASD MOVE",28,73,1.25f,white);
        uiText(vertices,w,h,"MOUSE LOOK",28,92,1.25f,white);
        uiText(vertices,w,h,"SHIFT SPRINT",28,109,1.25f,white);
        const float skillX=std::max(12.0f,w-174.0f), skillY=static_cast<float>(h)-68.0f;
        uiRect(vertices,w,h,skillX,skillY,162,52,{0.004f,0.010f,0.016f,0.78f});
        uiText(vertices,w,h,"ROUTE",skillX+12,skillY+8,1.15f,cyan);
        char skillText[32]{};
        if (game.routeCooldown>0.0f) wsprintfA(skillText,"O %02dS",static_cast<int>(std::ceil(game.routeCooldown)));
        else lstrcpyA(skillText,"O READY");
        uiText(vertices,w,h,skillText,skillX+12,skillY+29,1.35f,game.routeCooldown>0.0f?white:green);
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

void drawModel(Game& game,const ModelObject& object,const std::array<float,16>& transform,bool bodyOnly=false) {
    if (!object.loaded || object.parts.empty()) return;
    glUniformMatrix4fv(game.modelLoc,1,GL_FALSE,transform.data());
    glBindVertexArray(object.vao);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE3);
    for (const ModelPart& part:object.parts) {
        if (bodyOnly && part.material.find("Up")==std::string::npos && part.material.find("Down")==std::string::npos) continue;
        glBindTexture(GL_TEXTURE_2D,part.texture);
        glDrawArrays(GL_TRIANGLES,part.first,part.count);
    }
    glDisable(GL_BLEND);
}

void render(Game& game) {
    glViewport(0,0,game.width,game.height);
    glClearColor(0.004f,0.007f,0.012f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    float cameraYaw=game.yaw, cameraPitch=game.pitch;
    const float bob=std::sin(game.walkPhase*2.0f)*0.025f*game.walking;
    Vec3 eye{game.x,1.38f+bob,game.z};
    if (game.thirdPerson && game.started) {
        const float forwardX=std::cos(game.yaw), forwardZ=std::sin(game.yaw);
        float distance=0.0f;
        for (float candidate=0.025f;candidate<=2.25f;candidate+=0.025f) {
            if (!canStand(game,game.x-forwardX*candidate,game.z-forwardZ*candidate)) break;
            distance=candidate;
        }
        eye={game.x-forwardX*distance,1.62f,game.z-forwardZ*distance};
        cameraPitch=std::clamp(game.pitch-0.18f,-1.15f,1.15f);
    }
    const float cp=std::cos(game.pitch);
    const Vec3 flash{std::cos(game.yaw)*cp,std::sin(game.pitch),std::sin(game.yaw)*cp};
    Vec3 heldRight{},heldUp{},heldForward{},heldOrigin{};
    float heldScale=0.12f;
    if (game.thirdPerson) {
        const float angle=Pi*0.5f-game.yaw;
        heldRight={std::cos(angle),0.0f,-std::sin(angle)};
        heldUp={0.0f,1.0f,0.0f};
        heldForward={std::sin(angle),0.0f,std::cos(angle)};
        heldOrigin={game.x+heldForward.x*0.22f+heldRight.x*0.18f,0.96f,
                    game.z+heldForward.z*0.22f+heldRight.z*0.18f};
        heldScale=0.085f;
    } else {
        heldForward={std::cos(game.yaw)*cp,std::sin(game.pitch),std::sin(game.yaw)*cp};
        heldRight=normalize(cross(heldForward,{0.0f,1.0f,0.0f}));
        heldUp=cross(heldRight,heldForward);
        heldOrigin={eye.x+heldRight.x*0.26f-heldUp.x*0.22f+heldForward.x*0.62f,
                    eye.y+heldRight.y*0.26f-heldUp.y*0.22f+heldForward.y*0.62f,
                    eye.z+heldRight.z*0.26f-heldUp.z*0.22f+heldForward.z*0.62f};
    }
    const Vec3 lightPos{heldOrigin.x+heldForward.x*heldScale*2.871889f,
                        heldOrigin.y+heldForward.y*heldScale*2.871889f,
                        heldOrigin.z+heldForward.z*heldScale*2.871889f};
    const auto view=viewMatrix(eye,cameraYaw,cameraPitch);
    const auto projection=perspective(70.0f*Pi/180.0f,static_cast<float>(game.width)/game.height,0.05f,70.0f);
    const std::array<float,16> model{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};

    glDisable(GL_DEPTH_TEST);
    glUseProgram(game.skyProgram);
    glUniformMatrix4fv(game.skyViewLoc,1,GL_FALSE,view.data());
    glUniformMatrix4fv(game.skyProjectionLoc,1,GL_FALSE,projection.data());
    glUniform3f(game.skyEyeLoc,eye.x,eye.y,eye.z);
    glUniform1f(game.skyBrightnessLoc,game.brightness);
    glUniform1f(game.skyContrastLoc,game.contrast);
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
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,game.grassTexture);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D,game.stoneTexture);
    glBindVertexArray(game.worldVao);
    glDrawArrays(GL_TRIANGLES,0,game.worldVertexCount);
    if (game.routeVisible>0.0f && game.routeVertexCount>0) {
        glBindVertexArray(game.routeVao);
        glDrawArrays(GL_TRIANGLES,0,game.routeVertexCount);
    }
    if (game.started && game.character.loaded) {
        int frame=0;
        if (!game.characterWalk.empty())
            frame=std::clamp(static_cast<int>(game.walkPhase/(2.0f*Pi)*game.characterWalk.size()),
                             0,static_cast<int>(game.characterWalk.size())-1);
        const ModelObject& actor=game.walking>0.05f && !game.characterWalk.empty() && game.characterWalk[frame].loaded
            ? game.characterWalk[frame] : game.character;
        const float modelBob=std::sin(game.walkPhase*2.0f)*0.018f*game.walking;
        const float angle=Pi*0.5f-game.yaw;
        const Vec3 right{std::cos(angle),0.0f,-std::sin(angle)};
        const Vec3 up{0.0f,1.0f,0.0f};
        const Vec3 forward{std::sin(angle),0.0f,std::cos(angle)};
        const Vec3 modelBack{-forward.x,0.0f,-forward.z};
        const auto characterTransform=basisTransform(right,modelBack,up,{game.x,modelBob,game.z},1.0f);
        if (game.thirdPerson) drawModel(game,actor,characterTransform);
        else drawModel(game,actor,characterTransform,true);
        const auto heldLight=basisTransform(heldRight,heldUp,heldForward,heldOrigin,heldScale);
        drawModel(game,game.flashlight,heldLight);
    }
    renderUi(game);
    SwapBuffers(game.dc);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int) {
    Gdiplus::GdiplusStartupInput gdiplusInput;
    ULONG_PTR gdiplusToken=0;
    if (Gdiplus::GdiplusStartup(&gdiplusToken,&gdiplusInput,nullptr)!=Gdiplus::Ok) {
        MessageBoxW(nullptr,L"Could not initialize image support.",L"Maze Escape",MB_OK|MB_ICONERROR);
        return 1;
    }
    Game game;
    if (!loadMap(game)) {
        MessageBoxW(nullptr,L"Could not generate the maze layout.",L"Maze Escape",MB_OK|MB_ICONERROR);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return 1;
    }
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
        movePlayer(game,dt);
        render(game);
    }
    captureMouse(game,false);
    if (wglGetCurrentContext()==game.gl) wglMakeCurrent(nullptr,nullptr);
    if (game.gl) wglDeleteContext(game.gl);
    if (game.dc && game.window) ReleaseDC(game.window,game.dc);
    if (game.window && IsWindow(game.window)) DestroyWindow(game.window);
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return 0;
}
