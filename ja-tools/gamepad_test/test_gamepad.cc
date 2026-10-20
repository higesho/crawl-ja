// ゲームパッドの状態機械(crawl-ref/source/gamepad.cc)の単体検査。
// SDL もゲームも要らない。組み方と走らせ方は run.py を参照。
// 設計は ja-tools/GAMEPAD_DESIGN.md。

#include "gamepad.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static int failures = 0;
static int checks = 0;

#define CHECK(cond, what)                                              \
    do {                                                               \
        ++checks;                                                      \
        if (!(cond))                                                   \
        {                                                              \
            ++failures;                                                \
            std::printf("FAIL %s (%s:%d)\n", what, __FILE__, __LINE__); \
        }                                                              \
    } while (0)

// 出力を全部取り出して "dir 1,0" / "X" の列にする。
static std::vector<std::string> drain(gamepad_state &pad)
{
    std::vector<std::string> out;
    pad_output o;
    while (pad.take_output(o))
    {
        char buf[32];
        if (o.is_direction)
            std::snprintf(buf, sizeof buf, "dir %d,%d", o.dx, o.dy);
        else
            std::snprintf(buf, sizeof buf, "%s",
                          pad_slot_name(o.button, o.mods).c_str());
        out.push_back(buf);
    }
    return out;
}

static std::string join(const std::vector<std::string> &v)
{
    std::string s;
    for (const std::string &x : v)
    {
        if (!s.empty())
            s += " | ";
        s += x;
    }
    return s;
}

#define EXPECT_OUT(pad, expected, what)                                 \
    do {                                                                \
        const std::string got = join(drain(pad));                       \
        ++checks;                                                       \
        if (got != (expected))                                          \
        {                                                               \
            ++failures;                                                 \
            std::printf("FAIL %s: got [%s] expected [%s] (%s:%d)\n",    \
                        what, got.c_str(), std::string(expected).c_str(), \
                        __FILE__, __LINE__);                            \
        }                                                               \
    } while (0)

static const int DELAY = 200; // tile_key_repeat_delay の既定

// 中立を一度見せて、受け付ける状態にした状態機械を作る。
static gamepad_state armed(uint32_t now = 0)
{
    gamepad_state pad;
    pad.update(now, true, DELAY);
    return pad;
}

static void test_names()
{
    pad_button b;
    CHECK(parse_pad_button("lb", b) && b == PAD_LB, "lb を読む");
    CHECK(parse_pad_button("Select", b) && b == PAD_BACK, "Select は Back の別名");
    CHECK(parse_pad_button("RS", b) && b == PAD_R3, "RS は R3 の別名");
    CHECK(parse_pad_button("start", b) && b == PAD_START, "start を読む");
    CHECK(!parse_pad_button("foo", b), "知らない名前は読めない");
    CHECK(!parse_pad_button("", b), "空は読めない");
    for (int i = 0; i < NUM_PAD_BUTTONS; ++i)
    {
        const pad_button bi = static_cast<pad_button>(i);
        CHECK(parse_pad_button(pad_button_name(bi), b) && b == bi,
              "綴りの往復");
    }
    CHECK(pad_default_command_name(PAD_A) == nullptr, "A は固定");
    CHECK(pad_default_command_name(PAD_B) == nullptr, "B は固定");
    CHECK(pad_default_command_name(PAD_L3) == nullptr, "L3 は固定(左クリック)");
    CHECK(pad_default_command_name(PAD_R3) == nullptr, "R3 は固定(右クリック)");
    CHECK(pad_slot_fixed(PAD_A, 0) && pad_slot_fixed(PAD_B, 0)
          && pad_slot_fixed(PAD_L3, 0) && pad_slot_fixed(PAD_R3, 0),
          "単独押しの A・B・L3・R3 は固定");
    CHECK(!pad_slot_fixed(PAD_X, 0) && !pad_slot_fixed(PAD_LB, 0),
          "ほかの単独押しは割り当てられる");
    CHECK(!pad_slot_fixed(PAD_L3, PAD_MOD_LB)
          && !pad_slot_fixed(PAD_R3, PAD_MOD_RB)
          && !pad_slot_fixed(PAD_A, PAD_MOD_LB | PAD_MOD_RB),
          "同時押しの層の A・B・L3・R3 は割り当てられる");
    CHECK(!std::strcmp(pad_default_command_name(PAD_X), "CMD_EXPLORE"),
          "X の既定は自動探索");
    CHECK(!std::strcmp(pad_default_command_name(PAD_LB), "CMD_GO_DOWNSTAIRS"),
          "LB の既定は下り階段");
    CHECK(!std::strcmp(pad_default_command_name(PAD_RB), "CMD_GO_UPSTAIRS"),
          "RB の既定は上り階段");
}

