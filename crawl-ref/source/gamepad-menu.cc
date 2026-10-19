/**
 * @file
 * @brief ゲームパッドの割り当て(画面・保存・引き方)。ローカルのタイル版だけ。
 *
 * 設計は ja-tools/GAMEPAD_DESIGN.md §8。
 */

#include "AppHdr.h"

#ifdef USE_TILE_LOCAL

#include "gamepad-menu.h"

#include <map>

#include "cio.h"
#include "database.h"
#include "macro.h"
#include "menu.h"
#include "message.h"
#include "options.h"
#include "output.h"
#include "prompt.h"
#include "stringutil.h"
#include "syscalls.h"
#include "unwind.h"

// 画面で変えた分(枠 → コマンド。CMD_NO_CMD = 割り当てを外した)。
// Options とは別に持つので、rc を読み直しても消えない。
static map<int, int> _screen_binds;
static bool _capturing = false;

namespace
{
    struct pad_command_info
    {
        command_type cmd;
        const char *label; // 表示名(英語。tagged_jtrans("[pad command]") で訳す)
    };

    struct pad_command_group
    {
        const char *name; // 分類名(tagged_jtrans("[pad group]") で訳す)
        vector<pad_command_info> commands;
    };

    // 割り当ての画面に並べるコマンド。メイン画面の割り当てられるコマンドは
    // 全部ここに名前付きで載せる(名前は `?` のヘルプの訳に合わせた)。
    // 載っていないものがあれば(本家でコマンドが増えたときなど)、最後の
    // 「その他のコマンド」に CMD_ の名前で並ぶ。
    const vector<pad_command_group> &pad_command_groups()
    {
        static const vector<pad_command_group> groups =
        {
            { "Movement and exploration", {
                { CMD_EXPLORE, "Auto-explore" },
                { CMD_EXPLORE_NO_REST, "Auto-explore without resting" },
                { CMD_INTERLEVEL_TRAVEL, "Travel to a level or place" },
                { CMD_GO_DOWNSTAIRS, "Go down stairs" },
                { CMD_GO_UPSTAIRS, "Go up stairs" },
                { CMD_REST, "Rest and search" },
                { CMD_WAIT, "Wait a turn" },
                { CMD_DISPLAY_MAP, "Level map" },
                { CMD_LOOK_AROUND, "Look around" },
                { CMD_OPEN_DOOR, "Open a door" },
                { CMD_CLOSE_DOOR, "Close a door" },
                { CMD_FIX_WAYPOINT, "Set a waypoint" },
            } },
            { "Combat", {
                { CMD_AUTOFIGHT, "Auto-fight" },
                { CMD_AUTOFIGHT_NOMOVE, "Auto-fight without moving" },
                { CMD_AUTOFIRE, "Fire at the nearest target" },
                { CMD_PRIMARY_ATTACK, "Primary attack" },
                { CMD_FIRE, "Fire quivered action" },
                { CMD_FIRE_ITEM_NO_QUIVER, "Fire or throw an item" },
                { CMD_QUIVER_ITEM, "Choose quivered action" },
                { CMD_CYCLE_QUIVER_FORWARD, "Next quivered action" },
                { CMD_CYCLE_QUIVER_BACKWARD, "Previous quivered action" },
                { CMD_SWAP_QUIVER_RECENT, "Swap with the last quivered action" },
                { CMD_CAST_SPELL, "Cast a spell" },
                { CMD_FORCE_CAST_SPELL, "Cast a spell even without targets" },
                { CMD_USE_ABILITY, "Use an ability" },
                { CMD_EVOKE, "Evoke an item" },
                { CMD_ZAP_WAND, "Zap a wand" },
                { CMD_SHOUT, "Shout or give orders to allies" },
            } },
            { "Items", {
                { CMD_DISPLAY_INVENTORY, "Inventory" },
                { CMD_PICKUP, "Pick up" },
                { CMD_PICKUP_QUANTITY, "Pick up a chosen number" },
                { CMD_DROP, "Drop" },
                { CMD_DROP_LAST, "Drop the last items picked up" },
                { CMD_QUAFF, "Quaff a potion" },
                { CMD_READ, "Read a scroll" },
                { CMD_WIELD_WEAPON, "Wield a weapon" },
                { CMD_WEAPON_SWAP, "Swap weapons (a and b)" },
                { CMD_EQUIP, "Equip an item" },
                { CMD_UNEQUIP, "Unequip an item" },
                { CMD_WEAR_ARMOUR, "Wear armour" },
                { CMD_REMOVE_ARMOUR, "Take off armour" },
                { CMD_WEAR_JEWELLERY, "Put on jewellery" },
                { CMD_REMOVE_JEWELLERY, "Remove jewellery" },
                { CMD_MEMORISE_SPELL, "Memorise a spell" },
                { CMD_INSCRIBE_ITEM, "Inscribe an item" },
                { CMD_ADJUST_INVENTORY, "Reassign item and spell letters" },
            } },
            { "Information", {
                { CMD_RESISTS_SCREEN, "Character overview" },
                { CMD_DISPLAY_CHARACTER_STATUS, "Character status" },
                { CMD_DISPLAY_SKILLS, "Skills" },
                { CMD_DISPLAY_SPELLS, "Spells" },
                { CMD_DISPLAY_RELIGION, "Religion" },
                { CMD_DISPLAY_MUTATIONS, "Mutations" },
                { CMD_EXPERIENCE_CHECK, "Experience" },
                { CMD_LIST_ARMOUR, "Worn armour" },
                { CMD_LIST_JEWELLERY, "Worn jewellery" },
                { CMD_LIST_GOLD, "Gold" },
                { CMD_DISPLAY_KNOWN_OBJECTS, "Item knowledge" },
                { CMD_DISPLAY_RUNES, "Runes collected" },
                { CMD_INSPECT_FLOOR, "Examine your square" },
                { CMD_FULL_VIEW, "List what is in view" },
                { CMD_SHOW_TERRAIN, "Toggle terrain-only view" },
                { CMD_REPLAY_MESSAGES, "Message history" },
                { CMD_READ_MESSAGES, "Read messages (online play)" },
                { CMD_DISPLAY_OVERMAP, "Dungeon overview" },
                { CMD_ANNOTATE_LEVEL, "Annotate a level" },
                { CMD_SEARCH_STASHES, "Search for items" },
                { CMD_LOOKUP_HELP, "Lookup info" },
                { CMD_DISPLAY_COMMANDS, "Help" },
            } },
            { "Other", {
                { CMD_GAME_MENU, "Game menu" },
                { CMD_MACRO_MENU, "Edit macros" },
                { CMD_MACRO_ADD, "Quick add macro" },
                { CMD_GAMEPAD_MENU, "Gamepad bindings" },
                { CMD_PREV_CMD_AGAIN, "Repeat the last command" },
                { CMD_REPEAT_CMD, "Repeat the next command several times" },
                { CMD_TOGGLE_AUTOPICKUP, "Toggle autopickup" },
                { CMD_TOGGLE_SOUND, "Toggle sound effects" },
                { CMD_MAKE_NOTE, "Add a note" },
                { CMD_CHARACTER_DUMP, "Dump character to file" },
                { CMD_SHOW_CHARACTER_DUMP, "Generate and view character dump" },
                { CMD_EDIT_PLAYER_TILE, "Edit player tile" },
                { CMD_ZOOM_IN, "Zoom in" },
                { CMD_ZOOM_OUT, "Zoom out" },
                { CMD_REDRAW_SCREEN, "Redraw the screen" },
                { CMD_CLEAR_MAP, "Clear the level map" },
            } },
            { "Saving, quitting and special modes", {
                { CMD_SAVE_GAME, "Save and quit" },
                { CMD_SAVE_GAME_NOW, "Save and quit without asking" },
                { CMD_SUSPEND_GAME, "Suspend the game (console only)" },
                { CMD_QUIT, "Abandon the character" },
                { CMD_WIZARD, "Wizard mode" },
                { CMD_EXPLORE_MODE, "Explore mode" },
                { CMD_LUA_CONSOLE, "Lua console" },
            } },
            { "Moves in a given direction", {
                { CMD_MOVE_LEFT, "Move left" },
                { CMD_MOVE_RIGHT, "Move right" },
                { CMD_MOVE_UP, "Move up" },
                { CMD_MOVE_DOWN, "Move down" },
                { CMD_MOVE_UP_LEFT, "Move up-left" },
                { CMD_MOVE_UP_RIGHT, "Move up-right" },
                { CMD_MOVE_DOWN_LEFT, "Move down-left" },
                { CMD_MOVE_DOWN_RIGHT, "Move down-right" },
                { CMD_RUN_LEFT, "Run left" },
                { CMD_RUN_RIGHT, "Run right" },
                { CMD_RUN_UP, "Run up" },
                { CMD_RUN_DOWN, "Run down" },
                { CMD_RUN_UP_LEFT, "Run up-left" },
                { CMD_RUN_UP_RIGHT, "Run up-right" },
                { CMD_RUN_DOWN_LEFT, "Run down-left" },
                { CMD_RUN_DOWN_RIGHT, "Run down-right" },
                { CMD_SAFE_WAIT, "Wait a turn (only when safe)" },
                { CMD_SAFE_MOVE_LEFT, "Move left (only when safe)" },
                { CMD_SAFE_MOVE_RIGHT, "Move right (only when safe)" },
                { CMD_SAFE_MOVE_UP, "Move up (only when safe)" },
                { CMD_SAFE_MOVE_DOWN, "Move down (only when safe)" },
                { CMD_SAFE_MOVE_UP_LEFT, "Move up-left (only when safe)" },
                { CMD_SAFE_MOVE_UP_RIGHT, "Move up-right (only when safe)" },
                { CMD_SAFE_MOVE_DOWN_LEFT, "Move down-left (only when safe)" },
                { CMD_SAFE_MOVE_DOWN_RIGHT, "Move down-right (only when safe)" },
                { CMD_ATTACK_LEFT, "Attack left" },
                { CMD_ATTACK_RIGHT, "Attack right" },
                { CMD_ATTACK_UP, "Attack up" },
                { CMD_ATTACK_DOWN, "Attack down" },
                { CMD_ATTACK_UP_LEFT, "Attack up-left" },
                { CMD_ATTACK_UP_RIGHT, "Attack up-right" },
                { CMD_ATTACK_DOWN_LEFT, "Attack down-left" },
                { CMD_ATTACK_DOWN_RIGHT, "Attack down-right" },
                { CMD_CLOSE_DOOR_LEFT, "Close the door to the left" },
                { CMD_CLOSE_DOOR_RIGHT, "Close the door to the right" },
                { CMD_CLOSE_DOOR_UP, "Close the door above" },
                { CMD_CLOSE_DOOR_DOWN, "Close the door below" },
                { CMD_CLOSE_DOOR_UP_LEFT, "Close the door to the up-left" },
                { CMD_CLOSE_DOOR_UP_RIGHT, "Close the door to the up-right" },
                { CMD_CLOSE_DOOR_DOWN_LEFT, "Close the door to the down-left" },
                { CMD_CLOSE_DOOR_DOWN_RIGHT, "Close the door to the down-right" },
            } },
        };
        return groups;
    }

