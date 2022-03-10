/*
TODO:
1. Login to usos
2. Go to plan for current week
3. Take a screenshot of plan and explanations of buildings
4. Change to next week
5. Take a screenshot of plan and explanations of buildings
6. Open plan on plan.ii.us.edu.pl of current week
7. Take a screenshot of plan
6. Open plan on plan.ii.us.edu.pl of next week
8. Take a screenshot of plan
9. Send screenshots to my phone
10. Display screenshots one after the other (add scroll if necessary)
*/
#include <SDL.h>
#include <SDL_image.h>T
#include <SDL_mixer.h>
#include <SDL_net.h>
#include <SDL_ttf.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <random>
#include <windows.h>
//#include <SDL_gpu.h>
//#include <SFML/Network.hpp>
//#include <SFML/Graphics.hpp>
#include <algorithm>
#include <atomic>
#include <codecvt>
#include <functional>
#include <locale>
#include <mutex>
#include <thread>
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_sdlrenderer.h>
#include <misc/cpp/imgui_stdlib.h>
#ifdef __ANDROID__
#include "vendor/PUGIXML/src/pugixml.hpp"
#include <android/log.h> //__android_log_print(ANDROID_LOG_VERBOSE, "PlanDisplayer", "Example number log: %d", number);
#include <jni.h>
#include "vendor/GLM/include/glm/glm.hpp"
#include "vendor/GLM/include/glm/gtc/matrix_transform.hpp"
#include "vendor/GLM/include/glm/gtc/type_ptr.hpp"
#else
#include <filesystem>
#include <pugixml.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#ifdef __EMSCRIPTEN__
namespace fs = std::__fs::filesystem;
#else
namespace fs = std::filesystem;
#endif
using namespace std::chrono_literals;
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

// NOTE: Remember to uncomment it on every release
//#define RELEASE

#if defined _MSC_VER && defined RELEASE
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

//240 x 240 (smart watch)
//240 x 320 (QVGA)
//360 x 640 (Galaxy S5)
//640 x 480 (480i - Smallest PC monitor)

#define SERVER_PORT 8021
#define MAX_SOCKETS 65534

int windowWidth = 240;
int windowHeight = 320;
SDL_Point mousePos;
SDL_Point realMousePos;
bool keys[SDL_NUM_SCANCODES];
bool buttons[SDL_BUTTON_X2 + 1];
SDL_Window* window;
SDL_Renderer* renderer;

void logOutputCallback(void* userdata, int category, SDL_LogPriority priority, const char* message)
{
    std::cout << message << std::endl;
}

int random(int min, int max)
{
    return min + rand() % ((max + 1) - min);
}

int SDL_QueryTextureF(SDL_Texture* texture, Uint32* format, int* access, float* w, float* h)
{
    int wi, hi;
    int result = SDL_QueryTexture(texture, format, access, &wi, &hi);
    if (w) {
        *w = wi;
    }
    if (h) {
        *h = hi;
    }
    return result;
}

SDL_bool SDL_PointInFRect(const SDL_Point* p, const SDL_FRect* r)
{
    return ((p->x >= r->x) && (p->x < (r->x + r->w)) && (p->y >= r->y) && (p->y < (r->y + r->h))) ? SDL_TRUE : SDL_FALSE;
}

std::ostream& operator<<(std::ostream& os, SDL_FRect r)
{
    os << r.x << " " << r.y << " " << r.w << " " << r.h;
    return os;
}

std::ostream& operator<<(std::ostream& os, SDL_Rect r)
{
    SDL_FRect fR;
    fR.w = r.w;
    fR.h = r.h;
    fR.x = r.x;
    fR.y = r.y;
    os << fR;
    return os;
}

SDL_Texture* renderText(SDL_Texture* previousTexture, TTF_Font* font, SDL_Renderer* renderer, const std::string& text, SDL_Color color)
{
    if (previousTexture) {
        SDL_DestroyTexture(previousTexture);
    }
    SDL_Surface* surface;
    if (text.empty()) {
        surface = TTF_RenderUTF8_Blended(font, " ", color);
    }
    else {
        surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    }
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        return texture;
    }
    else {
        return 0;
    }
}

