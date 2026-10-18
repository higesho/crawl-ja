/**
 * @file
 * @brief Wizard mode command handling.
**/
#include "AppHdr.h"
#include "options.h"

#ifdef WIZARD

#include "wizard.h"

#include "abyss.h" // banished
#include "acquire.h"
#include "cio.h" // cursor_control
#include "clua.h"
#include "command.h" // show_keyhelp_menu
#include "database.h"
#include "dbg-util.h"
#include "dgn-shoals.h" // wizard_mod_tide
#include "files.h" // save_game
#include "god-companions.h" // wizard_list_companions
#include "god-passive.h" // jiyva_eat_offlevel_items
#include "hiscores.h"
#include "items.h"
#include "luaterp.h" // debug_terp_lua
#include "macro.h"
#include "menu.h" // column_composer
#include "message.h"
#include "notes.h"
#include "output.h"
#include "player.h"
#include "prompt.h" // yes_or_no
#include "religion.h" // religion_turn_end
#include "skills.h"
#include "spl-transloc.h" // wizard_blink
#include "stairs.h" // down_stairs
#include "state.h"
#include "traps.h" // do_trap_effects
#include "wizard-option-type.h"
#include "wiz-dgn.h"
#include "wiz-dump.h"
#include "wiz-fsim.h"
#include "wiz-item.h"
#include "wiz-mon.h"
#include "wiz-you.h"
#include "xom.h" // debug_xom_effects