    const char *pad_command_label(command_type cmd)
    {
        for (const pad_command_group &group : pad_command_groups())
            for (const pad_command_info &info : group.commands)
                if (info.cmd == cmd)
                    return info.label;
        return nullptr;
    }

    // 書式付きの文字列に入れるため < を逃がす。
    string escape_tags(const string &s)
    {
        return replace_all(s, "<", "<<");
    }

    // コマンドの表示: 「自動探索 (o)」。表に無いものは CMD_ の名前。
    string command_text(command_type cmd)
    {
        const char *label = pad_command_label(cmd);
        string text = label ? tagged_jtrans("[pad command]", label)
                            : command_to_name(cmd);
        const int key = command_to_key(cmd);
        if (key != '\0')
            text += " (" + keycode_to_name(key) + ")";
        return escape_tags(text);
    }

    int pad_keycode(int slot)
    {
        return CK_PAD_BASE - slot;
    }

    string binds_file()
    {
        const string macro_file = macro_file_name();
        const size_t sep = macro_file.find_last_of("/\\");
        const string dir = sep == string::npos ? ""
                                               : macro_file.substr(0, sep + 1);
        return dir + "pad_binds.txt";
    }

    void save_binds()
    {
        const string file = binds_file();
        if (_screen_binds.empty())
        {
            unlink_u(file.c_str());
            return;
        }
        FILE *f = fopen_u(file.c_str(), "w");
        if (!f)
        {
            mprf(MSGCH_ERROR, jtransc("Couldn't open %s for writing!"),
                 file.c_str());
            return;
        }
        // 書式は rc の pad_bind と同じ(そのまま rc へ写して使える)。
        fprintf(f, "# Gamepad bindings changed in the game"
                   " (Game menu > Gamepad bindings).\n"
                   "# These take precedence over pad_bind in the rc file.\n");
        for (const auto &bind : _screen_binds)
        {
            const command_type cmd = static_cast<command_type>(bind.second);
            fprintf(f, "pad_bind = %s:%s\n", pad_slot_name(bind.first).c_str(),
                    cmd == CMD_NO_CMD ? "none" : command_to_name(cmd).c_str());
        }
        fclose(f);
    }

