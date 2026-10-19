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
            std::snprintf(buf, sizeof buf, "%s", pad_button_name(o.button));
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
    std::printf("%s: %d checks, %d failures\n",
                failures ? "FAIL" : "PASS", checks, failures);
    return failures ? 1 : 0;
}