static void test_rearm()
{
    // 挿した直後(作った直後)に握ったままなら、中立を見るまで出さない(約束 2)。
    gamepad_state pad;
    pad.set_dpad(PAD_DPAD_RIGHT, true);
    pad.update(0, true, DELAY);
    pad.update(500, true, DELAY);
    EXPECT_OUT(pad, "", "中立を見る前は出さない");
    pad.set_dpad(PAD_DPAD_RIGHT, false);
    pad.update(600, true, DELAY);
    pad.set_dpad(PAD_DPAD_RIGHT, true);
    pad.update(610, true, DELAY);
    pad.update(650, true, DELAY);
    EXPECT_OUT(pad, "dir 1,0", "中立を見たあとは出す");
}

static void test_focus()
{
    gamepad_state pad = armed();
    pad.set_dpad(PAD_DPAD_UP, true);
    pad.update(10, true, DELAY);
    pad.update(60, true, DELAY);
    EXPECT_OUT(pad, "dir 0,-1", "焦点ありで上");
    // 焦点を失った(約束 1)。押したままでも出さない。
    pad.update(100, false, DELAY);
    pad.update(400, false, DELAY);
    EXPECT_OUT(pad, "", "焦点が無いと出さない");
    // 焦点が戻った。忘れているので、新しく押し直すまで出ない。
    pad.set_dpad(PAD_DPAD_UP, true); // 戻ったあとに届いた出来事
    pad.update(500, true, DELAY);
    pad.update(900, true, DELAY);
    EXPECT_OUT(pad, "", "焦点が戻った直後は中立を見るまで出さない");
    // 押していたボタンの出力も焦点を失うと捨てる。
    pad.set_button(PAD_X, true);
    pad.update(1000, false, DELAY);
    EXPECT_OUT(pad, "", "焦点を失うと溜まった押しも捨てる");
}

static void test_combine()
{
    // 十字の斜め: 上が先、右が 20ms 遅れて届く → 斜めだけを 1 回出す。
    gamepad_state pad = armed();
    pad.set_dpad(PAD_DPAD_UP, true);
    pad.update(100, true, DELAY);
    CHECK(pad.ms_until_due(100) == gamepad_state::COMBINE_MS, "合成の猶予を待つ");
    pad.set_dpad(PAD_DPAD_RIGHT, true);
    pad.update(120, true, DELAY);
    EXPECT_OUT(pad, "dir 1,-1", "斜めの合成");

    // 1 方向だけなら猶予(40ms)のあとで出す。
    gamepad_state pad2 = armed();
    pad2.set_dpad(PAD_DPAD_LEFT, true);
    pad2.update(100, true, DELAY);
    pad2.update(130, true, DELAY);
    EXPECT_OUT(pad2, "", "猶予の間は出さない");
    pad2.update(140, true, DELAY);
    EXPECT_OUT(pad2, "dir -1,0", "猶予のあとで 1 方向");

    // 同時に届いた斜めはすぐ出す。
    gamepad_state pad3 = armed();
    pad3.set_dpad(PAD_DPAD_DOWN, true);
    pad3.set_dpad(PAD_DPAD_LEFT, true);
    pad3.update(100, true, DELAY);
    EXPECT_OUT(pad3, "dir -1,1", "同時の斜めはすぐ");

    // 短く叩いた(猶予の間に離した)ら取りこぼさず 1 回出す。
    gamepad_state pad4 = armed();
    pad4.set_dpad(PAD_DPAD_DOWN, true);
    pad4.update(100, true, DELAY);
    pad4.set_dpad(PAD_DPAD_DOWN, false);
    pad4.update(110, true, DELAY);
    EXPECT_OUT(pad4, "dir 0,1", "短い叩きを取りこぼさない");
    pad4.update(500, true, DELAY);
    EXPECT_OUT(pad4, "", "叩きは 1 回だけ");
}

