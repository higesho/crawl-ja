#pragma once

#ifdef USE_TILE_LOCAL
#ifdef USE_SDL

#include <array>

#include "gamepad.h"
#include "windowmanager.h"

struct SDL_Surface;
struct SDL_Window;
struct SDL_Cursor;
typedef void* SDL_GLContext;
struct _SDL_GameController;
union SDL_Event;

class SDLWrapper : public WindowManager
{
public:
    SDLWrapper();
    ~SDLWrapper();

    // Class functions
    virtual int init(coord_def *m_windowsz)
        override;

    // Environment state functions
    virtual void set_window_title(const char *title) override;
    virtual bool set_window_icon(const char* icon_name) override;
#ifdef TARGET_OS_WINDOWS
    virtual void set_window_placement(coord_def *m_windowsz);
#endif
    virtual unsigned char get_mod_state() const override;
    virtual void set_mod_state(tiles_key_mod mod) override;
    virtual void set_mouse_cursor(mouse_cursor_type id) override;
    virtual unsigned short get_mouse_state(int *x, int *y) const override;
    virtual string get_clipboard() override;
    virtual bool has_clipboard() override;

    // System time functions
    virtual unsigned int set_timer(unsigned int interval,
                                   wm_timer_callback callback) override;
    virtual void remove_timer(unsigned int& timer_id) override;
    virtual unsigned int get_ticks() const override;
    virtual void delay(unsigned int ms) override;

    // Event functions
    virtual int wait_event(wm_event *event, int timeout) override;
    virtual bool next_event_is(wm_event_type type) override;

    // Display functions
    virtual bool init_hidpi() override;
    virtual void resize(coord_def &m_windowsz) override;
    virtual void swap_buffers() override;
    virtual int screen_width() const override;
    virtual int screen_height() const override;
    virtual int desktop_width() const override;
    virtual int desktop_height() const override;

    // Texture loading
    virtual bool load_texture(GenericTexture *tex, const char *filename,
                              MipMapOptions mip_opt, unsigned int &orig_width,
                              unsigned int &orig_height,
                              tex_proc_func proc = nullptr,
                              bool force_power_of_two = true) override;

protected:
    // Helper functions
    SDL_Surface *load_image(const char *file) const;
    int send_textinput(wm_event *event);

    SDL_Window *m_window;
    SDL_GLContext m_context;
    int _desktop_width;
    int _desktop_height;

private:
    void glDebug(const char *msg);

    int prev_keycode;
    string m_textinput_queue;
    array<SDL_Cursor*, NUM_MOUSE_CURSORS> m_cursors;

    // ゲームパッド(ja-tools/GAMEPAD_DESIGN.md)
    bool pad_handle_event(const SDL_Event &event);
    void pad_pump_events();
    void pad_update();
    bool pad_take_key(int &key);
    bool pad_mouse_position(int &x, int &y);
    void pad_move_cursor(int dx, int dy);
    bool pad_click(const pad_output &out);
    void pad_open(int device_index);
    void pad_close();
    void pad_forget();

    _SDL_GameController *m_pad;
    int m_pad_id;          // 開いているパッドの SDL_JoystickID(無ければ -1)
    bool m_pad_focused;    // 前回見たとき窓が焦点を持っていたか
    gamepad_state m_pad_state;
};

#endif // USE_SDL
#endif // USE_TILE_LOCAL
