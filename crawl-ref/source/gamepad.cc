/**
 * @file
 * @brief ゲームパッド(ローカルのタイル版)の名前の表と状態機械。
 *
 * 設計は ja-tools/GAMEPAD_DESIGN.md。SDL にもゲームにも依存しない。
 */

#include "AppHdr.h"

#include "gamepad.h"

#include <cctype>

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
        { PAD_L3,    "L3",    "CMD_REST" },              // 休む・探す
        { PAD_R3,    "R3",    "CMD_USE_ABILITY" },       // 能力を使う
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

const char *pad_default_command_name(pad_button b)
{
    if (b < 0 || b >= NUM_PAD_BUTTONS)
        return nullptr;
    return pad_buttons[b].default_command;
}

void gamepad_state::set_button(pad_button b, bool down)
{
    if (down)
        queue_press(b);
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
        queue_press(which == 0 ? PAD_LT : PAD_RT);
    }
    else if (m_trigger_down[which] && value < TRIGGER_UP)
        m_trigger_down[which] = false;
}

void gamepad_state::sync_trigger(int which, int value)
{
    if (which >= 0 && which <= 1)
        m_trigger_down[which] = value > TRIGGER_DOWN;
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
    m_outputs.push_back(out);
}

void gamepad_state::queue_press(pad_button b)
{
    pad_output out;
    out.button = b;
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
    if ((m_held_dx != 0 || m_held_dy != 0) && m_repeat)
        return remaining(now, m_next_repeat);
    return -1;
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
    m_outputs.clear();
}