static void test_repeat()
{
    gamepad_state pad = armed();
    pad.set_dpad(PAD_DPAD_RIGHT, true);
    pad.update(0, true, DELAY);
    pad.update(40, true, DELAY);
    EXPECT_OUT(pad, "dir 1,0", "1 回目");
    CHECK(pad.ms_until_due(40) == DELAY, "2 回目まで tile_key_repeat_delay");
    pad.update(239, true, DELAY);
    EXPECT_OUT(pad, "", "待ちの間は出さない");
    pad.update(240, true, DELAY);
    EXPECT_OUT(pad, "dir 1,0", "2 回目");
    CHECK(pad.ms_until_due(240) == gamepad_state::REPEAT_MS, "以降は 90ms");
    pad.update(329, true, DELAY);
    EXPECT_OUT(pad, "", "90ms たつまで出さない");
    pad.update(330, true, DELAY);
    EXPECT_OUT(pad, "dir 1,0", "3 回目");
    pad.set_dpad(PAD_DPAD_RIGHT, false);
    pad.update(340, true, DELAY);
    CHECK(pad.ms_until_due(340) == -1, "離したら待つものは無い");

    // tile_key_repeat_delay = 0 ならリピートしない。
    gamepad_state pad2;
    pad2.update(0, true, 0);
    pad2.set_dpad(PAD_DPAD_UP, true);
    pad2.update(0, true, 0);
    pad2.update(40, true, 0);
    EXPECT_OUT(pad2, "dir 0,-1", "リピート無しでも 1 回は出す");
    CHECK(pad2.ms_until_due(40) == -1, "リピート無しでは起きなくてよい");
    pad2.update(5000, true, 0);
    EXPECT_OUT(pad2, "", "リピート無し");

    // 受け取られる前に何度 update されても、方向は 1 つしか溜めない。
    gamepad_state pad3 = armed();
    pad3.set_dpad(PAD_DPAD_UP, true);
    pad3.update(0, true, DELAY);
    for (uint32_t t = 40; t < 3000; t += 10)
        pad3.update(t, true, DELAY);
    EXPECT_OUT(pad3, "dir 0,-1", "方向は溜めない");
}

static void test_release_diagonal()
{
    // 斜めを押し続けていて、片方だけ先に離した → その一瞬を 1 歩にしない。
    gamepad_state pad = armed();
    pad.set_dpad(PAD_DPAD_UP, true);
    pad.set_dpad(PAD_DPAD_RIGHT, true);
    pad.update(0, true, DELAY);
    EXPECT_OUT(pad, "dir 1,-1", "斜め");
    pad.set_dpad(PAD_DPAD_RIGHT, false);
    pad.update(100, true, DELAY);
    EXPECT_OUT(pad, "", "片方を離した瞬間は出さない");
    pad.set_dpad(PAD_DPAD_UP, false);
    pad.update(120, true, DELAY);
    EXPECT_OUT(pad, "", "両方離した");

    // 片方を離して残りを押し続ければ、リピートの待ちのあとで出る。
    gamepad_state pad2 = armed();
    pad2.set_dpad(PAD_DPAD_UP, true);
    pad2.set_dpad(PAD_DPAD_RIGHT, true);
    pad2.update(0, true, DELAY);
    drain(pad2);
    pad2.set_dpad(PAD_DPAD_RIGHT, false);
    pad2.update(100, true, DELAY);
    pad2.update(299, true, DELAY);
    EXPECT_OUT(pad2, "", "待ちの間");
    pad2.update(300, true, DELAY);
    EXPECT_OUT(pad2, "dir 0,-1", "押し続けた残りの向き");

    // 1 方向から斜めへ足した・曲がったときはすぐ出す(約束 4 で待ちは数え直す)。
    gamepad_state pad3 = armed();
    pad3.set_dpad(PAD_DPAD_RIGHT, true);
    pad3.update(0, true, DELAY);
    pad3.update(40, true, DELAY);
    drain(pad3);
    pad3.set_dpad(PAD_DPAD_DOWN, true);
    pad3.update(100, true, DELAY);
    EXPECT_OUT(pad3, "dir 1,1", "斜めへ足した");
    CHECK(pad3.ms_until_due(100) == DELAY, "待ちを数え直す");
}

