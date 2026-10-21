/**
 * @file
 * @brief ゲームパッド(ローカルのタイル版)の名前の表と状態機械。
 *
 * 設計は ja-tools/GAMEPAD_DESIGN.md。SDL にもゲームにも依存しない。
 */

#include "AppHdr.h"

#include "gamepad.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace
{
    struct pad_button_info
    {
        pad_button button;
        const char *name;
        // 既定の割り当て(GAMEPAD_DESIGN.md §2)。変愚の既定(LB=下り・RB=上り・
        // LT/RT にコマンド)に合わせ、X/Y は DCSS でいちばん押すものにした。
        const char *default_command;
    };

    const pad_button_info pad_buttons[NUM_PAD_BUTTONS] =
    {
        { PAD_A,     "A",     nullptr },                 // 決定(固定)
        { PAD_B,     "B",     nullptr },                 // 取消(固定)
        { PAD_X,     "X",     "CMD_EXPLORE" },           // 自動探索
        { PAD_Y,     "Y",     "CMD_AUTOFIGHT" },         // 自動戦闘
        { PAD_LB,    "LB",    "CMD_GO_DOWNSTAIRS" },     // 階段を下りる
        { PAD_RB,    "RB",    "CMD_GO_UPSTAIRS" },       // 階段を上る
        { PAD_LT,    "LT",    "CMD_DISPLAY_INVENTORY" }, // 所持品
        { PAD_RT,    "RT",    "CMD_FIRE" },              // 射撃
        { PAD_BACK,  "Back",  "CMD_GAME_MENU" },         // ゲームメニュー
        { PAD_START, "Start", "CMD_CAST_SPELL" },        // 呪文を唱える
        { PAD_L3,    "L3",    nullptr },                 // 左クリック(固定)
        { PAD_R3,    "R3",    nullptr },                 // 右クリック(固定)
    };

    // 別名(小文字で比べる)。
    const struct { const char *alias; pad_button button; } pad_aliases[] =
    {
        { "select", PAD_BACK },
        { "ls",     PAD_L3 },
        { "rs",     PAD_R3 },
        { "l1",     PAD_LB },
        { "r1",     PAD_RB },
        { "l2",     PAD_LT },
        { "r2",     PAD_RT },
    };

    std::string lowercase(const std::string &s)
    {
        std::string out = s;
        for (char &c : out)
            c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
        return out;
    }

    // スティックの 1 軸を -1/0/+1 に丸める。死区の出入りで別のしきい値(約束 3)。
    int latch(int current, int value)
    {
        if (value > gamepad_state::STICK_ENTER)
            return 1;
        if (value < -gamepad_state::STICK_ENTER)
            return -1;
        if (value < gamepad_state::STICK_LEAVE
            && value > -gamepad_state::STICK_LEAVE)
        {
            return 0;
        }
        return current; // 2 つのしきい値の間は今の状態のまま
    }

    // a が b より後(または同時)か。SDL_GetTicks の一巡りに耐えるよう差で見る。
    bool reached(uint32_t now, uint32_t when)
    {
        return static_cast<int32_t>(now - when) >= 0;
    }

    int remaining(uint32_t now, uint32_t when)
    {
        const int32_t left = static_cast<int32_t>(when - now);
        return left > 0 ? left : 0;
    }
}

const char *pad_button_name(pad_button b)
{
    if (b < 0 || b >= NUM_PAD_BUTTONS)
        return "?";
    return pad_buttons[b].name;
}

bool parse_pad_button(const std::string &name, pad_button &out)
{
    const std::string lower = lowercase(name);
    for (const pad_button_info &info : pad_buttons)
    {
        if (lower == lowercase(info.name))
        {
            out = info.button;
            return true;
        }
    }
    for (const auto &alias : pad_aliases)
    {
        if (lower == alias.alias)
        {
            out = alias.button;
            return true;
        }
    }
    return false;
}

