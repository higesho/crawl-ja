/**
 * @file
 * @brief ゲームパッド(ローカルのタイル版)の名前の表と状態機械。
 *
 * 設計は ja-tools/GAMEPAD_DESIGN.md。手本はユーザーの変愚蛮怒 SDL2 版
 * (hd2d/ui/game_pad.h)。SDL にもゲームにも依存しない部分だけをここに置き、
 * 単体で検査できるようにしてある(ja-tools/gamepad_test/)。SDL の出来事の
 * 読み取りと「押し → キー」の変換は windowmanager-sdl.cc が受け持つ。
 *
 * 落とすと必ず壊れる約束(変愚の game_pad.h 冒頭と同じ):
 *  1. 窓が入力の焦点を持っていないときは方向を出さない。
 *  2. 焦点が戻った・パッドを挿した直後は、一度中立を見るまで方向を受け付けない。
 *  3. スティックの死区は出入りで別のしきい値にする(ヒステリシス)。
 *  4. 方向を入れ替えたときもリピートの間隔を置く。
 *  5. トリガは軸。立ち上がりでだけ押したことにする。
 */

#pragma once

#include <cstdint>
#include <deque>
#include <string>

// 割り当てられるボタン。方向(十字・スティック)は含まない(移動専用)。
enum pad_button
{
    PAD_A,
    PAD_B,
    PAD_X,
    PAD_Y,
    PAD_LB,
    PAD_RB,
    PAD_LT,
    PAD_RT,
    PAD_BACK,
    PAD_START,
    PAD_L3,
    PAD_R3,
    NUM_PAD_BUTTONS
};

enum pad_dpad
{
    PAD_DPAD_UP,
    PAD_DPAD_DOWN,
    PAD_DPAD_LEFT,
    PAD_DPAD_RIGHT,
    NUM_PAD_DPAD
};

// 同時押しの修飾(GAMEPAD_DESIGN.md §4.1)。LB / RB を押しながら他のボタンを押す。
enum
{
    PAD_MOD_LB = 1,
    PAD_MOD_RB = 2,
    NUM_PAD_LAYERS = 4, // 単独・LB+・RB+・LB+RB+
};

// 割り当ての枠の番号(Options.pad_binds の鍵。マクロのキーコードもこれから作る)。
// 層ごとに 16 ずつ空けてある。
static const int PAD_SLOTS_PER_LAYER = 16;
static const int NUM_PAD_SLOTS = NUM_PAD_LAYERS * PAD_SLOTS_PER_LAYER;
inline int pad_slot(pad_button b, int mods = 0)
{
    return mods * PAD_SLOTS_PER_LAYER + b;
}

// その層でそのボタンが枠になるか。修飾自身(LB+LB、LB+RB など)は押しようがないので枠にしない。
bool pad_slot_valid(pad_button b, int mods);
// 枠の綴り(X、LB+X、RB+X、LB+RB+X)。
std::string pad_slot_name(pad_button b, int mods);
// 枠の番号から綴りへ。枠でなければ空。
std::string pad_slot_name(int slot);
// 綴りから読む(LB+X、RB+LB+X も読む。大文字小文字は問わない)。
bool parse_pad_slot(const std::string &name, pad_button &button, int &mods);

// rc とログに出す綴り(A, LB, Back ...)。
const char *pad_button_name(pad_button b);
// 綴りから読む(大文字小文字は問わない。Select, LS/RS, L1/R1, L2/R2 も読む)。
bool parse_pad_button(const std::string &name, pad_button &out);

// 単独押しで固定の役目を持ち、割り当てられないボタンか。A = 決定、B = 取消、
// L3 = 左クリック、R3 = 右クリック。同時押しの層(LB+A など)は枠として割り当てられる。
bool pad_slot_fixed(pad_button b, int mods);

// 既定の割り当て(コマンド名。bindkey と同じ綴り)。無ければ nullptr。
// 単独押しが固定のボタン(A・B・L3・R3)は nullptr。
const char *pad_default_command_name(pad_button b);

// パッドが出すもの 1 つ。方向か、ボタンの押し。
struct pad_output
{
    bool is_direction = false;
    int dx = 0, dy = 0;          // 方向のとき -1..1
    pad_button button = PAD_A;   // 押しのとき
    int mods = 0;                // 押しのとき、同時押しの修飾(PAD_MOD_*)
};