struct Text {
    std::string text;
    SDL_Surface* surface = 0;
    SDL_Texture* t = 0;
    SDL_FRect dstR{};
    bool autoAdjustW = false;
    bool autoAdjustH = false;
    float wMultiplier = 1;
    float hMultiplier = 1;

    ~Text()
    {
        if (surface) {
            SDL_FreeSurface(surface);
        }
        if (t) {
            SDL_DestroyTexture(t);
        }
    }

    Text()
    {
    }

    Text(const Text& rightText)
    {
        text = rightText.text;
        if (surface) {
            SDL_FreeSurface(surface);
        }
        if (t) {
            SDL_DestroyTexture(t);
        }
        if (rightText.surface) {
            surface = SDL_ConvertSurface(rightText.surface, rightText.surface->format, SDL_SWSURFACE);
        }
        if (rightText.t) {
            t = SDL_CreateTextureFromSurface(renderer, surface);
        }
        dstR = rightText.dstR;
        autoAdjustW = rightText.autoAdjustW;
        autoAdjustH = rightText.autoAdjustH;
        wMultiplier = rightText.wMultiplier;
        hMultiplier = rightText.hMultiplier;
    }

    Text& operator=(const Text& rightText)
    {
        text = rightText.text;
        if (surface) {
            SDL_FreeSurface(surface);
        }
        if (t) {
            SDL_DestroyTexture(t);
        }
        if (rightText.surface) {
            surface = SDL_ConvertSurface(rightText.surface, rightText.surface->format, SDL_SWSURFACE);
        }
        if (rightText.t) {
            t = SDL_CreateTextureFromSurface(renderer, surface);
        }
        dstR = rightText.dstR;
        autoAdjustW = rightText.autoAdjustW;
        autoAdjustH = rightText.autoAdjustH;
        wMultiplier = rightText.wMultiplier;
        hMultiplier = rightText.hMultiplier;
        return *this;
    }

    void setText(SDL_Renderer* renderer, TTF_Font* font, std::string text, SDL_Color c = { 255, 255, 255 })
    {
        this->text = text;
#if 1 // NOTE: renderText
        if (surface) {
            SDL_FreeSurface(surface);
        }
        if (t) {
            SDL_DestroyTexture(t);
        }
        if (text.empty()) {
            surface = TTF_RenderUTF8_Blended(font, " ", c);
        }
        else {
            surface = TTF_RenderUTF8_Blended(font, text.c_str(), c);
        }
        if (surface) {
            t = SDL_CreateTextureFromSurface(renderer, surface);
        }
#endif
        if (autoAdjustW) {
            SDL_QueryTextureF(t, 0, 0, &dstR.w, 0);
        }
        if (autoAdjustH) {
            SDL_QueryTextureF(t, 0, 0, 0, &dstR.h);
        }
        dstR.w *= wMultiplier;
        dstR.h *= hMultiplier;
    }

    void setText(SDL_Renderer* renderer, TTF_Font* font, int value, SDL_Color c = { 255, 255, 255 })
    {
        setText(renderer, font, std::to_string(value), c);
    }

    void draw(SDL_Renderer* renderer)
    {
        if (t) {
            SDL_RenderCopyF(renderer, t, 0, &dstR);
        }
    }
};

