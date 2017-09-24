#include "stdafx.h"
#include "VlcPlayer.h"

// Helper function -> convert std::wstring to UTF8 encoded string
std::string utf8_encode(const std::wstring &wstr)
{
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}
// Helper function -> convert std::string in UTF8 encoding to std::wstring
std::wstring utf8_decode(const std::string &str)
{
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}


VlcPlayer::VlcPlayer(HWND hwnd) : m_vlcInstance(NULL),
                                  m_mediaPlayer(NULL),
                                  m_hwnd(hwnd),
                                  m_currentFps(-1)
{
    m_vlcInstance = libvlc_new(0, NULL);
    if (m_vlcInstance == NULL) return;

    InitializeCriticalSection(&m_lock);
}
VlcPlayer::~VlcPlayer()
{
    EnterCriticalSection(&m_lock);
    if (m_mediaPlayer)
    {
        libvlc_media_player_release(m_mediaPlayer);
        m_mediaPlayer = NULL;
    }
    if (m_vlcInstance)
    {
        libvlc_release(m_vlcInstance);
        m_vlcInstance = NULL;
    }
    LeaveCriticalSection(&m_lock);
    DeleteCriticalSection(&m_lock);

    //DeleteFile(L"overlay.png");
}

void VlcPlayer::SetFilename(const wchar_t* filename)
{
    if (filename == NULL) return;

    EnterCriticalSection(&m_lock);

    m_duration = -1;

    libvlc_event_manager_t* evMgr = NULL;

    if (m_mediaPlayer != NULL)
    {
        evMgr = libvlc_media_player_event_manager(m_mediaPlayer);
        libvlc_event_detach(evMgr, libvlc_MediaPlayerEndReached, &VlcPlayer::EventThunk, (void*)this);

        libvlc_media_player_set_hwnd(m_mediaPlayer, NULL);

        libvlc_media_player_stop(m_mediaPlayer);
        libvlc_media_player_release(m_mediaPlayer);
        m_mediaPlayer = NULL;
    }

    m_filename = wstring(filename);
    std::string fn = utf8_encode(m_filename);
    libvlc_media_t* media = libvlc_media_new_path(m_vlcInstance, fn.c_str());
    m_mediaPlayer = libvlc_media_player_new_from_media(media);
    libvlc_media_release(media);

    libvlc_media_player_set_hwnd(m_mediaPlayer, (void*)m_hwnd);

    libvlc_video_set_mouse_input(m_mediaPlayer, 0);
    libvlc_video_set_key_input(m_mediaPlayer, 0);

    m_currentFps = -1;

    evMgr = libvlc_media_player_event_manager(m_mediaPlayer);
    if (evMgr != NULL)
    {
        libvlc_event_attach(evMgr, libvlc_MediaPlayerEndReached, &VlcPlayer::EventThunk, (void*)this);
    }

    LeaveCriticalSection(&m_lock);
}
const wchar_t* VlcPlayer::GetFilename()
{
    return m_filename.c_str();
}
PlayerState VlcPlayer::GetFilterState()
{
    EnterCriticalSection(&m_lock);

    if (m_mediaPlayer == NULL)
    {
        LeaveCriticalSection(&m_lock);
        return PlayerState::NoGraph;
    }
    libvlc_state_t state = libvlc_media_player_get_state(m_mediaPlayer);
    PlayerState result;
    switch (state)
    {
    case libvlc_state_t::libvlc_Playing:
        result = PlayerState::Running;
        break;
    case libvlc_state_t::libvlc_Paused:
        result = PlayerState::Paused;
        break;
    case libvlc_state_t::libvlc_Stopped:
        result = PlayerState::Stopped;
        break;
    default:
        result = PlayerState::NoGraph;
        break;
    }
    LeaveCriticalSection(&m_lock);
    return result;
}
void VlcPlayer::Play()
{
    EnterCriticalSection(&m_lock);

    if (m_mediaPlayer != NULL)
        libvlc_media_player_play(m_mediaPlayer);

    LeaveCriticalSection(&m_lock);
}
void VlcPlayer::Pause()
{
    EnterCriticalSection(&m_lock);

    if (m_mediaPlayer != NULL)
        libvlc_media_player_pause(m_mediaPlayer);

    LeaveCriticalSection(&m_lock);
}
void VlcPlayer::Stop()
{
    EnterCriticalSection(&m_lock);

    if (m_mediaPlayer != NULL)
        libvlc_media_player_stop(m_mediaPlayer);

    LeaveCriticalSection(&m_lock);
}
float VlcPlayer::GetVolume()
{
    EnterCriticalSection(&m_lock);
    if (m_mediaPlayer != NULL)
    {
        float result = (float)libvlc_audio_get_volume(m_mediaPlayer);
        LeaveCriticalSection(&m_lock);
        return result;
    }
    else
    {
        LeaveCriticalSection(&m_lock);
        return 0.f;
    }
}
void VlcPlayer::SetVolume(float value)
{
    EnterCriticalSection(&m_lock);

    if (value < 0) value = 0;
    else if (value > 100) value = 100;

    if (m_mediaPlayer != NULL)
        libvlc_audio_set_volume(m_mediaPlayer, (int)value);

    LeaveCriticalSection(&m_lock);
}
bool VlcPlayer::GetMute()
{
    EnterCriticalSection(&m_lock);

    if (m_mediaPlayer != NULL)
    {
        bool result = libvlc_audio_get_mute(m_mediaPlayer) != FALSE;
        LeaveCriticalSection(&m_lock);
        return result;
    }
    else
    {
        LeaveCriticalSection(&m_lock);
        return false;
    }
}
void VlcPlayer::SetMute(bool value)
{
    EnterCriticalSection(&m_lock);

    if (m_mediaPlayer != NULL)
        libvlc_audio_set_mute(m_mediaPlayer, value ? TRUE : FALSE);

    LeaveCriticalSection(&m_lock);
}
double VlcPlayer::GetDuration()
{
    EnterCriticalSection(&m_lock);
    if (m_mediaPlayer == NULL)
    {
        LeaveCriticalSection(&m_lock);
        return 0;
    }

    if (m_duration <= 0)
    {
        m_duration = libvlc_media_player_get_length(m_mediaPlayer);
    }

    LeaveCriticalSection(&m_lock);

    return (double)m_duration / 1000.0;
}
double VlcPlayer::GetPosition()
{
    EnterCriticalSection(&m_lock);

    if (m_mediaPlayer == NULL)
    {
        LeaveCriticalSection(&m_lock);
        return 0.0;
    }

    libvlc_time_t position = libvlc_media_player_get_time(m_mediaPlayer);

    LeaveCriticalSection(&m_lock);

    return (double)position / 1000.0;

}
void VlcPlayer::SetPosition(double value)
{
    EnterCriticalSection(&m_lock);

    if (m_mediaPlayer == NULL)
    {
        LeaveCriticalSection(&m_lock);
        return;
    }

    libvlc_time_t duration = libvlc_media_player_get_length(m_mediaPlayer);

    libvlc_time_t position = (libvlc_time_t)(value * 1000);

    if (position < 0) position = 0;
    else if (position >= duration) position = duration - 1;

    libvlc_media_player_set_time(m_mediaPlayer, position);

    LeaveCriticalSection(&m_lock);
}
void VlcPlayer::OnPaint()
{
    // todo?
}
void VlcPlayer::OnMoveWindow()
{
    // todo?
}
void VlcPlayer::OnDisplayModeChanged()
{
    // todo?
}
void VlcPlayer::OnGraphEvent()
{
    // todo?
}
HRESULT VlcPlayer::BlendApplicationImage(HBITMAP hbm, COLORREF transparencyKey, float alpha)
{
    return E_FAIL;
}