bool pad_slot_valid(pad_button b, int mods)
{
    if (b < 0 || b >= NUM_PAD_BUTTONS || mods < 0 || mods >= NUM_PAD_LAYERS)
        return false;
    return mods == 0 || b != PAD_LB && b != PAD_RB;
}

std::string pad_slot_name(pad_button b, int mods)
{
    std::string name;
    if (mods & PAD_MOD_LB)
        name += "LB+";
    if (mods & PAD_MOD_RB)
        name += "RB+";
    return name + pad_button_name(b);
}

std::string pad_slot_name(int slot)
{
    const int mods = slot / PAD_SLOTS_PER_LAYER;
    const pad_button b = static_cast<pad_button>(slot % PAD_SLOTS_PER_LAYER);
    if (slot < 0 || !pad_slot_valid(b, mods))
        return "";
    return pad_slot_name(b, mods);
}

bool parse_pad_slot(const std::string &name, pad_button &button, int &mods)
{
    mods = 0;
    std::string rest = name;
    size_t plus;
    while ((plus = rest.find('+')) != std::string::npos)
    {
        pad_button modifier;
        if (!parse_pad_button(rest.substr(0, plus), modifier))
            return false;
        const int bit = modifier == PAD_LB ? PAD_MOD_LB
                      : modifier == PAD_RB ? PAD_MOD_RB : 0;
        if (!bit || (mods & bit))
            return false; // 修飾になれるのは LB と RB だけ(重ねて書くのも誤り)
        mods |= bit;
        rest = rest.substr(plus + 1);
    }
    return parse_pad_button(rest, button) && pad_slot_valid(button, mods);
}

// 同時押しの既定の割り当て(GAMEPAD_DESIGN.md §4.1)。パッドだけで遊べるよう、
// 既定の単独押しに無いよく使うコマンドを置く。LB の層は「動く・拾う・見る」、
// RB の層は「使う・読む・待つ」に寄せた。
namespace
{
    struct pad_chord_default
    {
        pad_button button;
        int mods;
        const char *command;
    };

    const pad_chord_default pad_chord_defaults[] =
    {
        { PAD_X,     PAD_MOD_LB, "CMD_REST" },                     // 休む・探す
        { PAD_X,     PAD_MOD_RB, "CMD_WAIT" },                     // 1 ターン待つ
        { PAD_A,     PAD_MOD_LB, "CMD_PICKUP" },                   // 拾う
        { PAD_A,     PAD_MOD_RB, "CMD_CLOSE_DOOR" },               // 扉を閉める
        { PAD_B,     PAD_MOD_LB, "CMD_DROP" },                     // 置く
        { PAD_B,     PAD_MOD_RB, "CMD_REPLAY_MESSAGES" },          // メッセージの履歴
        { PAD_Y,     PAD_MOD_LB, "CMD_INTERLEVEL_TRAVEL" },        // 階を指定して旅行(G)
        { PAD_Y,     PAD_MOD_RB, "CMD_USE_ABILITY" },              // 能力を使う
        { PAD_LT,    PAD_MOD_LB, "CMD_DISPLAY_MAP" },              // 階の地図(X)
        { PAD_LT,    PAD_MOD_RB, "CMD_QUAFF" },                    // 薬を飲む
        { PAD_RT,    PAD_MOD_LB, "CMD_LOOK_AROUND" },              // 見回す(x)
        { PAD_RT,    PAD_MOD_RB, "CMD_READ" },                     // 巻物を読む
        { PAD_START, PAD_MOD_LB, "CMD_MEMORISE_SPELL" },           // 呪文を覚える
        { PAD_START, PAD_MOD_RB, "CMD_DISPLAY_SKILLS" },           // スキル
        { PAD_BACK,  PAD_MOD_LB, "CMD_DISPLAY_CHARACTER_STATUS" }, // キャラクター概要(%)
        { PAD_BACK,  PAD_MOD_RB, "CMD_DISPLAY_OVERMAP" },          // ダンジョンの概観
    };
}