int SDL_RenderDrawCircle(SDL_Renderer* renderer, int x, int y, int radius)
{
    int offsetx, offsety, d;
    int status;

    offsetx = 0;
    offsety = radius;
    d = radius - 1;
    status = 0;

    while (offsety >= offsetx) {
        status += SDL_RenderDrawPoint(renderer, x + offsetx, y + offsety);
        status += SDL_RenderDrawPoint(renderer, x + offsety, y + offsetx);
        status += SDL_RenderDrawPoint(renderer, x - offsetx, y + offsety);
        status += SDL_RenderDrawPoint(renderer, x - offsety, y + offsetx);
        status += SDL_RenderDrawPoint(renderer, x + offsetx, y - offsety);
        status += SDL_RenderDrawPoint(renderer, x + offsety, y - offsetx);
        status += SDL_RenderDrawPoint(renderer, x - offsetx, y - offsety);
        status += SDL_RenderDrawPoint(renderer, x - offsety, y - offsetx);

        if (status < 0) {
            status = -1;
            break;
        }

        if (d >= 2 * offsetx) {
            d -= 2 * offsetx + 1;
            offsetx += 1;
        }
        else if (d < 2 * (radius - offsety)) {
            d += 2 * offsety - 1;
            offsety -= 1;
        }
        else {
            d += 2 * (offsety - offsetx - 1);
            offsety -= 1;
            offsetx += 1;
        }
    }

    return status;
}

int SDL_RenderFillCircle(SDL_Renderer* renderer, int x, int y, int radius)
{
    int offsetx, offsety, d;
    int status;

    offsetx = 0;
    offsety = radius;
    d = radius - 1;
    status = 0;

    while (offsety >= offsetx) {

        status += SDL_RenderDrawLine(renderer, x - offsety, y + offsetx,
            x + offsety, y + offsetx);
        status += SDL_RenderDrawLine(renderer, x - offsetx, y + offsety,
            x + offsetx, y + offsety);
        status += SDL_RenderDrawLine(renderer, x - offsetx, y - offsety,
            x + offsetx, y - offsety);
        status += SDL_RenderDrawLine(renderer, x - offsety, y - offsetx,
            x + offsety, y - offsetx);

        if (status < 0) {
            status = -1;
            break;
        }

        if (d >= 2 * offsetx) {
            d -= 2 * offsetx + 1;
            offsetx += 1;
        }
        else if (d < 2 * (radius - offsety)) {
            d += 2 * offsety - 1;
            offsety -= 1;
        }
        else {
            d += 2 * (offsety - offsetx - 1);
            offsety -= 1;
            offsetx += 1;
        }
    }

    return status;
}

struct Clock {
    Uint64 start = SDL_GetPerformanceCounter();

    float getElapsedTime()
    {
        Uint64 stop = SDL_GetPerformanceCounter();
        float secondsElapsed = (stop - start) / (float)SDL_GetPerformanceFrequency();
        return secondsElapsed * 1000;
    }

    float restart()
    {
        Uint64 stop = SDL_GetPerformanceCounter();
        float secondsElapsed = (stop - start) / (float)SDL_GetPerformanceFrequency();
        start = SDL_GetPerformanceCounter();
        return secondsElapsed * 1000;
    }
};

SDL_bool SDL_FRectEmpty(const SDL_FRect* r)
{
    return ((!r) || (r->w <= 0) || (r->h <= 0)) ? SDL_TRUE : SDL_FALSE;
}

SDL_bool SDL_IntersectFRect(const SDL_FRect* A, const SDL_FRect* B, SDL_FRect* result)
{
    int Amin, Amax, Bmin, Bmax;

    if (!A) {
        SDL_InvalidParamError("A");
        return SDL_FALSE;
    }

    if (!B) {
        SDL_InvalidParamError("B");
        return SDL_FALSE;
    }

    if (!result) {
        SDL_InvalidParamError("result");
        return SDL_FALSE;
    }

    /* Special cases for empty rects */
    if (SDL_FRectEmpty(A) || SDL_FRectEmpty(B)) {
        result->w = 0;
        result->h = 0;
        return SDL_FALSE;
    }

    /* Horizontal intersection */
    Amin = A->x;
    Amax = Amin + A->w;
    Bmin = B->x;
    Bmax = Bmin + B->w;
    if (Bmin > Amin)
        Amin = Bmin;
    result->x = Amin;
    if (Bmax < Amax)
        Amax = Bmax;
    result->w = Amax - Amin;

    /* Vertical intersection */
    Amin = A->y;
    Amax = Amin + A->h;
    Bmin = B->y;
    Bmax = Bmin + B->h;
    if (Bmin > Amin)
        Amin = Bmin;
    result->y = Amin;
    if (Bmax < Amax)
        Amax = Bmax;
    result->h = Amax - Amin;

    return (SDL_FRectEmpty(result) == SDL_TRUE) ? SDL_FALSE : SDL_TRUE;
}