static void _do_wizard_command(int wiz_command)
{
    ASSERT(you.wizard);

    switch (wiz_command)
    {
    case '?':
    {
        const int key = list_wizard_commands(true);
        _do_wizard_command(key);
        return;
    }

    case 'a': acquirement_menu(); break;
    case 'A': wizard_set_all_skills(); break;
    case CONTROL('A'):
        if (player_in_branch(BRANCH_ABYSS))
            wizard_set_abyss();
        else
            mpr(jtrans("You can only abyss_teleport() inside the Abyss."));
        break;

    case 'b': wizard_blink(); break;
    case 'B': you.teleport(true, true); break;
    case CONTROL('B'):
        if (!player_in_branch(BRANCH_ABYSS))
            banished("wizard command");
        else
            down_stairs(DNGN_EXIT_ABYSS);
        break;

    case 'c': wizard_draw_card(); break;
    case CONTROL('C'): die("Intentional crash");

    case 'd': wizard_level_travel(true); break;
    case 'D': wizard_detect_creatures(); break;
    case CONTROL('D'): wizard_edit_durations(); break;

    case 'e': do_trap_effects(); break;
    case 'E': wizard_freeze_time(); break;
    case CONTROL('E'): debug_dump_levgen(); break;

    case 'f': wizard_quick_fsim(); break;
    case 'F': wizard_fight_sim(false); break;
    case CONTROL('F'): wizard_fight_sim(true); break;

    case 'g': wizard_exercise_skill(); break;
    case 'G': wizard_dismiss_all_monsters(); break;
#ifdef DEBUG_BONES
    case CONTROL('G'): debug_ghosts(); break;
#endif

    case 'h': wizard_heal(false); break;
    case 'H': wizard_heal(true); break;
    // case CONTROL('H'): break;

    case 'i': wizard_identify_pack(); break;
    case 'I': wizard_unidentify_pack(); break;
    case CONTROL('I'): debug_item_statistics(); break;

    // case 'j': break;
    case 'J':
        mpr(jtrans("Running Jiyva off-level sacrifice."));
        jiyva_eat_offlevel_items();
        break;
    // case CONTROL('J'): break;

    case 'k': wizard_set_xl(true); break;
    case 'K': wizard_clear_used_vaults(); break;

    case 'l': wizard_set_xl(); break;
    case 'L': debug_place_map(false); break;
    case CONTROL('L'): debug_show_builder_logs(); break;

    case 'M':
    case 'm': wizard_create_spec_monster_name(); break;
    // case CONTROL('M'): break; // XXX do not use, menu command

    case 'n': wizard_set_zot_clock(); break;
    case 'N': wizard_get_god_tension(); break;
    // case CONTROL('N'): break;

    case 'o': wizard_create_spec_object(); break;
    case 'O': debug_test_explore(); break;
    // case CONTROL('O'): break;

    case 'p': wizard_transform(); break;
    case 'P': debug_place_map(true); break;
    case CONTROL('P'): wizard_list_props(); break;

    // case 'q': break;
    // case 'Q': break;
    case CONTROL('Q'): wizard_toggle_dprf(); break;

    case 'r': wizard_change_species(); break;
    case 'R':
    case CONTROL('R'): wizard_recreate_level(); break;

    case 's':
    case 'S': wizard_set_skill_level(); break;
    case CONTROL('S'): wizard_abyss_speed(); break;

    case 't': wizard_tweak_object(); break;
    case 'T': debug_make_trap(); break;
    case CONTROL('T'): debug_terp_dlua(); break;

    case 'u': wizard_level_travel(false); break;
    // case 'U': break;
    case CONTROL('U'): debug_terp_dlua(clua); break;

    case 'v': wizard_recharge_evokers(); break;
    case 'V': wizard_toggle_xray_vision(); break;
    case CONTROL('V'): wizard_value_item(); break;

    case 'w': wizard_god_mollify(); break;
    case 'W': wizard_god_wrath(); break;
    case CONTROL('W'): wizard_mod_tide(); break;

    case 'x':
        you.experience = 1 + exp_needed(1 + you.experience_level);
        level_change();
        break;
    case 'X': wizard_xom_acts(); break;
    case CONTROL('X'): debug_xom_effects(); break;

    case 'y': wizard_identify_all_items(); break;
    case 'Y': wizard_unidentify_all_items(); break;
    case CONTROL('Y'): wizard_suppress(); break;

    case 'z': wizard_cast_spec_spell(); break;
    case 'Z': wizard_unobtain_runes_and_orb(); break;
    // case CONTROL('Z'): break;

    case '!': wizard_memorise_spec_spell(); break;
    case '@': wizard_set_stats(); break;
    case '#': wizard_load_dump_file(); break;
    case '$': wizard_set_gold(); break;
    case '%': wizard_create_spec_object_by_name(); break;
    case '^': wizard_set_piety(); break;
    case '&': wizard_list_companions(); break;
    // case '*': break; // XXX do not use, this is the alternate control prefix
    case '(': wizard_create_feature(); break;
    // case ')': break;

    case '`': debug_list_vacant_keys(); break;
    case '~': wizard_interlevel_travel(); break;

    case '-': wizard_get_god_gift(); break;
    case '_': wizard_join_religion(); break;

    case '=':
        mprf(jtransc("Cost level: %d  Total experience: %d  Next cost level: %d Skill cost: %d"),
              you.skill_cost_level, you.total_experience,
              skill_cost_needed(you.skill_cost_level + 1),
              calc_skill_cost(you.skill_cost_level));
        break;
    case '+': wizard_make_object_randart(); break;

    // case '[': break;
    case '{': wizard_map_level(); break;

    case ']':
        if (!wizard_add_mutation())
            mpr(jtrans("Failure to give mutation."));
        break;

    case '}':
        if (!wizard_toggle_bane())
            mpr(jtrans("Failure to toggle bane."));
        break;

    case '\\': debug_make_shop(); break;
    case '|': wizard_create_all_artefacts(true); break;

    case CONTROL('\\'):
#ifdef USE_TILE_LOCAL
    // Control-\ generates CONTROL(xxx) on console, but LC_CONTROL(xxx) on
    // local tiles, as do all the non-letter control sequences that work on
    // console.
    case LC_CONTROL('\\'):
#endif
        wizard_create_all_artefacts(false);
        break;

    case ';': wizard_list_levels(); break;
    case ':': wizard_list_branches(); break;

    case '\'': wizard_list_items(); break;
    case '"': debug_list_monsters(); break;

    case ',': wizard_place_stairs(false); break;
    // case '<': break; // XXX do not use, menu command

    case '.': wizard_place_stairs(true); break;
    // case '>': break; // XXX do not use, menu command

    // case '/': break;

    case ' ':
    case '\r':
    case '\n':
    case ESCAPE:
        canned_msg(MSG_OK);
        break;
    default:
        formatted_mpr(formatted_string::parse_string(
                          jtrans("Not a <magenta>Wizard</magenta> Command.")));
        break;
    }
    // Force the placement of any delayed monster gifts.
    you.turn_is_over = true;
    religion_turn_end();

    you.turn_is_over = false;
}

static void _log_wizmode_entrance()
{
    scorefile_entry se(INSTANT_DEATH, MID_NOBODY, KILLED_BY_WIZMODE, nullptr);
    logfile_new_entry(se);
}