    // rc と既定だけで決まる割り当て(画面で変えた分を見ない)。
    command_type configured_command(pad_button button, int mods)
    {
        const auto it = Options.pad_binds.find(pad_slot(button, mods));
        if (it != Options.pad_binds.end())
            return static_cast<command_type>(it->second);
        if (!Options.pad_bind_defaults || mods != 0)
            return CMD_NO_CMD;
        const char *name = pad_default_command_name(button);
        return name ? name_to_command(name) : CMD_NO_CMD;
    }

    string bind_text(command_type cmd)
    {
        // 割り当てが無ければマクロのキー。キーの名前(Pad-X)は行の頭に
        // ホットキーとして出るので、ここでは繰り返さない。
        if (cmd == CMD_NO_CMD)
            return jtrans("(no command: macro key)");
        return command_text(cmd);
    }

    // タイル版のメニューは、ホットキーを持つ行にしかカーソルの帯を描かない
    // (menu.cc の UIMenu)。そこで押されることの無いキー(パッドの枠のキーの
    // 範囲の外)をホットキーに持たせ、行の頭にはキーの名前の代わりに空白を出す。
    class pad_choice_entry : public MenuEntry
    {
    public:
        pad_choice_entry(const string &txt,
                         function<bool(const MenuEntry&)> action)
            : MenuEntry(txt, CK_PAD_LAST - 1, action)
        {
        }