SDL_bool SDL_HasIntersectionF(const SDL_FRect* A, const SDL_FRect* B)
{
    float Amin, Amax, Bmin, Bmax;

    if (!A) {
        SDL_InvalidParamError("A");
        return SDL_FALSE;
    }

    if (!B) {
        SDL_InvalidParamError("B");
        return SDL_FALSE;
    }

    /* Special cases for empty rects */
    if (SDL_FRectEmpty(A) || SDL_FRectEmpty(B)) {
        return SDL_FALSE;
    }

    /* Horizontal intersection */
    Amin = A->x;
    Amax = Amin + A->w;
    Bmin = B->x;
    Bmax = Bmin + B->w;
    if (Bmin > Amin)
        Amin = Bmin;
    if (Bmax < Amax)
        Amax = Bmax;
    if (Amax <= Amin)
        return SDL_FALSE;

    /* Vertical intersection */
    Amin = A->y;
    Amax = Amin + A->h;
    Bmin = B->y;
    Bmax = Bmin + B->h;
    if (Bmin > Amin)
        Amin = Bmin;
    if (Bmax < Amax)
        Amax = Bmax;
    if (Amax <= Amin)
        return SDL_FALSE;

    return SDL_TRUE;
}

int eventWatch(void* userdata, SDL_Event* event)
{
    // WARNING: Be very careful of what you do in the function, as it may run in a different thread
    if (event->type == SDL_APP_TERMINATING || event->type == SDL_APP_WILLENTERBACKGROUND) {
    }
    return 0;
}

float clamp(float n, float lower, float upper)
{
    return std::max(lower, std::min(n, upper));
}

void leftMousButtonClick()
{
    INPUT inputs[2]{};
    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(2, inputs, sizeof(INPUT));
}

void takeScreenshot()
{
    INPUT ip;
    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0; // hardware scan code for key
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;

    ip.ki.wVk = VK_LWIN;
    ip.ki.dwFlags = 0; // 0 for key press
    SendInput(1, &ip, sizeof(INPUT));
    ip.ki.wVk = VK_SNAPSHOT;
    ip.ki.dwFlags = 0; // 0 for key press
    SendInput(1, &ip, sizeof(INPUT));

    ip.ki.wVk = VK_LWIN;
    ip.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &ip, sizeof(INPUT));
    ip.ki.wVk = VK_SNAPSHOT;
    ip.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &ip, sizeof(INPUT));
}

#define MAX_LENGTH 1024 * 100

int send(TCPsocket socket, std::string msg)
{
    return SDLNet_TCP_Send(socket, msg.c_str(), msg.size() + 1);
}

int receive(TCPsocket socket, std::string& msg)
{
    char message[MAX_LENGTH]{};
    int result = SDLNet_TCP_Recv(socket, message, MAX_LENGTH);
    msg = message;
    return result;
}

struct Client {
    TCPsocket socket = 0;
};

std::vector<Client> clients;

#define i8 int8_t
#define i16 int16_t
#define i32 int32_t
#define i64 int64_t
#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

enum PacketType : i32 {
    ReceivePlan,
};