/// Prompt for and perform a wizard-mode command. If we are not already
// in wizard mode, prompt first.
void handle_wizard_command()
{
    int wiz_command;

    // WIZ_NEVER gives protection for those who have wiz compiles,
    // and don't want to risk their characters. Also, and hackishly,
    // it's used to prevent access for non-authorised users to wizard
    // builds in dgamelaunch builds unless the game is started with the
    // -wizard flag.
    if (Options.wiz_mode == WIZ_NEVER)
        return;

    if (you.suppress_wizard)
    {
        mprf(MSGCH_WARN, jtransc("Re-activating wizard mode."));
        you.wizard = true;
        you.suppress_wizard = false;
#ifdef USE_TILE_LOCAL
        tiles.layout_statcol();
#endif
        redraw_screen();
        update_screen();
        if (crawl_state.cmd_repeat_start)
        {
            crawl_state.cancel_cmd_repeat(jtrans("Can't repeat re-activating wizard "
                                                 "mode."));
        }
        return;
    }
    else if (!you.wizard)
    {
        mprf(MSGCH_WARN, jtransc("WARNING: ABOUT TO ENTER WIZARD MODE!"));

#ifndef SCORE_WIZARD_CHARACTERS
        if (!you.explore)
            mprf(MSGCH_WARN, jtransc("If you continue, your game will not be scored!"));
#endif

        if (!confirm_prompt("wiz", "%s", jtransc("Do you really want to enter wizard mode?")))
        {
            canned_msg(MSG_OK);
            return;
        }

        take_note(Note(NOTE_MESSAGE, 0, 0, jtrans("Entered wizard mode.")));

#ifndef SCORE_WIZARD_CHARACTERS
        if (!you.explore)
            _log_wizmode_entrance();
#endif

        you.wizard = true;
        save_game(false);
#ifdef USE_TILE_LOCAL
        tiles.layout_statcol();
#endif
        redraw_screen();
        update_screen();

        if (crawl_state.cmd_repeat_start)
        {
            crawl_state.cancel_cmd_repeat(jtrans("Can't repeat entering wizard "
                                                 "mode."));
            return;
        }
    }

    {
        mprf(MSGCH_PROMPT, jtrans_notrimc("Enter Wizard Command (? - help): "));
        cursor_control con(true);
        wiz_command = getchm();
        if (wiz_command == '*')
            wiz_command = CONTROL(toupper_safe(getchm()));
    }

    if (crawl_state.cmd_repeat_start)
    {
        // Easiest to list which wizard commands *can* be repeated.
        switch (wiz_command)
        {
        case 'x':
        case '$':
        case 'a':
        case 'c':
        case 'h':
        case 'H':
        case 'm':
        case 'M':
        case 'X':
        case ']':
        case '^':
        case '%':
        case 'o':
        case 'z':
        case CONTROL('Z'):
            break;

        default:
            crawl_state.cant_cmd_repeat(jtrans("You cannot repeat that "
                                               "wizard command."));
            return;
        }
    }

    _do_wizard_command(wiz_command);
}

void enter_explore_mode()
{
    // WIZ_NEVER gives protection for those who have wiz compiles,
    // and don't want to risk their characters.
    if (Options.explore_mode == WIZ_NEVER)
        return;

    if (you.wizard || you.suppress_wizard)
        handle_wizard_command();
    else if (!you.explore)
    {
        mprf(MSGCH_WARN, jtransc("WARNING: ABOUT TO ENTER EXPLORE MODE!"));
        mpr(jtrans("In explore mode, death is optional."));
        mpr(jtrans("Once you set a character to explore mode, you can't switch back."));

#ifndef SCORE_WIZARD_CHARACTERS
        mprf(MSGCH_WARN, jtransc("If you continue, your game will not be scored!"));
#endif

        if (!confirm_prompt("explore", "%s",
                            jtransc("Do you really want to enter explore mode?")))
        {
            canned_msg(MSG_OK);
            return;
        }

        take_note(Note(NOTE_MESSAGE, 0, 0, jtrans("Entered explore mode.")));

#ifndef SCORE_WIZARD_CHARACTERS
        scorefile_entry se(INSTANT_DEATH, MID_NOBODY, KILLED_BY_EXPLORING, nullptr);
        logfile_new_entry(se);
#endif

        you.explore = true;
        save_game(false);
        redraw_screen();
        update_screen();

        if (crawl_state.cmd_repeat_start)
        {
            crawl_state.cancel_cmd_repeat(jtrans("Can't repeat entering explore mode"));
            return;
        }
    }
}