static void test_stick()
{
    gamepad_state pad = armed();
    pad.set_stick(0, 17000);
    pad.update(0, true, DELAY);
    pad.update(100, true, DELAY);
    EXPECT_OUT(pad, "", "倒すしきい値の手前");
    pad.set_stick(0, 20000);
    pad.update(100, true, DELAY);
    pad.update(140, true, DELAY);
    EXPECT_OUT(pad, "dir 1,0", "倒した");
    pad.set_stick(0, 15000); // 戻すしきい値より上 → 倒したまま
    pad.update(340, true, DELAY);
    EXPECT_OUT(pad, "dir 1,0", "ヒステリシス: 戻しきらないうちは倒したまま");
    pad.set_stick(0, 10000);
    pad.update(400, true, DELAY);
    pad.set_stick(0, 15000); // 戻したあとは倒すしきい値まで 0
    pad.update(500, true, DELAY);
    pad.update(900, true, DELAY);
    EXPECT_OUT(pad, "", "戻したあとは倒すしきい値まで中立");
    // スティックの斜め(上と左)。
    pad.set_stick(0, -30000);
    pad.set_stick(1, -30000);
    pad.update(1000, true, DELAY);
    EXPECT_OUT(pad, "dir -1,-1", "スティックの斜め");
    // 十字が入っていれば十字を優先。
    pad.set_dpad(PAD_DPAD_DOWN, true);
    pad.update(1100, true, DELAY);
    EXPECT_OUT(pad, "dir 0,1", "十字を優先");
}

static void test_trigger_and_buttons()
{
    gamepad_state pad = armed();
    pad.set_trigger(0, 20000);
    EXPECT_OUT(pad, "LT", "トリガの立ち上がり");
    pad.set_trigger(0, 30000);
    pad.set_trigger(0, 12000); // 戻すしきい値より上 → 押したまま
    pad.set_trigger(0, 25000);
    EXPECT_OUT(pad, "", "押しっぱなしで連射しない");
    pad.set_trigger(0, 5000);
    pad.set_trigger(0, 17000);
    EXPECT_OUT(pad, "LT", "離してからもう一度");
    // 握ったまま焦点が戻った: sync_trigger で押したことにせず合わせる。
    pad.forget();
    pad.sync_trigger(1, 32000);
    pad.set_trigger(1, 32767);
    EXPECT_OUT(pad, "", "握ったままのトリガは押しにしない");
    pad.set_trigger(1, 0);
    pad.set_trigger(1, 32767);
    EXPECT_OUT(pad, "RT", "離して押せば押し");

    // ボタンは押した瞬間。離しは使わない。方向との順番は保つ。
    gamepad_state pad2 = armed();
    pad2.set_button(PAD_X, true);
    pad2.set_button(PAD_X, false);
    pad2.set_dpad(PAD_DPAD_UP, true);
    pad2.set_dpad(PAD_DPAD_RIGHT, true);
    pad2.update(0, true, DELAY);
    pad2.set_button(PAD_A, true);
    EXPECT_OUT(pad2, "X | dir 1,-1 | A", "押しと方向の順番");
    CHECK(!pad2.has_output(), "取り出したら空");
}