bool VlcPlayer::SetOverlayImageFile(const char* filename)
{
    EnterCriticalSection(&m_lock);
    if (m_mediaPlayer == NULL)
    {
        LeaveCriticalSection(&m_lock);
        return false;
    }

    m_currentFps = libvlc_media_player_get_fps(m_mediaPlayer);

    if (m_currentFps > 40)
    {
        libvlc_video_set_logo_int(m_mediaPlayer, libvlc_logo_enable, 0);
    }
    else if (filename == NULL)
    {
        libvlc_video_set_logo_int(m_mediaPlayer, libvlc_logo_enable, 0);
    }
    else
    {
        libvlc_video_set_logo_string(m_mediaPlayer, libvlc_logo_file, filename);
        libvlc_video_set_logo_int(m_mediaPlayer, libvlc_logo_x, 0);
        libvlc_video_set_logo_int(m_mediaPlayer, libvlc_logo_y, 0);

        libvlc_video_set_logo_int(m_mediaPlayer, libvlc_logo_enable, 1);
    }

    LeaveCriticalSection(&m_lock);
    return true;
}

void VlcPlayer::GetVideoDims(RECT* srcRect)
{
    EnterCriticalSection(&m_lock);
    if (m_mediaPlayer == NULL)
    {
        LeaveCriticalSection(&m_lock);
        return;
    }

    unsigned int w, h;
    libvlc_video_get_size(m_mediaPlayer, 0, &w, &h);
    SetRect(srcRect, 0, 0, w, h);

    LeaveCriticalSection(&m_lock);
}
void VlcPlayer::SetAspectMode(VMR9AspectRatioMode mode)
{
    EnterCriticalSection(&m_lock);

    if (m_mediaPlayer == NULL)
    {
        LeaveCriticalSection(&m_lock);
        return;
    }


    LeaveCriticalSection(&m_lock);
}

bool VlcPlayer::GetSupportsAlpha()
{
    return true;
}
bool VlcPlayer::GetSupportsPng()
{
    return true;
}
bool VlcPlayer::GetMediaIsHighFPS()
{
    if (m_currentFps < 0) GetMediaFramerate();
    return m_currentFps > 40;
}
int VlcPlayer::GetMediaFramerate()
{
    EnterCriticalSection(&m_lock);
    if (m_currentFps < 0 && m_mediaPlayer != NULL)
    {
        m_currentFps = libvlc_media_player_get_fps(m_mediaPlayer);
    }
    LeaveCriticalSection(&m_lock);

    return m_currentFps;
}

void VlcPlayer::EventThunk(const struct libvlc_event_t* ev, void* instanceData)
{
    VlcPlayer* player = (VlcPlayer*)instanceData;
    player->VlcEventCallback(ev);
}
void VlcPlayer::VlcEventCallback(const struct libvlc_event_t* ev)
{
    switch (ev->type)
    {
    case libvlc_MediaPlayerEndReached:
        ::PostMessageW(m_hwnd, WM_MEDIACOMPLETECALLBACK, (WPARAM)NULL, (LPARAM)NULL);
        break;
    }
}