void runServer()
{
    /*
    TODO: Write a software which detects modification date and loads files in folder from oldest to the newest xor find another way to taking a screenshot and saving it to a file

    I can just write a software which will put a screenshot into a paint and save it from there
    */
    IPaddress ip;
    SDLNet_ResolveHost(&ip, 0, SERVER_PORT);
    TCPsocket serverSocket = SDLNet_TCP_Open(&ip);
    SDLNet_SocketSet socketSet = SDLNet_AllocSocketSet(MAX_SOCKETS + 1);
    SDLNet_TCP_AddSocket(socketSet, serverSocket);
    bool running = true;
    while (running) {
        if (SDLNet_CheckSockets(socketSet, 0) > 0) {
            if (SDLNet_SocketReady(serverSocket)) {
                clients.push_back(Client());
                clients.back().socket = SDLNet_TCP_Accept(serverSocket);
                SDLNet_TCP_AddSocket(socketSet, clients.back().socket);
                printf("Client joined!\n");
            }
            for (int i = 0; i < clients.size(); ++i) {
                if (SDLNet_SocketReady(clients[i].socket)) {
                    std::string msg;
                    receive(clients[i].socket, msg);
                    pugi::xml_document doc;
                    doc.load_string(msg.c_str());
                    PacketType packetType = (PacketType)doc.child("root").child("packetType").text().as_int();
                    if (packetType == PacketType::ReceivePlan) {

/*
                TODO:
                Set usos to 50%
                Set plan.ii.us.edu.pl to 80%
                Put some delays so that google chrome will be on time to open pages
                On plan.ii.us.edu.pl open plan of IT of group B3
                Save login and password to usos in a browser
*/
#if 1 // NOTE: usos \
    // TODO: Open first tab
                        SetCursorPos(1501, 110);
                        leftMousButtonClick();
                        SetCursorPos(562, 656);
                        leftMousButtonClick();
                        SetCursorPos(689, 294);
                        leftMousButtonClick();
                        takeScreenshot(); // TODO: Figure out how to send screenshot from clipboard to the client and display it there as SDL_Texture*
                        /*
                TODO:
                1. Save it to a file
                2. Load it as std::string
                3. Send it to the client
                4. Save it as a file
                5. Load it as SDL_Texture* and display
                */
                        SetCursorPos(881, 231);
                        leftMousButtonClick();
                        takeScreenshot();
                        SetCursorPos(1501, 110);
                        leftMousButtonClick();
                        SetCursorPos(205, 82);
                        leftMousButtonClick();
#endif
#if 1 // NOTE: plan.ii.us.edu.pl
                        SetCursorPos(332, 16); // TODO: Put here coordinates of second tab
                        leftMousButtonClick();
                        SetCursorPos(488, 302);
                        leftMousButtonClick();
                        takeScreenshot();
                        SetCursorPos(642, 258);
                        INPUT ip;
                        ip.type = INPUT_KEYBOARD;
                        ip.ki.wScan = 0; // hardware scan code for key
                        ip.ki.time = 0;
                        ip.ki.dwExtraInfo = 0;

                        ip.ki.wVk = VK_DOWN;
                        ip.ki.dwFlags = 0; // 0 for key press
                        SendInput(1, &ip, sizeof(INPUT));
                        ip.ki.wVk = VK_DOWN;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP;
                        SendInput(1, &ip, sizeof(INPUT));

                        ip.ki.wVk = VK_RETURN;
                        ip.ki.dwFlags = 0; // 0 for key press
                        SendInput(1, &ip, sizeof(INPUT));
                        ip.ki.wVk = VK_RETURN;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP;
                        SendInput(1, &ip, sizeof(INPUT));

                        SetCursorPos(488, 302);
                        leftMousButtonClick();
                        takeScreenshot();
#endif
                        pugi::xml_document doc;
                        pugi::xml_node rootNode = doc.append_child("root");

                        std::stringstream ss;
                        doc.save(ss);
                        send(clients[i].socket, ss.str());
                    }
                }
            }
        }
    }
}