static void test_slots()
{
    pad_button b;
    int mods;
    CHECK(parse_pad_slot("X", b, mods) && b == PAD_X && mods == 0, "単独の枠");
    CHECK(parse_pad_slot("lb+x", b, mods) && b == PAD_X && mods == PAD_MOD_LB,
          "LB+X");
    CHECK(parse_pad_slot("RB+Start", b, mods) && b == PAD_START
          && mods == PAD_MOD_RB, "RB+Start");
    CHECK(parse_pad_slot("RB+LB+Y", b, mods) && b == PAD_Y
          && mods == (PAD_MOD_LB | PAD_MOD_RB), "RB+LB+Y は LB+RB+Y");
    CHECK(parse_pad_slot("LB+A", b, mods) && b == PAD_A, "層の中の A は枠");
    CHECK(!parse_pad_slot("LB+LB", b, mods), "LB+LB は枠でない");
    CHECK(!parse_pad_slot("LB+RB", b, mods), "LB+RB は枠でない");
    CHECK(!parse_pad_slot("X+Y", b, mods), "修飾は LB と RB だけ");
    CHECK(!parse_pad_slot("LB+LB+X", b, mods), "修飾を重ねて書くのは誤り");
    CHECK(!parse_pad_slot("LB+", b, mods), "ボタンが無い");
    CHECK(parse_pad_slot("LB", b, mods) && b == PAD_LB && mods == 0,
          "単独の LB は枠");
    CHECK(pad_slot_name(PAD_Y, PAD_MOD_LB | PAD_MOD_RB) == "LB+RB+Y", "綴り");
    // 枠の番号と綴りの往復。番号は重ならない。
    int valid = 0;
    for (int slot = 0; slot < NUM_PAD_SLOTS; ++slot)
    {
        const std::string name = pad_slot_name(slot);
        if (name.empty())
            continue;
        ++valid;
        CHECK(parse_pad_slot(name, b, mods) && pad_slot(b, mods) == slot,
              "枠の番号と綴りの往復");
    }
    CHECK(valid == NUM_PAD_BUTTONS + 3 * (NUM_PAD_BUTTONS - 2), "枠の数");
}

static void test_chords()
{
    // LB は押した瞬間には何も出さず、離したときに単独押しとして出す。
    gamepad_state pad = armed();
    pad.set_button(PAD_LB, true);
    EXPECT_OUT(pad, "", "LB を押した瞬間は出さない");
    pad.set_button(PAD_LB, false);
    EXPECT_OUT(pad, "LB", "LB を離したら単独押し");

    // LB を押しながら X → LB+X。LB を離しても単独押しにならない。
    pad.set_button(PAD_LB, true);
    pad.set_button(PAD_X, true);
    pad.set_button(PAD_X, false);
    pad.set_button(PAD_LB, false);
    EXPECT_OUT(pad, "LB+X", "同時押し");

    // 同じ層で 2 つ押せば 2 つとも同時押し。
    pad.set_button(PAD_RB, true);
    pad.set_button(PAD_Y, true);
    pad.set_trigger(0, 30000);
    pad.set_button(PAD_RB, false);
    EXPECT_OUT(pad, "RB+Y | RB+LT", "RB の層で 2 つ");
    pad.set_trigger(0, 0);

    // LB と RB を重ねると LB+RB+ の層。どちらも単独押しにならない。
    pad.set_button(PAD_LB, true);
    pad.set_button(PAD_RB, true);
    pad.set_button(PAD_START, true);
    pad.set_button(PAD_RB, false);
    pad.set_button(PAD_LB, false);
    EXPECT_OUT(pad, "LB+RB+Start", "2 段の層");

    // 層を作っただけで離した → 何も出さない。
    pad.set_button(PAD_RB, true);
    pad.set_button(PAD_LB, true);
    pad.set_button(PAD_LB, false);
    pad.set_button(PAD_RB, false);
    EXPECT_OUT(pad, "", "層を作って離しただけ");

    // 層のあとの単独押しは元どおり(印が残らない)。
    pad.set_button(PAD_RB, true);
    pad.set_button(PAD_RB, false);
    EXPECT_OUT(pad, "RB", "印は離したら消える");

    // 修飾を握ったまま焦点を失ったら忘れる(戻ってから押した X は単独)。
    pad.set_button(PAD_LB, true);
    pad.update(0, false, DELAY);
    pad.set_button(PAD_X, true);
    pad.set_button(PAD_LB, false);
    EXPECT_OUT(pad, "X", "焦点を失うと修飾も忘れる");
}