        string _get_text_preface() const override
        {
            return "     ";
        }
    };

    // 2 枚目: その枠に割り当てるコマンドを選ぶ。
    class PadCommandMenu : public Menu
    {
    public:
        enum choice_type { KEEP, SET, RESET };

        PadCommandMenu(pad_button button, int mods)
            : Menu(MF_SINGLESELECT | MF_ALLOW_FORMATTING | MF_ARROWS_SELECT
                   | MF_WRAP),
              choice(KEEP), cmd(CMD_NO_CMD)
        {
            set_tag("gamepad_command");
            action_cycle = Menu::CYCLE_NONE;
            menu_action = Menu::ACT_EXECUTE;
            const int slot = pad_slot(button, mods);
            const command_type current = gamepad_bound_command(button, mods);
            // 今の割り当ては題名と黄色の行で示し、カーソルもそこから始める。
            set_title(new MenuEntry(
                make_stringf(jtransc("Command for %s (now: %s)"),
                             pad_slot_name(button, mods).c_str(),
                             bind_text(current).c_str()),
                MEL_TITLE));

            add(make_stringf(jtransc("No command (send the macro key %s)"),
                             keycode_to_name(pad_keycode(slot)).c_str()),
                SET, CMD_NO_CMD, current == CMD_NO_CMD);
            const string configured
                = bind_text(configured_command(button, mods));
            add(make_stringf(jtransc("Reset (use the rc file or the default: %s)"),
                             configured.c_str()),
                RESET, CMD_NO_CMD, false);

            vector<bool> listed(CMD_MAX_NORMAL + 1, false);
            for (const pad_command_group &group : pad_command_groups())
            {
                add_entry(new MenuEntry(tagged_jtrans("[pad group]", group.name),
                                        MEL_SUBTITLE));
                for (const pad_command_info &info : group.commands)
                {
                    if (!gamepad_command_bindable(info.cmd))
                        continue;
                    listed[info.cmd] = true;
                    add(command_text(info.cmd), SET, info.cmd,
                        current == info.cmd);
                }
            }
            // 表に載っていないコマンドがあれば(本家で増えたときなど)、
            // CMD_ の名前で最後に並べる。今は全部載っているので出ない。
            bool other_heading = false;
            for (int c = CMD_NO_CMD + 1; c <= CMD_MAX_NORMAL; ++c)
            {
                const command_type other = static_cast<command_type>(c);
                if (listed[c] || !gamepad_command_bindable(other))
                    continue;
                if (!other_heading)
                {
                    add_entry(new MenuEntry(tagged_jtrans("[pad group]",
                                                          "All other commands"),
                                            MEL_SUBTITLE));
                    other_heading = true;
                }
                add(command_text(other), SET, other, current == other);
            }
            set_more(formatted_string::parse_string(
                jtrans("[<w>Up</w>/<w>Down</w>] choose, [<w>A</w>/<w>Enter</w>] "
                       "assign, [<w>B</w>/<w>Esc</w>] go back")));
            update_menu(true);
            if (current_index >= 0)
                set_hovered(current_index);
            else
                cycle_hover();
        }