const char *pad_default_slot_command_name(pad_button b, int mods)
{
    if (mods == 0)
        return pad_default_command_name(b);
    for (const pad_chord_default &d : pad_chord_defaults)
        if (d.button == b && d.mods == mods)
            return d.command;
    return nullptr;
}

bool pad_slot_fixed(pad_button b, int mods)
{
    return mods == 0
           && (b == PAD_A || b == PAD_B || b == PAD_L3 || b == PAD_R3);
}

const char *pad_default_command_name(pad_button b)
{
    if (b < 0 || b >= NUM_PAD_BUTTONS)
        return nullptr;
    return pad_buttons[b].default_command;
}

void gamepad_state::set_button(pad_button b, bool down)
{
    if (b == PAD_LB || b == PAD_RB)
    {
        const int i = b == PAD_LB ? 0 : 1;
        if (down)
        {
            if (m_mod_held[i])
                return; // 押したままの知らせが重なった
            m_mod_held[i] = true;
            m_mod_used[i] = false;
            // LB と RB を重ねたら、どちらも単独押しではなくなる(LB+RB+ の層を
            // 作ったつもりで離した瞬間に階段を上り下りしないように)。
            if (m_mod_held[1 - i])
            {
                m_mod_used[0] = true;
                m_mod_used[1] = true;
            }
        }
        else
        {
            // 離した。その間に他のボタンを押していなければ単独押し。
            if (m_mod_held[i] && !m_mod_used[i])
                queue_press(b, 0);
            m_mod_held[i] = false;
            m_mod_used[i] = false;
        }
        return;
    }
    if (down)
        press(b);
}

void gamepad_state::press(pad_button b)
{
    int mods = 0;
    for (int i = 0; i < 2; ++i)
    {
        if (m_mod_held[i])
        {
            mods |= i == 0 ? PAD_MOD_LB : PAD_MOD_RB;
            m_mod_used[i] = true; // 離しても単独押しにしない
        }
    }
    queue_press(b, mods);
}

void gamepad_state::set_dpad(pad_dpad d, bool down)
{
    if (d >= 0 && d < NUM_PAD_DPAD)
        m_dpad[d] = down;
}

void gamepad_state::set_stick(int axis, int value)
{
    if (axis == 0)
        m_stick_x = latch(m_stick_x, value);
    else if (axis == 1)
        m_stick_y = latch(m_stick_y, value);
}

void gamepad_state::set_trigger(int which, int value)
{
    if (which < 0 || which > 1)
        return;
    // 立ち上がりでだけ押したことにする(約束 5。押しっぱなしで連射しない)。
    if (!m_trigger_down[which] && value > TRIGGER_DOWN)
    {
        m_trigger_down[which] = true;
        press(which == 0 ? PAD_LT : PAD_RT);
    }
    else if (m_trigger_down[which] && value < TRIGGER_UP)
        m_trigger_down[which] = false;
}

void gamepad_state::sync_trigger(int which, int value)
{
    if (which >= 0 && which <= 1)
        m_trigger_down[which] = value > TRIGGER_DOWN;
}

void gamepad_state::set_cursor_stick(int axis, int value)
{
    if (axis == 0)
        m_cursor_x = value;
    else if (axis == 1)
        m_cursor_y = value;
}

bool gamepad_state::cursor_active() const
{
    const double x = m_cursor_x, y = m_cursor_y;
    return x * x + y * y > double(CURSOR_DEADZONE) * CURSOR_DEADZONE;
}

