#pragma once

#include <string>
#include <gdiplus.h>
#include <vlc/vlc.h>

using std::wstring;

// Event Window Messages
#define WM_MEDIAEVENT (WM_USER + 1)
#define WM_MEDIACOMPLETECALLBACK (WM_USER + 2)
#define WM_ERRORCALLBACK (WM_USER + 3)

enum PlayerState
{
    NoGraph = -100,
    Stopped = 0,
    Paused = 1,
    Running = 2,
};

enum VMR9AspectRatioMode
{
    VMR9ARMode_None = 0,
    VMR9ARMode_LetterBox = (VMR9ARMode_None + 1)
};

class VlcPlayer
{
private:
    HWND m_hwnd;
    libvlc_instance_t* m_vlcInstance;
    libvlc_media_player_t* m_mediaPlayer;

    CRITICAL_SECTION m_lock;

    libvlc_time_t m_duration;
    wstring m_filename;

    ULONG_PTR m_gdiToken;
    Gdiplus::GdiplusStartupInput m_gdiStartupInput;

    float m_volume;
    bool m_mute;
    int m_currentFps;

public:
    VlcPlayer(HWND hwnd);
    virtual ~VlcPlayer();

    virtual void SetFilename(const wchar_t* filename);
    virtual const wchar_t* GetFilename();
    virtual PlayerState GetFilterState();
    virtual void Play();
    virtual void Pause();
    virtual void Stop();
    virtual float GetVolume();
    virtual void SetVolume(float value);
    virtual bool GetMute();
    virtual void SetMute(bool value);
    virtual double GetDuration();
    virtual double GetPosition();
    virtual void SetPosition(double value);
    virtual void OnPaint();
    virtual void OnMoveWindow();
    virtual void OnDisplayModeChanged();
    virtual void OnGraphEvent();
    virtual HRESULT BlendApplicationImage(HBITMAP hbm, COLORREF transparencyKey, float alpha);
    virtual bool SetOverlayImageFile(const char* filename);
    virtual void GetVideoDims(RECT* srcRect);
    virtual void SetAspectMode(VMR9AspectRatioMode mode);
    virtual bool GetSupportsAlpha();
    virtual bool GetSupportsPng();
    virtual bool GetMediaIsHighFPS();
    virtual int GetMediaFramerate();

private:
    static void EventThunk(const struct libvlc_event_t* ev, void* instanceData);
    void VlcEventCallback(const struct libvlc_event_t* ev);

};