static void test_cursor()
{
    gamepad_state pad;
    int dx, dy;

    // 中立・死区の中では動かず、起きる予定も無い。
    CHECK(!pad.cursor_motion(0, true, dx, dy), "中立では動かない");
    CHECK(pad.ms_until_due(0) == -1, "中立では起きる予定が無い");
    pad.set_cursor_stick(0, 5000);
    pad.set_cursor_stick(1, -5000);
    CHECK(!pad.cursor_motion(10, true, dx, dy), "死区の中では動かない");
    CHECK(pad.ms_until_due(10) == -1, "死区の中では起きる予定が無い");

    // 右へ倒し切る。倒し始めは時刻を覚えるだけで、刻みごとに起きる。
    pad.set_cursor_stick(0, 32767);
    pad.set_cursor_stick(1, 0);
    CHECK(!pad.cursor_motion(100, true, dx, dy), "倒し始めはまだ動かない");
    CHECK(pad.ms_until_due(100) == gamepad_state::CURSOR_TICK_MS,
          "倒している間は刻みごとに起きる");
    CHECK(pad.ms_until_due(110) == gamepad_state::CURSOR_TICK_MS - 10,
          "次の刻みまでの残り");
    CHECK(pad.cursor_motion(1100, true, dx, dy)
          && dx == gamepad_state::CURSOR_MAX_SPEED / 10 && dy == 0,
          "長く待っても 100ms 分しか進まない");
    CHECK(pad.cursor_motion(1116, true, dx, dy) && dx > 0 && dy == 0,
          "倒し切ると右へ進む");
    const int full = dx;

    // 浅く倒すとゆっくり(端数は持ち越すので、何刻みかで合計が出る)。
    pad.set_cursor_stick(0, 0);
    CHECK(!pad.cursor_motion(1132, true, dx, dy), "戻したら止まる");
    pad.set_cursor_stick(1, -12000);
    pad.cursor_motion(2000, true, dx, dy);
    int total = 0;
    for (uint32_t t = 2016; t <= 3000; t += 16)
    {
        pad.cursor_motion(t, true, dx, dy);
        CHECK(dx == 0 && dy <= 0, "上へだけ動く");
        total += dy;
    }
    CHECK(total < 0 && -total < full * 1000 / 16 / 4,
          "浅く倒すと 1 秒で少しだけ上へ進む");

    // 焦点が無ければ動かず、forget() で倒し方も忘れる。
    pad.set_cursor_stick(1, -32767);
    CHECK(!pad.cursor_motion(4000, false, dx, dy), "焦点が無ければ動かない");
    pad.update(4000, false, DELAY);
    CHECK(pad.ms_until_due(4000) == -1, "焦点を失うと倒し方も忘れる");
    CHECK(!pad.cursor_motion(4016, true, dx, dy)
          && !pad.cursor_motion(4032, true, dx, dy),
          "忘れたあとは次にスティックが動くまで止まっている");

    // 方向のリピートと重なったときは、早い方に起きる。
    gamepad_state both;
    both.set_dpad(PAD_DPAD_RIGHT, false);
    both.update(0, true, DELAY); // 中立を見せる
    both.set_dpad(PAD_DPAD_RIGHT, true);
    both.set_dpad(PAD_DPAD_UP, true);
    both.update(0, true, DELAY);
    drain(both);
    both.set_cursor_stick(0, 32767);
    both.cursor_motion(0, true, dx, dy);
    CHECK(both.ms_until_due(0) == gamepad_state::CURSOR_TICK_MS,
          "リピートの待ちより刻みが早い");
}

int main()
{
    test_names();
    test_rearm();
    test_focus();
    test_combine();
    test_repeat();
    test_release_diagonal();
    test_stick();
    test_trigger_and_buttons();
    test_slots();
    test_chords();
    test_cursor();
    std::printf("%s: %d checks, %d failures\n",
                failures ? "FAIL" : "PASS", checks, failures);
    return failures ? 1 : 0;
}