class gamepad_state
{
public:
    // スティックのしきい値(ヒステリシス。約束 3)。
    static const int STICK_ENTER = 18000;
    static const int STICK_LEAVE = 11000;
    // トリガのしきい値(約束 5)。
    static const int TRIGGER_DOWN = 16000;
    static const int TRIGGER_UP = 9000;
    // 中立から 1 方向だけ入ったとき、斜めの 2 つ目を待つ時間。
    static const int COMBINE_MS = 40;
    // 2 回目以降のリピートの間隔。
    static const int REPEAT_MS = 90;
    // 右スティックで動かすマウスのカーソル(GAMEPAD_DESIGN.md §2.2)。
    // 死区は円で取る。倒し切ったときの速さは 1 秒あたりの窓の座標。
    static const int CURSOR_DEADZONE = 8000;
    static const int CURSOR_MIN_SPEED = 60;
    static const int CURSOR_MAX_SPEED = 1400;
    // 倒している間、カーソルを動かす間隔。
    static const int CURSOR_TICK_MS = 16;

    void set_button(pad_button b, bool down);
    void set_dpad(pad_dpad d, bool down);
    // axis: 0 = 左右(右が正), 1 = 上下(下が正)。値は SDL の -32768..32767。
    void set_stick(int axis, int value);
    // which: 0 = LT, 1 = RT。値は 0..32767。
    void set_trigger(int which, int value);
    // トリガの押し下げ状態だけを今の値に合わせる(押したことにはしない)。
    // 焦点が戻った・挿した直後に、握ったままのトリガを押しと数えないために使う。
    void sync_trigger(int which, int value);
    // 右スティック。axis: 0 = 左右(右が正), 1 = 上下(下が正)。
    void set_cursor_stick(int axis, int value);
    /**
     * 右スティックの倒し方から、前回からのカーソルの移動量を出す。端数は持ち越す。
     * @return 動かす量が 0 でなければ true。
     */
    bool cursor_motion(uint32_t now, bool focused, int &dx, int &dy);

    /**
     * 方向の状態を進める。出すべき方向があれば出力の列に積む。
     * @param now          ミリ秒の時刻(SDL_GetTicks)。
     * @param focused      窓が入力の焦点を持っているか(約束 1)。
     * @param repeat_delay 押し続けたとき 2 回目までの待ち(ミリ秒)。0 以下ならリピートしない。
     */
    void update(uint32_t now, bool focused, int repeat_delay);
    // 次に update() を呼ぶべきまでのミリ秒。待つものが無ければ -1。
    int ms_until_due(uint32_t now) const;

    bool has_output() const { return !m_outputs.empty(); }
    bool take_output(pad_output &out);

    // 押していた状態を全部忘れる(焦点を失った・抜き差しした)。
    void forget();

private:
    void current_direction(int &dx, int &dy) const;
    void emit_direction(int dx, int dy, uint32_t now, int repeat_delay);
    void queue_direction(int dx, int dy);
    void press(pad_button b);
    void queue_press(pad_button b, int mods);

    bool m_dpad[NUM_PAD_DPAD] = {};
    int m_stick_x = 0, m_stick_y = 0;    // -1/0/1 に丸めたもの
    bool m_trigger_down[2] = {};

    bool cursor_active() const;
    int m_cursor_x = 0, m_cursor_y = 0;  // 右スティックの生の値
    bool m_cursor_moving = false;        // 前回 cursor_motion() で動いていたか
    uint32_t m_cursor_last = 0;          // 前回 cursor_motion() の時刻
    double m_cursor_frac_x = 0, m_cursor_frac_y = 0; // 1 に満たない端数

    int m_held_dx = 0, m_held_dy = 0;    // 最後に出した(押し続けている)向き
    uint32_t m_next_repeat = 0;
    bool m_repeat = true;                // 最後の update() でリピートが有効だったか
    bool m_pending = false;              // 中立から 1 方向が入り、斜めを待っている
    uint32_t m_pending_since = 0;
    int m_pending_dx = 0, m_pending_dy = 0;
    bool m_rearm_pending = true;         // 一度中立を見るまで受け付けない(約束 2)

    // 同時押し(変愚の PadChord と同じ約束)。添字 0 = LB、1 = RB。
    //  1. 修飾を押した瞬間には何も起きない。離したときに、その間に他のボタンを
    //     押していなければ単独押しとして出す。
    //  2. 修飾を押している間の他のボタンは同時押しになり、修飾は「使われた」印が
    //     付く(離しても単独押しにならない)。LB と RB を重ねたときも両方に付く。
    bool m_mod_held[2] = {};
    bool m_mod_used[2] = {};

    std::deque<pad_output> m_outputs;
};