bool gamepad_state::cursor_motion(uint32_t now, bool focused, int &dx, int &dy)
{
    dx = 0;
    dy = 0;
    if (!focused || !cursor_active())
    {
        m_cursor_moving = false;
        m_cursor_frac_x = 0;
        m_cursor_frac_y = 0;
        return false;
    }
    if (!m_cursor_moving)
    {
        // 倒し始め。時刻だけ覚え、次の刻みから動かす。
        m_cursor_moving = true;
        m_cursor_last = now;
        return false;
    }

    // 待ちが長引いても(重い処理のあとなど)まとめて大きく飛ばない。
    const uint32_t dt = std::min<uint32_t>(now - m_cursor_last, 100);
    m_cursor_last = now;

    const double x = m_cursor_x, y = m_cursor_y;
    const double mag = std::sqrt(x * x + y * y);
    const double t = (std::min(mag, 32767.0) - CURSOR_DEADZONE)
                     / (32767.0 - CURSOR_DEADZONE);
    // 2 乗の曲線: 浅く倒すと細かく、倒し切ると速く動く。
    const double speed = CURSOR_MIN_SPEED
                         + (CURSOR_MAX_SPEED - CURSOR_MIN_SPEED) * t * t;
    const double step = speed * dt / 1000.0;
    m_cursor_frac_x += x / mag * step;
    m_cursor_frac_y += y / mag * step;
    dx = static_cast<int>(m_cursor_frac_x);
    dy = static_cast<int>(m_cursor_frac_y);
    m_cursor_frac_x -= dx;
    m_cursor_frac_y -= dy;
    return dx != 0 || dy != 0;
}

void gamepad_state::current_direction(int &dx, int &dy) const
{
    dx = 0;
    dy = 0;
    if (m_dpad[PAD_DPAD_UP])
        dy -= 1;
    if (m_dpad[PAD_DPAD_DOWN])
        dy += 1;
    if (m_dpad[PAD_DPAD_LEFT])
        dx -= 1;
    if (m_dpad[PAD_DPAD_RIGHT])
        dx += 1;
    // 十字が中立のときだけスティックを見る(両方入っていれば十字を優先)。
    if (dx == 0 && dy == 0)
    {
        dx = m_stick_x;
        dy = m_stick_y;
    }
}

void gamepad_state::queue_direction(int dx, int dy)
{
    // 方向は 1 つまでしか溜めない。ゲームが受け取る前に kbhit() などで何度も
    // update() されても、押し続けた分の歩数が溜まって暴走しないように。
    for (const pad_output &o : m_outputs)
        if (o.is_direction)
            return;
    pad_output out;
    out.is_direction = true;
    out.dx = dx;
    out.dy = dy;
    // LB・RB を握ったままの方向は Shift・Ctrl+方向になる(走る・その場で攻撃など。
    // GAMEPAD_DESIGN.md §4.3)。修飾には「使った」印を付ける(離しても階段に入らない)。
    for (int i = 0; i < 2; ++i)
    {
        if (m_mod_held[i])
        {
            out.mods |= i == 0 ? PAD_MOD_LB : PAD_MOD_RB;
            m_mod_used[i] = true;
        }
    }
    m_outputs.push_back(out);
}

void gamepad_state::queue_press(pad_button b, int mods)
{
    pad_output out;
    out.button = b;
    out.mods = mods;
    m_outputs.push_back(out);
}

void gamepad_state::emit_direction(int dx, int dy, uint32_t now,
                                   int repeat_delay)
{
    queue_direction(dx, dy);
    m_held_dx = dx;
    m_held_dy = dy;
    m_pending = false;
    m_next_repeat = now + (repeat_delay > 0 ? repeat_delay : 0);
}