        choice_type choice;
        command_type cmd;

    private:
        void add(const string &text, choice_type what, command_type what_cmd,
                 bool current)
        {
            MenuEntry *me = new pad_choice_entry(text,
                [this, what, what_cmd](const MenuEntry &)
                {
                    choice = what;
                    cmd = what_cmd;
                    return false; // 選んだら閉じる
                });
            add_entry(me);
            if (current)
            {
                me->colour = YELLOW;
                current_index = item_count() - 1;
            }
        }

        int current_index = -1;
    };

    // 1 枚目: 枠の一覧。
    class PadBindMenu : public Menu
    {
    public:
        PadBindMenu()
            : Menu(MF_SINGLESELECT | MF_ALLOW_FORMATTING | MF_ARROWS_SELECT
                   | MF_WRAP)
        {
            set_tag("gamepad_bindings");
            action_cycle = Menu::CYCLE_NONE;
            menu_action = Menu::ACT_EXECUTE;
            set_title(new MenuEntry(jtrans("Gamepad bindings"), MEL_TITLE));
        }

        void fill_entries(int hover_slot = -1)
        {
            static const char *layer_names[NUM_PAD_LAYERS] =
            {
                "Single press",
                "Holding LB",
                "Holding RB",
                "Holding LB and RB",
            };
            clear();
            int hover_index = -1;
            for (int mods = 0; mods < NUM_PAD_LAYERS; ++mods)
            {
                add_entry(new MenuEntry(tagged_jtrans("[pad layer]",
                                                      layer_names[mods]),
                                        MEL_SUBTITLE));
                for (int b = 0; b < NUM_PAD_BUTTONS; ++b)
                {
                    const pad_button button = static_cast<pad_button>(b);
                    if (!pad_slot_valid(button, mods))
                        continue;
                    // 単独押しの A と B は決定・取消で固定(下の案内に書く)。
                    if (mods == 0 && (button == PAD_A || button == PAD_B))
                        continue;
                    const int slot = pad_slot(button, mods);
                    // ホットキーを枠のキーにする: 行の頭に Pad-X と出て、
                    // パッドのそのボタンを押すとこの行が選ばれる。
                    MenuEntry *me = new MenuEntry(
                        bind_text(gamepad_bound_command(button, mods)),
                        pad_keycode(slot),
                        [this, button, mods](const MenuEntry &)
                        {
                            edit(button, mods);
                            return true;
                        });
                    if (_screen_binds.count(slot))
                        me->colour = YELLOW; // この画面で変えた枠
                    add_entry(me);
                    if (slot == hover_slot)
                        hover_index = item_count() - 1;
                }
            }
            add_entry(new MenuEntry("", MEL_SUBTITLE));
            add_entry(new pad_choice_entry(
                jtrans("Undo all changes made in this screen"),
                [this](const MenuEntry &)
                {
                    undo_all();
                    return true;
                }));

            set_more(formatted_string::parse_string(
                status + "\n"
                + jtrans("[<w>Up</w>/<w>Down</w>] choose, [<w>A</w>/<w>Enter</w>] "
                         "change, [<w>B</w>/<w>Esc</w>] close. Press a gamepad "
                         "button or chord to change it.")
                + "\n"
                + jtrans("A (confirm) and B (cancel) alone are fixed. "
                         "<yellow>Yellow</yellow> rows were changed here: they "
                         "are saved to pad_binds.txt and take precedence over "
                         "pad_bind in the rc file.")));
            update_menu(true);
            if (hover_index >= 0)
                set_hovered(hover_index, true);
            else
                cycle_hover();
        }

