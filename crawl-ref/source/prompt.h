/**
 * @file
 * @brief Prompts.
 **/

#pragma once

#include "menu.h"

namespace ui
{
    int error(string err, string title="Error!", bool force_popup=false);
    int message(string msg,
        string title="", string prompt="<cyan>Hit any key to continue...</cyan>",
        bool format_msg=false);
}

bool confirm_prompt(const char* require, const char* fmt, ...);
typedef map<int, int> explicit_keymap;
int yesno(const char * str, bool allow_lowercase, int default_answer,
          bool clear_after = true, bool interrupt_delays = true,
          bool noprompt = false,
          const explicit_keymap *map = nullptr,
          bool allow_popup = true,
          bool ask_always = false,
          const char *cursor_popup = nullptr);
// cursor_popup(日本語版): メッセージ欄や more の行で訊いているときに、カーソル
// (パッドの十字)が来たら、この文を問いにしてポップアップへ切り替える。
// str があるときは str を使うので要らない(GAMEPAD_DESIGN.md §10)。

// 日本語版(ja-tools/GAMEPAD_DESIGN.md §10): カーソルを動かすキー(矢印・Home・End・
// PgUp・PgDn)か。ゲームパッドの十字はこれらを出す。
bool key_is_cursor(int key);

// 日本語版: 1 文字で答える問いの選択肢。
struct prompt_choice
{
    // C++11 でも { キー, 名前 } と書けるように、構築子を持たせる
    prompt_choice(int k, const string &l, bool a = true)
        : key(k), label(l), any_case(a)
    {
    }

    int key;        // 選んだときに返すキー(表示にも出る)
    string label;   // 表示する名前(訳した後の文)
    bool any_case;  // 英字なら、大文字小文字のどちらで打っても選べる
};

// 日本語版: メッセージ欄で 1 文字を待っていた問いを、カーソルでも選べるメニューで出す
// (prompt_menu が偽で、メッセージ欄に収まるならメッセージ欄に出す)。
// 選んだ選択肢の key を返す。取り消したら ESCAPE。
// init_hover が偽なら、カーソルを置かずに出す(Enter をうっかり押しても選ばれない)。
// grid が真なら、選択肢を格子に並べる(数が多いとき)。
int prompt_choose_key(const string &question,
                      const vector<prompt_choice> &choices,
                      int default_key = 0, bool init_hover = true,
                      bool grid = false);

// 日本語版: パッドで入力欄に入れる 1 文字を選ぶメニュー(GAMEPAD_DESIGN.md §10)。
// current は今の入力、last は前に選んだ文字(そこにカーソルを置く)。B で 0。
int pad_pick_char(const string &current, int last);

// max_qty(日本語版): 1 以上なら、上下キーで 1..max_qty の範囲で数を増減できる。
// 日本語版: ポップアップの下に並ぶ操作の行(「(r)読む、(d)置く…」)を、カーソルで選べるように
// する部品(GAMEPAD_DESIGN.md §10)。左右(パッドの十字)でカーソルを動かし、Enter(A)で
// カーソルの操作を選ぶ。選んでいる操作は黄色の [ ] で囲む。文字のキーは今までどおり呼ぶ側で受ける。
class action_bar
{
public:
    // items: { 操作のキー, 表示する名前(訳した後) }
    explicit action_bar(const vector<pair<int, string>> &items);
    bool empty() const { return m_items.empty(); }
    // 操作の行の文(prefix を頭に付ける)。色の印つき。
    string text(const string &prefix = "") const;
    // キーを受ける。0 = 受けない、1 = カーソルが動いた(描き直す)、
    // 2 = 選んだ(chosen に操作のキーを入れる)。
    int handle_key(int key, int &chosen);

private:
    vector<pair<int, string>> m_items;
    int m_cursor = 0;
};

int prompt_for_quantity(const char *prompt, int max_qty = 0);
// lo <= hi(日本語版)なら、上下キーで lo..hi の範囲で数を増減できる。
int prompt_for_int(const char *prompt, bool nonneg, const string &prefill = "",
                   int lo = 0, int hi = -1);
double prompt_for_float(const char* prompt);

char index_to_letter(int the_index);

int letter_to_index(int the_letter);


// PromptMenu is an implementation of Menu for use in the message pane. The
// main feature is that, if the menu is small enough to fit as a regular prompt,
// it displays it that way, otherwise, it uses a regular Menu popup. It is
// currently pretty minimalistic in that, when in prompt mode, a lot of the
// more involved Menu features are just not supported.
class PromptMenu : public Menu
{
public:
    PromptMenu(int _flags = MF_SINGLESELECT | MF_ARROWS_SELECT
                            | MF_INIT_HOVER | MF_GRID_LAYOUT)
        : Menu(_flags), columns(0), col_width(0), in_prompt_mode(false)
    {
        // no other mode is supported
        ASSERT(is_set(MF_SINGLESELECT));
    }

    bool process_key(int keyin) override;
    void update_menu(bool update_entries=false) override;

    virtual vector<MenuEntry *> show(bool reuse_selections = false) override;
    virtual vector<MenuEntry *> show_in_msgpane();

    void update_columns();
    void build_prompt_menu();

    bool fits_in_mpane() const;

protected:
    vector<formatted_string> menu_text;
    int columns;
    int col_width;
    bool in_prompt_mode;

};

#ifdef WIZARD
// If _symbol is 0, a copy in a WizardMenu will be given the next unused symbol.
// symbol in the menu's copy.
class WizardEntry : public MenuEntry
{
public:
    WizardEntry(int _symbol, string _text, int _output)
    : MenuEntry(_text, MEL_ITEM, 1, _symbol), output(_output)
    { indent_no_hotkeys = true; }
    WizardEntry(int _symbol, string _text) // Return the symbol from run().
    : WizardEntry(_symbol, _text, _symbol) { }
    WizardEntry(string _text, int _output) // Find a symbol later.
    : WizardEntry(0, _text, _output) { }

    int output;
};

class WizardMenu : private PromptMenu
{
public:
    WizardMenu(string _title, vector<WizardEntry> &_options, int _default=127);
    int run(bool to_bool = false);

    int result() const
    {
        return last_result;
    }
    bool success() const
    {
        return last_success;
    }
protected:
    void run_aux();
    int index_to_symbol(int index) const;
    int next_unused_symbol();
    bool skip_process_command(int keyin) override;

    unsigned last_symbol_index = 0;
    int last_result = 0;
    bool last_success = false;

    int default_value;
};
#endif