// ヘルプの見出し
static string _wiz_head(const char *title)
{
    return "<yellow>" + jtrans(title) + "</yellow>\n";
}

// ヘルプの 1 行(キーの表示 + 説明)
static string _wiz_help(const char *keys, const char *desc)
{
    return string(keys) + jtrans(desc) + "\n";
}

int list_wizard_commands(bool do_redraw_screen)
{
    // 2 columns
    column_composer cols(2, 44);
    string col0;
    col0 += _wiz_head("Player stats");
    col0 += _wiz_help("<w>A</w>      ", "set all skills to level");
    col0 += _wiz_help("<w>Ctrl-D</w> ", "change enchantments/durations");
    col0 += _wiz_help("<w>g</w>      ", "exercise a skill");
    col0 += _wiz_help("<w>k</w>      ", "change experience level and skills");
    col0 += _wiz_help("<w>l</w>      ", "change experience level");
    col0 += _wiz_help("<w>Ctrl-P</w> ", "list props");
    col0 += _wiz_help("<w>r</w>      ", "change character's species");
    col0 += _wiz_help("<w>s</w>      ", "set skill to level");
    col0 += _wiz_help("<w>x</w>      ", "gain an experience level");
    col0 += _wiz_help("<w>$</w>      ", "set gold to a specified value");
    col0 += _wiz_help("<w>]</w>      ", "get a mutation");
    col0 += _wiz_help("<w>}</w>      ", "toggle a bane");
    col0 += _wiz_help("<w>_</w>      ", "gain religion");
    col0 += _wiz_help("<w>^</w>      ", "set piety to a value");
    col0 += _wiz_help("<w>@</w>      ", "set Str Int Dex");
    col0 += _wiz_help("<w>#</w>      ", "load character from a dump file");
    col0 += _wiz_help("<w>&</w>      ", "list all divine followers");
    col0 += _wiz_help("<w>=</w>      ", "show info about skill points");
    col0 += _wiz_help("<w>n</w>      ", "set Zot clock to a value");
    col0 += _wiz_help("<w>N</w>      ", "get current tension value");
    col0 += "\n";
    col0 += _wiz_head("Dungeon features");
    col0 += _wiz_help("<w>T</w>      ", "make a trap");
    col0 += _wiz_help("<w>,</w>/<w>.</w>    ", "create up/down staircase");
    col0 += _wiz_help("<w>(</w>      ", "turn cell into feature");
    col0 += _wiz_help("<w>\\</w>      ", "make a shop");
    col0 += "\n";
    col0 += _wiz_head("Builder debugging");
    col0 += _wiz_help("<w>L</w>      ", "place a vault by name");
    col0 += _wiz_help("<w>P</w>      ", "create a level based on a vault");
    col0 += _wiz_help("<w>R</w>      ", "regenerate current level");
    col0 += _wiz_help("<w>Ctrl-A</w> ", "generate new Abyss area");
    col0 += _wiz_help("<w>K</w>      ", "mark all vaults as unused");
    col0 += _wiz_help("<w>:</w>      ", "find branches and overflow temples in the dungeon");
    col0 += _wiz_help("<w>;</w>      ", "list known levels and counters");
    col0 += _wiz_help("<w>Ctrl-E</w> ", "dump level builder information");
#ifdef DEBUG
    // might be present in any save, but only generated
    // in debug builds; hide this for regular wizmode so as
    // to not confuse non-devs. The command will still work.
    col0 += _wiz_help("<w>Ctrl-L</w> ", "show builder logs for level");
#endif
    col0 += "\n";
    col0 += _wiz_head("Other level related commands");
    col0 += _wiz_help("<w>{</w>      ", "magic mapping");
    col0 += _wiz_help("<w>b</w>      ", "controlled blink");
    col0 += _wiz_help("<w>B</w>      ", "controlled teleport");
    col0 += _wiz_help("<w>~</w>      ", "go to a specific level");
    col0 += _wiz_help("<w>u</w>/<w>d</w>    ", "shift up/down one level");
    col0 += _wiz_help("<w>e</w>      ", "trigger explore traps");
    col0 += _wiz_help("<w>Ctrl-B</w> ", "banish yourself to the Abyss");
    col0 += _wiz_help("<w>Ctrl-S</w> ", "change Abyss speed");
    col0 += _wiz_help("<w>Ctrl-W</w> ", "change Shoals' tide speed");
    cols.add_formatted(0, col0, true);

    string col1;
    col1 += _wiz_head("Other player related effects");
    col1 += _wiz_help("<w>c</w>      ", "card effect");
#ifdef DEBUG_BONES
    col1 += _wiz_help("<w>Ctrl-G</w> ", "save/load ghost (bones file)");
#endif
    col1 += _wiz_help("<w>h</w>/<w>H</w>    ", "heal yourself (super-Heal)");
    col1 += _wiz_help("<w>X</w>      ", "make Xom do something now");
    col1 += _wiz_help("<w>z</w>      ", "cast spell by number/name");
    col1 += _wiz_help("<w>!</w>      ", "memorise spell");
    col1 += _wiz_help("<w>W</w>      ", "god wrath");
    col1 += _wiz_help("<w>w</w>      ", "god mollification");
    col1 += _wiz_help("<w>p</w>      ", "polymorph into a form");
    col1 += _wiz_help("<w>V</w>      ", "toggle xray vision");
    col1 += _wiz_help("<w>E</w>      ", "(un)freeze time");
    col1 += "\n";
    col1 += _wiz_head("Monster related commands");
    col1 += _wiz_help("<w>m</w>/<w>M</w>    ", "create specified monster");
    col1 += _wiz_help("<w>D</w>      ", "detect all monsters");
    col1 += _wiz_help("<w>G</w>      ", "dismiss all monsters");
    col1 += _wiz_help("<w>\"</w>      ", "list monsters");
    col1 += "\n";
    col1 += _wiz_head("Item related commands");
    col1 += _wiz_help("<w>a</w>      ", "acquirement");
    col1 += _wiz_help("<w>i</w>/<w>I</w>    ", "identify/unidentify inventory");
    col1 += _wiz_help("<w>y</w>/<w>Y</w>    ", "id/unid item types+level items");
    col1 += _wiz_help("<w>o</w>/<w>%</w>    ", "create an object");
    col1 += _wiz_help("<w>t</w>      ", "tweak object properties");
    col1 += _wiz_help("<w>v</w>      ", "recharge all XP evokers");
    col1 += _wiz_help("<w>Ctrl-V</w> ", "show gold value of an item");
    col1 += _wiz_help("<w>-</w>      ", "get a god gift");
    col1 += _wiz_help("<w>|</w>      ", "create all unrand artefacts");
    col1 += _wiz_help("<w>Ctrl-\\</w> ", "create all unrands / fallbacks");
    col1 += _wiz_help("<w>+</w>      ", "make randart from item");
    col1 += _wiz_help("<w>'</w>      ", "list items");
    col1 += _wiz_help("<w>J</w>      ", "Jiyva off-level sacrifice");
    col1 += _wiz_help("<w>Z</w>      ", "Unobtain runes and Orb of Zot");
    col1 += "\n";
    col1 += _wiz_head("Debugging commands");
    col1 += _wiz_help("<w>f</w>      ", "quick fight simulation");
    col1 += _wiz_help("<w>F</w>      ", "single scale fsim");
    col1 += _wiz_help("<w>Ctrl-F</w> ", "double scale fsim");
    col1 += _wiz_help("<w>Ctrl-I</w> ", "item generation stats");
    col1 += _wiz_help("<w>O</w>      ", "measure exploration time");
    col1 += _wiz_help("<w>Ctrl-T</w> ", "dungeon (D)Lua interpreter");
    col1 += _wiz_help("<w>Ctrl-U</w> ", "client (C)Lua interpreter");
    col1 += _wiz_help("<w>Ctrl-X</w> ", "Xom effect stats");
#ifdef DEBUG_DIAGNOSTICS
    col1 += _wiz_help("<w>Ctrl-Q</w> ", "make some debug messages quiet");
#endif
    col1 += _wiz_help("<w>Ctrl-Y</w> ", "temporarily suppress wizmode");
    col1 += _wiz_help("<w>Ctrl-C</w> ", "force a crash");
    col1 += _wiz_help("<w>`</w>      ", "list unassigned command keys");
    col1 += "\n";
    col1 += _wiz_head("Other wizard commands");
    col1 += jtrans("(not prefixed with <w>&</w>!)") + "\n";
    col1 += _wiz_help("<w>x?</w>     ", "list targeted commands");
    col1 += _wiz_help("<w>X?</w>     ", "list map-mode commands");
    cols.add_formatted(1, col1, true);

    int key = show_keyhelp_menu(cols.formatted_lines());
    if (do_redraw_screen)
    {
        redraw_screen();
        update_screen();
    }
    return key;
}
#endif // defined(WIZARD)