void gamepad_state::update(uint32_t now, bool focused, int repeat_delay)
{
    if (!focused)
    {
        // 裏に回した窓のパッドで歩かせない(約束 1)。戻ったら中立から数え直す。
        forget();
        return;
    }

    m_repeat = repeat_delay > 0;

    int dx, dy;
    current_direction(dx, dy);

    if (dx == 0 && dy == 0)
    {
        // 斜めの 2 つ目を待っている間に離した = 短く叩いた。取りこぼさずに 1 回出す。
        if (m_pending)
            queue_direction(m_pending_dx, m_pending_dy);
        m_pending = false;
        m_held_dx = 0;
        m_held_dy = 0;
        m_rearm_pending = false; // 中立を見た。ここから受け付ける(約束 2)
        return;
    }
    if (m_rearm_pending)
        return; // まだ一度も中立を見ていない(挿した直後・焦点が戻った直後)

    const bool diagonal = dx != 0 && dy != 0;

    if (m_pending)
    {
        m_pending_dx = dx;
        m_pending_dy = dy;
        if (diagonal || reached(now, m_pending_since + COMBINE_MS))
            emit_direction(dx, dy, now, repeat_delay);
        return;
    }

    if (m_held_dx == 0 && m_held_dy == 0)
    {
        // 中立から入った。斜めならすぐ出す。1 方向だけなら斜めの 2 つ目を少し待つ
        // (十字の 2 つのボタンはわずかにずれて届くので、すぐ出すと狙った斜めの
        // 前に縦か横へ 1 歩出てしまう)。
        if (diagonal)
            emit_direction(dx, dy, now, repeat_delay);
        else
        {
            m_pending = true;
            m_pending_since = now;
            m_pending_dx = dx;
            m_pending_dy = dy;
        }
        return;
    }

    if (dx == m_held_dx && dy == m_held_dy)
    {
        // 押し続けている。
        if (repeat_delay > 0 && reached(now, m_next_repeat))
        {
            queue_direction(dx, dy);
            m_next_repeat = now + REPEAT_MS;
        }
        return;
    }

    const bool held_diagonal = m_held_dx != 0 && m_held_dy != 0;
    if (held_diagonal && !diagonal
        && (dx == m_held_dx && dy == 0 || dx == 0 && dy == m_held_dy))
    {
        // 斜めから片方だけ先に離した。離す途中の一瞬を 1 歩にしない。
        // 押し続けたときだけ、リピートの待ちのあとで出す。
        m_held_dx = dx;
        m_held_dy = dy;
        m_next_repeat = now + (repeat_delay > 0 ? repeat_delay : 0);
        return;
    }

    // 曲がった・斜めへ足した。すぐ出し、リピートの待ちは数え直す(約束 4)。
    emit_direction(dx, dy, now, repeat_delay);
}

int gamepad_state::ms_until_due(uint32_t now) const
{
    if (!m_outputs.empty())
        return 0;
    if (m_pending)
        return remaining(now, m_pending_since + COMBINE_MS);
    int due = -1;
    if ((m_held_dx != 0 || m_held_dy != 0) && m_repeat)
        due = remaining(now, m_next_repeat);
    if (cursor_active())
    {
        // 右スティックを倒している間は、刻みごとに起きてカーソルを動かす。
        const int cursor_due = m_cursor_moving
            ? remaining(now, m_cursor_last + CURSOR_TICK_MS) : CURSOR_TICK_MS;
        if (due < 0 || cursor_due < due)
            due = cursor_due;
    }
    return due;
}

bool gamepad_state::take_output(pad_output &out)
{
    if (m_outputs.empty())
        return false;
    out = m_outputs.front();
    m_outputs.pop_front();
    return true;
}

void gamepad_state::forget()
{
    for (bool &d : m_dpad)
        d = false;
    m_stick_x = 0;
    m_stick_y = 0;
    // トリガの実際の状態は呼ぶ側が sync_trigger() で合わせる(SDL を知らないため)。
    m_trigger_down[0] = false;
    m_trigger_down[1] = false;
    m_held_dx = 0;
    m_held_dy = 0;
    m_pending = false;
    m_rearm_pending = true;
    // 修飾を握ったままにしない(焦点が戻った瞬間に同時押しが成立しないように)。
    for (int i = 0; i < 2; ++i)
    {
        m_mod_held[i] = false;
        m_mod_used[i] = false;
    }
    m_cursor_x = 0;
    m_cursor_y = 0;
    m_cursor_moving = false;
    m_cursor_frac_x = 0;
    m_cursor_frac_y = 0;
    m_outputs.clear();
}