    private:
        void edit(pad_button button, int mods)
        {
            const int slot = pad_slot(button, mods);
            PadCommandMenu chooser(button, mods);
            chooser.show();
            if (chooser.choice == PadCommandMenu::KEEP)
                return;
            if (chooser.choice == PadCommandMenu::RESET)
                _screen_binds.erase(slot);
            else
                _screen_binds[slot] = chooser.cmd;
            save_binds();
            const string now
                = bind_text(gamepad_bound_command(button, mods));
            status = make_stringf("%s: %s", pad_slot_name(button, mods).c_str(),
                                  now.c_str());
            fill_entries(slot);
        }

        void undo_all()
        {
            if (_screen_binds.empty())
                return;
            if (!yesno(jtransc("Really undo all changes made in this screen?"),
                       true, 'n'))
            {
                return;
            }
            _screen_binds.clear();
            save_binds();
            status = jtrans("All changes made in this screen were undone.");
            fill_entries();
        }

        string status;
    };
}

bool gamepad_command_bindable(command_type cmd)
{
    // メイン画面のコマンドだけ(GAMEPAD_DESIGN.md §2.1)。
    if (cmd <= CMD_NO_CMD || cmd > CMD_MAX_NORMAL
        || context_for_command(cmd) != KMC_DEFAULT
        || command_to_name(cmd) == "CMD_NO_CMD")
    {
        return false;
    }
    switch (cmd)
    {
    // メイン画面の範囲にあるが、内部で使うだけのもの。
    case CMD_NO_CMD_DEFAULT: // キーの割り当てを消すための目印。何もしない
    case CMD_MOUSE_MOVE:     // マウスの処理用。移動先が決まらない
    case CMD_MOUSE_CLICK:
        return false;
    default:
        return true;
    }
}

command_type gamepad_bound_command(pad_button button, int mods)
{
    const auto it = _screen_binds.find(pad_slot(button, mods));
    if (it != _screen_binds.end())
        return static_cast<command_type>(it->second);
    return configured_command(button, mods);
}

bool gamepad_menu_capturing()
{
    return _capturing;
}

void gamepad_menu()
{
    unwind_bool capture(_capturing, true);
    PadBindMenu menu;
    menu.fill_entries();
    menu.show();
    redraw_screen();
    update_screen();
}

void gamepad_load_binds()
{
    _screen_binds.clear();
    FILE *f = fopen_u(binds_file().c_str(), "r");
    if (!f)
        return;
    char buf[256];
    while (fgets(buf, sizeof buf, f))
    {
        string line = trimmed_string(buf);
        if (line.empty() || line[0] == '#')
            continue;
        // pad_bind = X:CMD_PICKUP(rc と同じ書式。読めない行は黙って飛ばす)
        const size_t eq = line.find('=');
        if (eq != string::npos)
            line = trimmed_string(line.substr(eq + 1));
        const size_t colon = line.find(':');
        if (colon == string::npos)
            continue;
        pad_button button;
        int mods;
        if (!parse_pad_slot(trimmed_string(line.substr(0, colon)), button, mods)
            || mods == 0 && (button == PAD_A || button == PAD_B))
        {
            continue;
        }
        const string name = trimmed_string(line.substr(colon + 1));
        if (lowercase_string(name) == "none")
        {
            _screen_binds[pad_slot(button, mods)] = CMD_NO_CMD;
            continue;
        }
        const command_type cmd = name_to_command(uppercase_string(name));
        if (gamepad_command_bindable(cmd))
            _screen_binds[pad_slot(button, mods)] = cmd;
    }
    fclose(f);
}

#endif // USE_TILE_LOCAL
