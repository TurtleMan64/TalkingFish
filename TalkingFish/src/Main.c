#include <SDL2/SDL.h> 
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "../resource.h"

static volatile HMENU win32MenuMicrophone;

static volatile int currentMicAmplitude = 0;
static int AMPLITUDE_THRESHOLD_LOW  =  500;
static int AMPLITUDE_THRESHOLD_HIGH = 1000;

enum FishState
{
    Resting,
    RestToTalk,
    Talking,
    TalkToRest
};

void SDL_MicrophoneCallback(void* userdata, Uint8* stream, int len);

LRESULT CALLBACK Win32WindowCallback(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);

static volatile SDL_AudioDeviceID sdlRecordingDevice;
int NUM_RECORDING_DEVICES = 0;
int currentRecordingDeviceId = -1;
#define MICROPHONE_01  1
#define MICROPHONE_02  2
#define MICROPHONE_03  3
#define MICROPHONE_04  4
#define MICROPHONE_05  5
#define MICROPHONE_06  6
#define MICROPHONE_07  7
#define MICROPHONE_08  8
#define MICROPHONE_09  9
#define MICROPHONE_10 10
void switchToNewRecordingDevice(int newDeviceId);

void loadSettings();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    hPrevInstance;
    lpCmdLine;
    nCmdShow;

    loadSettings();

    SDL_Init(SDL_INIT_AUDIO);
    NUM_RECORDING_DEVICES = SDL_GetNumAudioDevices(1);

    WNDCLASSW wc = {0};
    wc.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wc.lpszClassName = L"Fish";
    wc.lpfnWndProc   = Win32WindowCallback;

    if (!RegisterClassW(&wc))
    {
        return -1;
    }

    DWORD style = WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
    int width  = 500;
    int height = 281;
    RECT rect;
    rect.left   = GetSystemMetrics(SM_CXSCREEN)/2 - width/2;
    rect.top    = GetSystemMetrics(SM_CYSCREEN)/2 - height/2;
    rect.right  = rect.left + width;
    rect.bottom = rect.top  + height;

    AdjustWindowRect(&rect, style, FALSE);

    HWND win32Window = CreateWindow("Fish", "Fish", style, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, hInstance, NULL);

    HMENU win32Menu     = CreateMenu();
    win32MenuMicrophone = CreateMenu();

    for (int i = 0; i < NUM_RECORDING_DEVICES; i++)
    {
        AppendMenu(win32MenuMicrophone, MF_STRING, MICROPHONE_01 + i, SDL_GetAudioDeviceName(i, 1));
        CheckMenuItem(win32MenuMicrophone, MICROPHONE_01 + i, MF_UNCHECKED);
    }
    CheckMenuItem(win32MenuMicrophone, MICROPHONE_01, MF_CHECKED);

    AppendMenu(win32Menu, MF_POPUP, (UINT_PTR)win32MenuMicrophone, "Microphone Select");

    SetMenu(win32Window, win32Menu);

    HBITMAP animationTurn[13];
    animationTurn[ 0] = (HBITMAP)LoadImage(NULL, "images/frame_000.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    animationTurn[ 1] = (HBITMAP)LoadImage(NULL, "images/frame_001.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    animationTurn[ 2] = (HBITMAP)LoadImage(NULL, "images/frame_002.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    animationTurn[ 3] = (HBITMAP)LoadImage(NULL, "images/frame_003.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    animationTurn[ 4] = (HBITMAP)LoadImage(NULL, "images/frame_004.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    animationTurn[ 5] = (HBITMAP)LoadImage(NULL, "images/frame_005.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    animationTurn[ 6] = (HBITMAP)LoadImage(NULL, "images/frame_006.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    animationTurn[ 7] = (HBITMAP)LoadImage(NULL, "images/frame_007.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    animationTurn[ 8] = (HBITMAP)LoadImage(NULL, "images/frame_008.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    animationTurn[ 9] = (HBITMAP)LoadImage(NULL, "images/frame_009.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    animationTurn[10] = (HBITMAP)LoadImage(NULL, "images/frame_010.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    animationTurn[11] = (HBITMAP)LoadImage(NULL, "images/frame_011.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    animationTurn[12] = (HBITMAP)LoadImage(NULL, "images/frame_012.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

    HBITMAP animationTalk[4];
    animationTalk[ 0] = (HBITMAP)LoadImage(NULL, "images/frame_098.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    animationTalk[ 1] = (HBITMAP)LoadImage(NULL, "images/frame_099.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    animationTalk[ 2] = (HBITMAP)LoadImage(NULL, "images/frame_100.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    animationTalk[ 3] = (HBITMAP)LoadImage(NULL, "images/frame_101.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);


    HWND win32FishImage = CreateWindowA("Static", NULL, WS_VISIBLE | WS_CHILD | SS_BITMAP, 0, 0, 100, 100, win32Window, NULL, NULL, NULL);
    SendMessage(win32FishImage, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)animationTurn[0]);

    ShowWindow(win32Window, SW_SHOW);
    
    switchToNewRecordingDevice(0);
    
    Uint64 timePrev = SDL_GetTicks64();
    
    enum FishState fishState = Resting;
    Uint64 fishStateTimer = 0;
    
    HBITMAP* imagePrev = NULL;

    MSG msg = {0};
    char running = 1;
    while (running)
    {
        Sleep(20);
        Uint64 timeNew = SDL_GetTicks64();
        Uint64 dt = timeNew - timePrev;
        timePrev = timeNew;

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
            {
                running = 0;
            }
        }

        HBITMAP* imageNew = NULL;

        switch (fishState)
        {
            case Resting:
            {
                if (currentMicAmplitude >= AMPLITUDE_THRESHOLD_LOW)
                {
                    fishStateTimer += dt;
                }
                else
                {
                    fishStateTimer = 0;
                }
        
                if (fishStateTimer > 0)
                {
                    fishState = RestToTalk;
                    fishStateTimer = 0;
                }
                imageNew = &animationTurn[0];
                break;
            }
        
            case RestToTalk:
            {
                fishStateTimer += dt;
        
                const Uint64 REST_TO_TALK_TIME = 150;
        
                float perc = ((float)fishStateTimer)/(REST_TO_TALK_TIME);
                int idx = (int)(perc*13);
                if (idx < 0)
                {
                    idx = 0;
                }
                else if (idx > 12)
                {
                    idx = 12;
                }
        
                if (fishStateTimer >= REST_TO_TALK_TIME)
                {
                    fishState = Talking;
                    fishStateTimer = 0;
                }
                imageNew = &animationTurn[idx];
                break;
            }
        
            case Talking:
            {
                if (currentMicAmplitude < AMPLITUDE_THRESHOLD_LOW)
                {
                    fishStateTimer += dt;
                }
                else
                {
                    fishStateTimer = 0;
                }
        
                float perc = ((float)(currentMicAmplitude-AMPLITUDE_THRESHOLD_LOW))/(AMPLITUDE_THRESHOLD_HIGH-AMPLITUDE_THRESHOLD_LOW);
                int idx = (int)(perc*4);
                if (idx < 0)
                {
                    idx = 0;
                }
                else if (idx > 3)
                {
                    idx = 3;
                }
        
                if (fishStateTimer > 2500)
                {
                    fishState = TalkToRest;
                    fishStateTimer = 0;
                }
        
                imageNew = &animationTalk[idx];
                break;
            }
        
            case TalkToRest:
            {
                fishStateTimer += dt;
        
                const Uint64 TALK_TO_REST_TIME = 250;
        
                float perc = ((float)fishStateTimer)/(TALK_TO_REST_TIME);
                int idx = (int)(perc*13);
                if (idx < 0)
                {
                    idx = 0;
                }
                else if (idx > 12)
                {
                    idx = 12;
                }
        
                if (fishStateTimer >= TALK_TO_REST_TIME)
                {
                    fishState = Resting;
                    fishStateTimer = 0;
                }
        
                imageNew = &animationTurn[12 - idx];
                break;
            }
                
            default: break;
        }

        if (imageNew != imagePrev)
        {
            imagePrev = imageNew;
            SendMessage(win32FishImage, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)(*imageNew));
        }
    }

    SDL_CloseAudioDevice(sdlRecordingDevice);
    SDL_Quit();
    
    return 0;
}

LRESULT CALLBACK Win32WindowCallback(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_COMMAND:
        switch (wp)
        {
            case MICROPHONE_01:
            case MICROPHONE_02:
            case MICROPHONE_03:
            case MICROPHONE_04:
            case MICROPHONE_05:
            case MICROPHONE_06:
            case MICROPHONE_07:
            case MICROPHONE_08:
            case MICROPHONE_09:
            case MICROPHONE_10:
            {
                switchToNewRecordingDevice(((int)wp) - 1);
                break;
            }

            default: break;
        }

    default:
        return DefWindowProcW(hWnd, msg, wp, lp);
    }

    return 0;
}

void SDL_MicrophoneCallback(void* userdata, Uint8* stream, int len)
{
    userdata;

    int localMax = 0;
    int numSamples = len/2;
    for (int i = 0; i < numSamples; i++)
    {
        short sample;
        memcpy(&sample, &stream[i*2], 2);

        int d = (int)(sample);
        if (d < 0)
        {
            d = -d;
        }
    
        if (d > localMax)
        {
            localMax = d;
        }
    }

    currentMicAmplitude = localMax;
}

void switchToNewRecordingDevice(int newDeviceId)
{
    int recordingDevicePrev = currentRecordingDeviceId;
    currentRecordingDeviceId = newDeviceId;
    
    for (int i = 0; i < NUM_RECORDING_DEVICES; i++)
    {
        CheckMenuItem(win32MenuMicrophone, MICROPHONE_01 + i, MF_UNCHECKED);
    }
    CheckMenuItem(win32MenuMicrophone, MICROPHONE_01 + currentRecordingDeviceId, MF_CHECKED);
    
    if (currentRecordingDeviceId != recordingDevicePrev)
    {
        SDL_PauseAudioDevice(sdlRecordingDevice, 1);
        SDL_CloseAudioDevice(sdlRecordingDevice);
        
        SDL_AudioSpec want, have;
        SDL_zero(want);
        want.freq = 44100;
        want.format = AUDIO_S16LSB;
        want.channels = 1;
        want.samples = 128;
        want.callback = SDL_MicrophoneCallback;
        sdlRecordingDevice = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(currentRecordingDeviceId, 1), 1, &want, &have, 0);
        SDL_PauseAudioDevice(sdlRecordingDevice, 0);
    }
}

void loadSettings()
{
    FILE* file = NULL;
    errno_t err = fopen_s(&file, "Settings.txt", "rb");
    if (err)
    {
        printf("Problem when trying to load settings file = %d\n", err);
        return;
    }

    char buf[1024];

    char c = ' ';
    while (c != ':')
    {
        size_t numRead = fread(&c, 1, 1, file);
        if (numRead != 1)
        {
            printf("End of file reached\n");
            fclose(file);
            return;
        }
    }

    for (int i = 0; i < 1024; i++)
    {
        size_t numRead = fread(&c, 1, 1, file);
        if (numRead != 1)
        {
            printf("End of file reached\n");
            fclose(file);
            return;
        }

        if (c == '%')
        {
            buf[i] = 0;
            double val = atof(buf);
            if (val < 0)
            {
                val = 0.0;
            }
            else if (val > 100)
            {
                val = 100;
            }
            AMPLITUDE_THRESHOLD_LOW = (int)((val*0.01)*32766);
            break;
        }
        else
        {
            buf[i] = c;
        }
    }

    c = ' ';
    while (c != ':')
    {
        size_t numRead = fread(&c, 1, 1, file);
        if (numRead != 1)
        {
            printf("End of file reached\n");
            fclose(file);
            return;
        }
    }

    for (int i = 0; i < 1024; i++)
    {
        size_t numRead = fread(&c, 1, 1, file);
        if (numRead != 1)
        {
            printf("End of file reached\n");
            fclose(file);
            return;
        }

        if (c == '%')
        {
            buf[i] = 0;
            double val = atof(buf);
            if (val < 0)
            {
                val = 0.0;
            }
            else if (val > 100)
            {
                val = 100;
            }
            AMPLITUDE_THRESHOLD_HIGH = (int)((val*0.01)*32766);
            break;
        }
        else
        {
            buf[i] = c;
        }
    }

    fclose(file);
}