int main(int argc, char* argv[])
{
    {
#if 1 // NOTE: usos
        SetCursorPos(1501, 110);
        leftMousButtonClick();
        SDL_Delay(20000);
        SetCursorPos(562, 656);
        leftMousButtonClick();
        SDL_Delay(20000);
        SetCursorPos(689, 294);
        leftMousButtonClick();
        SDL_Delay(20000);
        takeScreenshot();
        SetCursorPos(881, 231);
        leftMousButtonClick();
        SDL_Delay(20000);
        takeScreenshot();
        SetCursorPos(1501, 110);
        leftMousButtonClick();
        SDL_Delay(20000);
        SetCursorPos(205, 82);
        leftMousButtonClick();
        SDL_Delay(20000);
#endif
#if 1 // NOTE: plan.ii.us.edu.pl
        SetCursorPos(332, 16);
        leftMousButtonClick();
        SDL_Delay(20000);
        SetCursorPos(488, 302);
        leftMousButtonClick();
        SDL_Delay(20000);
        takeScreenshot();
        SetCursorPos(642, 258);
        leftMousButtonClick();
        SDL_Delay(20000);
        INPUT ip;
        ip.type = INPUT_KEYBOARD;
        ip.ki.wScan = 0; // hardware scan code for key
        ip.ki.time = 0;
        ip.ki.dwExtraInfo = 0;

        ip.ki.wVk = VK_DOWN;
        ip.ki.dwFlags = 0; // 0 for key press
        SendInput(1, &ip, sizeof(INPUT));
        SDL_Delay(20000);
        ip.ki.wVk = VK_DOWN;
        ip.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &ip, sizeof(INPUT));
        SDL_Delay(20000);

        ip.ki.wVk = VK_RETURN;
        ip.ki.dwFlags = 0; // 0 for key press
        SendInput(1, &ip, sizeof(INPUT));
        ip.ki.wVk = VK_RETURN;
        ip.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &ip, sizeof(INPUT));
        SDL_Delay(20000);

        SetCursorPos(488, 302);
        leftMousButtonClick();
        takeScreenshot();
#endif
    }
    return 0;
    std::srand(std::time(0));
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
    SDL_LogSetOutputFunction(logOutputCallback, 0);
    SDL_Init(SDL_INIT_EVERYTHING);
    TTF_Init();
    SDLNet_Init();
#ifdef SERVER
    std::thread t([&] {
        runServer();
    });
    t.detach();
#endif
    IPaddress ip;
    SDLNet_ResolveHost(&ip, "plandisplayer.freeddns.org", SERVER_PORT); // TOOD: Create domain on freeddns.org
    TCPsocket socket = SDLNet_TCP_Open(&ip);
    SDL_GetMouseState(&mousePos.x, &mousePos.y);
    window = SDL_CreateWindow("PlanDisplayer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font* robotoF = TTF_OpenFont("res/roboto.ttf", 72);
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    SDL_RenderSetScale(renderer, w / (float)windowWidth, h / (float)windowHeight);
    SDL_AddEventWatch(eventWatch, 0);
    bool running = true;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(window);
    ImGui_ImplSDLRenderer_Init(renderer);
    bool showDemoWindow = false;
#if 0
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT || event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                running = false;
                // NOTE: On mobile remember to use eventWatch function (it doesn't reach this code when terminating)
            }
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
                SDL_RenderSetScale(renderer, event.window.data1 / (float)windowWidth, event.window.data2 / (float)windowHeight);
            }
            if (event.type == SDL_KEYDOWN) {
                keys[event.key.keysym.scancode] = true;
            }
            if (event.type == SDL_KEYUP) {
                keys[event.key.keysym.scancode] = false;
            }
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                buttons[event.button.button] = true;
            }
            if (event.type == SDL_MOUSEBUTTONUP) {
                buttons[event.button.button] = false;
            }
            if (event.type == SDL_MOUSEMOTION) {
                float scaleX, scaleY;
                SDL_RenderGetScale(renderer, &scaleX, &scaleY);
                mousePos.x = event.motion.x / scaleX;
                mousePos.y = event.motion.y / scaleY;
                realMousePos.x = event.motion.x;
                realMousePos.y = event.motion.y;
            }
        }
        POINT point;
        GetCursorPos(&point);
        std::cout << point.x << " " << point.y << std::endl;
        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        io.MousePos.x = mousePos.x;
        io.MousePos.y = mousePos.y;
        ImGui::NewFrame();
        if (showDemoWindow) {
            ImGui::ShowDemoWindow(&showDemoWindow);
        }
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);
    }
#endif
    // NOTE: On mobile remember to use eventWatch function (it doesn't reach this code when terminating)
    return 0;
}