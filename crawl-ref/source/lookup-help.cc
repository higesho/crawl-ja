/**
 * @file
 * @brief Let the player search for descriptions of monsters, items, etc.
 **/

#include "AppHdr.h"

#include "lookup-help.h"

#include <functional>

#include "ability.h"
#include "abyss.h"
#include "artefact.h"
#include "branch.h"
#include "cio.h"
#include "colour.h"
#include "cloud.h"
#include "database.h"
#include "dbg-util.h"
#include "decks.h"
#include "describe.h"
#include "describe-god.h"
#include "directn.h"
#include "english.h"
#include "enum.h"
#include "env.h"
#include "god-menu.h"
#include "item-prop.h"
#include "item-name.h"
#include "items.h"
#include "japanese.h"
#include "jobs.h"
#include "json.h"
#include "json-wrapper.h"
#include "libutil.h" // map_find
#include "macro.h"
#include "makeitem.h" // item_colour
#include "menu.h"
#include "message.h"
#include "mon-info.h"
#include "mon-pick.h"
#include "mon-tentacle.h"
#include "mutation.h"
#include "output.h"
#include "prompt.h"
#include "religion.h"
#include "rltiles/tiledef-main.h"
#ifdef USE_TILE
// 日本語版: スポイラーの書き出しでタイルの切り出し位置を引くため
#include "rltiles/tiledef-feat.h"
#include "rltiles/tiledef-floor.h"
#include "rltiles/tiledef-gui.h"
#include "rltiles/tiledef-icons.h"
#include "rltiles/tiledef-player.h"
#include "rltiles/tiledef-wall.h"
#endif
#include "scroller.h"
#include "skills.h"
#include "species.h"
#include "spl-book.h"
#include "spl-util.h"
#include "stringutil.h"
#include "syscalls.h"
#include "tag-version.h"
#include "terrain.h"
#include "tile-flags.h"
#include "tilepick.h"
#include "tileview.h"
#include "ui.h"
#include "viewchar.h"
#include "view.h"
#include "version.h"


typedef vector<string> (*keys_by_glyph)(char32_t showchar);
typedef vector<string> (*simple_key_list)();
typedef void (*db_keys_recap)(vector<string>&);
typedef MenuEntry* (*menu_entry_generator)(char letter, const string &str,
                                           string &key);
typedef function<int (const string &, const string &, string)> key_describer;

/// A set of optional functionality for lookup types.
enum class lookup_type
{
    none            = 0,
    /// append the 'type' to the db lookup (e.g. "<input> spell")
    db_suffix       = 1<<0,
    /// whether the sorting functionality should be turned off
    disable_sort    = 1<<1,
    /// whether the display menu for this has toggleable sorting
    toggleable_sort = 1<<2,
};
DEF_BITFIELD(lookup_type_flags, lookup_type);

/// A description of a lookup that the player can do. (e.g. (M)onster data)
class LookupType
{
public:
    LookupType(char _symbol, string _type, db_keys_recap _recap,
               db_find_filter _filter_forbid, keys_by_glyph _glyph_fetch,
               simple_key_list _simple_key_fetch,
               menu_entry_generator _menu_gen, key_describer _describer,
               lookup_type_flags _flags)
    : symbol(_symbol), type(_type), filter_forbid(_filter_forbid),
      flags(_flags),
      simple_key_fetch(_simple_key_fetch), glyph_fetch(_glyph_fetch),
      recap(_recap), menu_gen(_menu_gen), describer(_describer)
    {
        // XXX: will crash at startup; compile-time would be better
        // also, ugh
        ASSERT(menu_gen != nullptr || type == "monster");
        ASSERT(describer != nullptr);
    }

    string prompt_string() const;
    string suffix() const;
    vector<string> matching_keys(string regex) const;
    void display_keys(vector<string> &key_list) const;

    /**
     * Does this lookup type have special support for single-character input
     * (looking up corresponding glyphs - e.g. 'o' for orc, '(' for ammo...)
     */
    bool supports_glyph_lookup() const { return glyph_fetch != nullptr; }

    /**
     * Does this lookup type return a list of keys without taking a search
     * request (e.g. branches or gods)?
     */
    bool no_search() const { return simple_key_fetch != nullptr; }

    bool find_description(string &response) const;
    int describe(const string &key, bool exact_match = false) const;

public:
    /// The letter pressed to choose this (e.g. 'M'). case insensitive
    char symbol;
    /// A description of the lookup type (e.g. "monster"). case insensitive
    string type;
    /// a function returning 'true' if the search result corresponding to
    /// the corresponding search should be filtered out of the results
    db_find_filter filter_forbid;
    /// A set of optional functionality; see lookup_type for details
    lookup_type_flags flags;
private:
    MenuEntry* make_menu_entry(char letter, string &key) const;
    string key_to_menu_str(const string &key) const;
    vector<string> get_desc_keys(string regex) const;

    /**
     * Does this lookup type support toggling the sort order of results?
     */
    bool toggleable_sort() const
    {
        return bool(flags & lookup_type::toggleable_sort);
    }

private:
    /// Function that fetches a list of keys, without taking arguments.
    simple_key_list simple_key_fetch;
    /// a function taking a single character & returning a list of keys
    /// corresponding to that glyph
    keys_by_glyph glyph_fetch;
    /// take the list of keys that were automatically found and fix them
    /// up if necessary
    db_keys_recap recap;
    /// take a letter & a key, return a corresponding new menu entry
    menu_entry_generator menu_gen;
    /// A function to handle describing & interacting with a given key.
    key_describer describer;
};




/**
 * What monster enum corresponds to the given Serpent of Hell name?
 *
 * @param soh_name  The name of the monster; e.g. "Serpent of Hell dis".
 * @return          The corresponding enum; e.g. MONS_SERPENT_OF_HELL_DIS.
 */
static monster_type _soh_type(string &soh_name)
{
    // If no branch or no valid branch was specified, this will be "hell", in
    // which case we default to Gehenna below.
    const string flavour = lowercase_string(soh_name.substr(soh_name.find_last_of(' ')+1));

    branch_type branch = NUM_BRANCHES;
    for (int b = BRANCH_FIRST_HELL; b <= BRANCH_LAST_HELL; ++b)
        if (ends_with(flavour, lowercase_string(branches[b].shortname)))
            branch = (branch_type)b;

    switch (branch)
    {
        case BRANCH_COCYTUS:
            return MONS_SERPENT_OF_HELL_COCYTUS;
        case BRANCH_DIS:
            return MONS_SERPENT_OF_HELL_DIS;
        case BRANCH_TARTARUS:
            return MONS_SERPENT_OF_HELL_TARTARUS;
        case BRANCH_GEHENNA:
        default:
            return MONS_SERPENT_OF_HELL;
    }
}

static bool _is_soh(string name)
{
    return starts_with(lowercase(name), "serpent of hell");
}

static string _soh_name(monster_type m_type)
{
    branch_type b = serpent_of_hell_branch(m_type);
    return string("Serpent of Hell (") + branches[b].longname + ")";
}

// 表示用(日本語)。_soh_name() は英語の名前のまま残す。
static string _soh_name_j(monster_type m_type)
{
    branch_type b = serpent_of_hell_branch(m_type);
    return mons_type_name_j(m_type, DESC_PLAIN) + "("
           + branch_name_j(branches[b].longname) + ")";
}

/**
 * 一覧の表示名。検索は英語の名前で行うので「日本語名 (英語名)」と併記する。
 * 訳がない(英語のまま)ときは英語名だけにする。
 */
static string _ja_en(const string &ja, const string &en)
{
    if (ja.empty() || lowercase_string(ja) == lowercase_string(en))
        return en;
    return ja + " (" + en + ")";
}

static monster_type _mon_by_name(string name)
{
    return _is_soh(name) ? _soh_type(name) : get_monster_by_name(name);
}

static bool _compare_mon_names(MenuEntry *entry_a, MenuEntry* entry_b)
{
    monster_info* a = static_cast<monster_info* >(entry_a->data);
    monster_info* b = static_cast<monster_info* >(entry_b->data);

    if (a->type == b->type)
        return false;

    string a_name = mons_type_name(a->type, DESC_PLAIN);
    string b_name = mons_type_name(b->type, DESC_PLAIN);
    return lowercase(a_name) < lowercase(b_name);
}

// Compare monsters by location-independent level, or by hitdice if
// levels are equal, or by name if both level and hitdice are equal.
static bool _compare_mon_toughness(MenuEntry *entry_a, MenuEntry* entry_b)
{
    monster_info* a = static_cast<monster_info* >(entry_a->data);
    monster_info* b = static_cast<monster_info* >(entry_b->data);

    if (a->type == b->type)
        return false;

    int a_toughness = mons_avg_hp(a->type);
    int b_toughness = mons_avg_hp(b->type);

    if (a_toughness == b_toughness)
    {
        string a_name = mons_type_name(a->type, DESC_PLAIN);
        string b_name = mons_type_name(b->type, DESC_PLAIN);
        return lowercase(a_name) < lowercase(b_name);
    }
    return a_toughness > b_toughness;
}

namespace
{
    class DescMenu : public Menu
    {
    public:
        DescMenu(int _flags, bool _toggleable_sort) : Menu(_flags, ""),
            sort_alpha(true), toggleable_sort(_toggleable_sort)
        {
            set_highlighter(nullptr);

            if (_toggleable_sort)
                toggle_sorting();

            set_prompt();
        }

        bool sort_alpha;
        bool toggleable_sort;

        void set_prompt()
        {
            string prompt = jtrans_notrim("Describe which? ");

            if (toggleable_sort)
            {
                if (sort_alpha)
                    prompt += jtrans("(CTRL-S to sort by monster toughness)");
                else
                    prompt += jtrans("(CTRL-S to sort by name)");
            }
            set_title(new MenuEntry(prompt, MEL_TITLE));
        }

        bool skip_process_command(int keyin) override
        {
            if (keyin == '!')
                return true; // Gauntlet branch help hotkey
            return Menu::skip_process_command(keyin);
        }

        void sort()
        {
            if (!toggleable_sort)
                return;

            if (sort_alpha)
                ::sort(items.begin(), items.end(), _compare_mon_names);
            else
                ::sort(items.begin(), items.end(), _compare_mon_toughness);

            for (unsigned int i = 0, size = items.size(); i < size; i++)
            {
                const char letter = index_to_letter(i % 52);

                items[i]->hotkeys.clear();
                items[i]->add_hotkey(letter);
            }
        }

        void toggle_sorting()
        {
            if (!toggleable_sort)
                return;

            sort_alpha = !sort_alpha;

            sort();
            set_prompt();
        }
    };
}

vector<string> LookupType::get_desc_keys(string regex) const
{
    vector<string> key_matches, body_matches;

    // Search by school skips regular searching.
    if (type == "spell" && starts_with(regex, "@"))
    {
        regex.erase(0, 1);
        spschools_type school;
        text_pattern tpat(regex, true);
        for (const auto _school : spschools_type::range())
        {
            if (tpat.matches(spelltype_long_name(_school)))
                school = _school;
        }

        for (spell_type i = SPELL_NO_SPELL; i < NUM_SPELLS; ++i)
        {
            if ((get_spell_disciplines(i) & school) && is_player_book_spell(i))
            {
                string str = lowercase_string(make_stringf("%s spell", spell_title(i)));
                key_matches.push_back(str);
            }
        }
    }
    else
    {
        key_matches = getLongDescKeysByRegex(regex, filter_forbid);
        body_matches = getLongDescBodiesByRegex(regex, filter_forbid);
    }

    // Merge all types of matches, discarding duplicates.
    vector<string> tmp = key_matches;
    tmp.insert(tmp.end(), body_matches.begin(), body_matches.end());
    sort(tmp.begin(), tmp.end());
    vector<string> all_matches;
    for (unsigned int i = 0, size = tmp.size(); i < size; i++)
        if (i == 0 || all_matches[all_matches.size() - 1] != tmp[i])
            all_matches.push_back(tmp[i]);

    return all_matches;
}

static vector<string> _get_monster_keys(char32_t showchar)
{
    vector<string> mon_keys;

    for (monster_type i = MONS_0; i < NUM_MONSTERS; ++i)
    {
        if (i == MONS_PROGRAM_BUG)
            continue;

        const monsterentry *me = get_monster_data(i);

        if (me == nullptr || me->name == nullptr || me->name[0] == '\0')
            continue;

        if (me->mc != i)
            continue;

        if ((char32_t)me->basechar != showchar)
            continue;

        if (mons_species(i) == MONS_SERPENT_OF_HELL)
        {
            mon_keys.push_back(string(me->name) + " "
                               + serpent_of_hell_flavour(i));
            continue;
        }

        if (getLongDescription(me->name).empty())
            continue;

        mon_keys.push_back(me->name);
    }

    return mon_keys;
}

static vector<string> _get_card_keys()
{
    vector<string> names;
    for (int i = 0; i < NUM_CARDS; ++i)
    {
        card_type card = static_cast<card_type>(i);
        if (!card_is_removed(card))
            names.push_back(make_stringf("%s card", card_name(card)));
    }
    return names;
}

static vector<string> _get_god_keys()
{
    vector<string> names;

    for (int i = GOD_NO_GOD + 1; i < NUM_GODS; i++)
    {
        god_type which_god = static_cast<god_type>(i);
#if TAG_MAJOR_VERSION == 34
        // XXX: currently disabled.
        if (which_god != GOD_PAKELLAS)
#endif
        names.push_back(god_name(which_god));
    }

    return names;
}

static vector<string> _get_branch_keys()
{
    vector<string> names;

    for (branch_iterator it; it; ++it)
    {
        // Skip unimplemented branches
        if (branch_is_unfinished(it->id))
            continue;

        names.push_back(it->shortname);
    }
    return names;
}

static vector<string> _get_cloud_keys()
{
    vector<string> names;

    for (int i = CLOUD_NONE + 1; i < NUM_CLOUD_TYPES; i++)
    {
        const cloud_type cloud = static_cast<cloud_type>(i);
        if (!cloud_is_removed(cloud))
            names.push_back(cloud_type_name(cloud) + " cloud");
    }

    return names;
}

/**
 * Return a list of all skill names.
 */
static vector<string> _get_skill_keys()
{
    vector<string> names;
    for (skill_type sk = SK_FIRST_SKILL; sk < NUM_SKILLS; ++sk)
    {
        const string name = lowercase_string(skill_name(sk));
#if TAG_MAJOR_VERSION == 34
        if (getLongDescription(name).empty())
            continue; // obsolete skills
#endif

        names.emplace_back(name);
    }
    return names;
}

static bool _monster_filter(string key, string /*body*/)
{
    const monster_type mon_num = _mon_by_name(key);
    return mons_class_flag(mon_num, M_CANT_SPAWN)
           || mons_is_tentacle_segment(mon_num);
}

static bool _spell_filter(string key, string /*body*/)
{
    if (!strip_suffix(key, "spell"))
        return true;

    spell_type spell = spell_by_name(key);

    if (spell == SPELL_NO_SPELL)
        return true;

    if (get_spell_flags(spell) & spflag::testing)
        return !you.wizard;

    return false;
}

static bool _item_filter(string key, string /*body*/)
{
    item_kind ik = item_kind_by_name(key);
    return ik.base_type == OBJ_UNASSIGNED
        && !item_type_removed(ik.base_type, ik.sub_type)
        && !extant_unrandart_by_exact_name(key);
}

static bool _feature_filter(string key, string /*body*/)
{
    return feat_by_desc(key) == DNGN_UNSEEN;
}

static bool _ability_filter(string key, string /*body*/)
{
    lowercase(key);

    if (!strip_suffix(key, "ability"))
        return true;

    return !string_matches_ability_name(key);
}

static bool _status_filter(string key, string /*body*/)
{
    return !strip_suffix(lowercase(key), " status");
}

static bool _mutation_filter(string key, string /*body*/)
{
    lowercase(key);

    if (!strip_suffix(key, " mutation"))
        return true;

    return starts_with(key, "potion of"); // hack alert!
}

static bool _bane_filter(string key, string /*body*/)
{
    lowercase(key);

    return !strip_suffix(key, " bane");
}

static bool _passive_filter(string key, string /*body*/)
{
    return !strip_suffix(lowercase(key), " passive");
}

static void _recap_mon_keys(vector<string> &keys)
{
    for (unsigned int i = 0, size = keys.size(); i < size; i++)
    {
        if (!_is_soh(keys[i]))
        {
            monster_type type = get_monster_by_name(keys[i]);
            // No "the Royal Jelly".
            keys[i] = remove_prepended_the(mons_type_name(type, DESC_PLAIN));
        }
    }
}

static void _recap_item_keys(vector<string> &keys)
{
    for (unsigned int i = 0, size = keys.size(); i < size; i++)
    {
        const int unrand_idx = extant_unrandart_by_exact_name(keys[i]);
        if (unrand_idx)
            keys[i] = get_unrand_entry(unrand_idx)->name; // fix capitalization
    }
}

/**
 * Fixup spell names. (Correcting capitalization, mainly.)
 *
 * @param[in,out] keys      A lowercased list of spell names.
 */
static void _recap_spell_keys(vector<string> &keys)
{
    for (unsigned int i = 0, size = keys.size(); i < size; i++)
    {
        // first, strip " spell"
        const string key_name = keys[i].substr(0, keys[i].length() - 6);
        // then get the real name
        keys[i] = make_stringf("%s spell",
                               spell_title(spell_by_name(key_name)));
    }
}

/**
 * Fixup ability names. (Correcting capitalization, mainly.)
 *
 * @param[in,out] keys      A lowercased list of ability names.
 */
static void _recap_ability_keys(vector<string> &keys)
{
    for (auto &key : keys)
    {
        strip_suffix(key, "ability");
        // get the real name
        key = make_stringf("%s ability", ability_name(ability_by_name(key)).c_str());
    }
}

static void _recap_feat_keys(vector<string> &keys)
{
    for (unsigned int i = 0, size = keys.size(); i < size; i++)
    {
        dungeon_feature_type type = feat_by_desc(keys[i]);
        if (type == DNGN_ENTER_SHOP)
            keys[i] = "A shop";
        else
        {
            keys[i] = feature_description(type, NUM_TRAPS, "", DESC_A,
                                          NUM_BRANCHES);
        }
    }
}

/**
 * Make a basic, no-frills ?/<foo> menu entry.
 *
 * @param letter    The letter for the entry. (E.g. 'e' for the fifth entry.)
 * @param str       A processed string for the entry. (E.g. "Blade".)
 * @param key       The raw database key for the entry. (E.g. "blade card".)
 * @return          A new menu entry.
 */
static MenuEntry* _simple_menu_gen(char letter, const string &str, string &key)
{
    MenuEntry* me = new MenuEntry(str, MEL_ITEM, 1, letter);
    me->data = &key;
    return me;
}

/**
 * Generate a ?/M entry.
 *
 * @param letter      The letter for the entry. (E.g. 'e' for the fifth entry.)
 * @param str         A processed string for the entry. (E.g. "Blade".)
 * @param mslot[out]  A space in memory to store a fake monster.
 * @return            A new menu entry.
 */
static MenuEntry* _monster_menu_gen(char letter, const string &str,
                                    monster_info &mslot)
{
    // Create and store fake monsters, so the menu code will
    // have something valid to refer to.
    monster_type m_type = _mon_by_name(str);
    // 表示は「日本語名 (英語名)」。検索キーは英語のまま
    const string name = _is_soh(str)
        ? _ja_en(_soh_name_j(m_type), _soh_name(m_type))
        : _ja_en(mons_type_name_j(m_type, DESC_PLAIN), str);

    monster_type base_type = MONS_NO_MONSTER;
    // HACK: Set an arbitrary humanoid monster as base type.
    if (mons_class_is_zombified(m_type))
        base_type = MONS_GOBLIN;

    monster_info fake_mon(m_type, base_type);
    fake_mon.props[FAKE_MON_KEY] = true;

    mslot = fake_mon;

#ifndef USE_TILE_LOCAL
    int colour = mons_class_colour(m_type);
    if (colour == BLACK)
        colour = LIGHTGREY;

    string prefix = "(<";
    prefix += colour_to_str(colour);
    prefix += ">";
    prefix += stringize_glyph(mons_char(m_type));
    prefix += "</";
    prefix += colour_to_str(colour);
    prefix += ">) ";

    const string title = prefix + name;
#else
    const string &title = name;
#endif

    // NOTE: MonsterMenuEntry::get_tiles() takes care of setting
    // up a fake weapon when displaying a fake dancing weapon's
    // tile.
    return new MonsterMenuEntry(title, &mslot, letter);
}

static bool _make_item_fake_unrandart(item_def &item, int unrand_index)
{
    // fill in the item info without marking the unrand as actually generated
    // use API rather than unwinds so that sanity checks don't need to be
    // duplicated
    const auto prior_status = get_unique_item_status(unrand_index);
    unwind_var<uint8_t> octo(you.octopus_king_rings, 0x0); // easier to do unconditionally
    const bool r = make_item_unrandart(item, unrand_index);
    set_unique_item_status(item, prior_status);
    return r;
}

/**
 * Generate a ?/I menu entry. (ref. _simple_menu_gen()).
 */
static MenuEntry* _item_menu_gen(char letter, const string &str, string &key)
{
    // アイテム名・固定アーティファクト名は data-items.txt / data-unrands.txt で引く
    MenuEntry* me = _simple_menu_gen(letter, _ja_en(jtrans(str), str), key);
    item_def item;
    item_kind kind = item_kind_by_name(key);
    if (kind.base_type == OBJ_UNASSIGNED)
        _make_item_fake_unrandart(item, extant_unrandart_by_exact_name(key));
    else
        get_item_by_name(&item, key.c_str(), kind.base_type);
    item_colour(item);
    tileidx_t idx = tileidx_item(item);
    tileidx_t base_item = tileidx_known_base_item(idx);
    if (base_item)
        me->add_tile(tile_def(base_item));
    me->add_tile(tile_def(idx));
    return me;
}

/**
 * Generate a ?/F menu entry. (ref. _simple_menu_gen()).
 */
static MenuEntry* _feature_menu_gen(char letter, const string &str, string &key)
{
    const dungeon_feature_type feat = feat_by_desc(str);
    const string name_j = feat ? feature_description_j(feat, NUM_TRAPS, "",
                                                       DESC_A, NUM_BRANCHES)
                               : str;
    MenuEntry* me = new MenuEntry(_ja_en(name_j, str), MEL_ITEM, 1, letter);
    me->data = &key;

    if (feat)
    {
        const tileidx_t idx = tileidx_feature_base(feat);
        me->add_tile(tile_def(idx));
    }

    return me;
}

/**
 * Generate a ?/G menu entry. (ref. _simple_menu_gen()).
 */
static MenuEntry* _god_menu_gen(char /*letter*/, const string &/*str*/, string &key)
{
    return new GodMenuEntry(str_to_god(key));
}

/**
 * Generate a ?/A menu entry. (ref. _simple_menu_gen()).
 */
static MenuEntry* _ability_menu_gen(char letter, const string &str, string &key)
{
    const ability_type ability = ability_by_name(str);
    MenuEntry* me = _simple_menu_gen(letter,
        ability != ABIL_NON_ABILITY ? _ja_en(ability_name_j(ability), str) : str,
        key);

    if (ability != ABIL_NON_ABILITY)
        me->add_tile(tile_def(tileidx_ability(ability)));

    return me;
}

/**
 * Generate a ?/C menu entry. (ref. _simple_menu_gen()).
 */
static MenuEntry* _card_menu_gen(char letter, const string &str, string &key)
{
    MenuEntry* me = _simple_menu_gen(letter,
                                     _ja_en(tagged_jtrans("[card]", str), str),
                                     key);
    me->add_tile(tile_def(TILEG_NEMELEX_CARD));
    return me;
}

/**
 * Generate a ?/S menu entry. (ref. _simple_menu_gen()).
 */
static MenuEntry* _spell_menu_gen(char letter, const string &str, string &key)
{
    const spell_type spell = spell_by_name(str);
    MenuEntry* me = _simple_menu_gen(letter,
        spell != SPELL_NO_SPELL ? _ja_en(spell_title_j(spell), str) : str,
        key);

    if (spell != SPELL_NO_SPELL)
        me->add_tile(tile_def(tileidx_spell(spell)));
    me->colour = is_player_spell(spell) ? WHITE
                                        : DARKGREY; // monster-only

    return me;
}

/**
 * Generate a ?/K menu entry. (ref. _simple_menu_gen()).
 */
static MenuEntry* _skill_menu_gen(char letter, const string &str, string &key)
{
    const skill_type skill = str_to_skill_safe(str);
    MenuEntry* me = _simple_menu_gen(letter,
        skill != SK_NONE ? _ja_en(skill_name_j(skill), str) : str, key);

    me->add_tile(tile_def(tileidx_skill(skill, TRAINING_ENABLED)));

    return me;
}

/**
 * Generate a ?/B menu entry. (ref. _simple_menu_gen()).
 */
static MenuEntry* _branch_menu_gen(char letter, const string &str, string &key)
{
    const branch_type branch = branch_by_shortname(str);
    // str は短い名前("D" など)。表示は長い名前の訳と併記する
    MenuEntry* me = _simple_menu_gen(letter,
        branch != NUM_BRANCHES
            ? _ja_en(branch_name_j(branches[branch].longname), str) : str,
        key);

    int hotkey = branches[branch].travel_shortcut;
    me->hotkeys = {hotkey, tolower_safe(hotkey)};
    me->add_tile(tile_def(tileidx_branch(branch)));

    return me;
}

/**
 * Generate a ?/L menu entry. (ref. _simple_menu_gen()).
 */
static MenuEntry* _cloud_menu_gen(char letter, const string &str, string &key)
{
    MenuEntry* me = _simple_menu_gen(letter,
                                     _ja_en(tagged_jtrans("[cloud]", str), str),
                                     key);

    const string cloud_name = lowercase_string(str);
    const cloud_type cloud = cloud_name_to_type(cloud_name);
    ASSERT(cloud != NUM_CLOUD_TYPES);

    cloud_struct fake_cloud;
    fake_cloud.type = cloud;
    fake_cloud.decay = 1000;
    me->colour = element_colour(get_cloud_colour(fake_cloud), fake_cloud.pos);

    cloud_info fake_cloud_info;
    fake_cloud_info.type = cloud;
    fake_cloud_info.colour = me->colour;
    const tileidx_t idx = tileidx_cloud(fake_cloud_info);
    me->add_tile(tile_def(idx));

    return me;
}

/**
 * Generate a ?/U menu entry. (ref. _simple_menu_gen()).
 */
static MenuEntry* _mut_menu_gen(char letter, const string &str, string &key)
{
    // 変異の名前は data-mutations.txt で引く
    MenuEntry* me = _simple_menu_gen(letter, _ja_en(jtrans(str), str), key);

    const mutation_type mut = mutation_from_name(str, false);
    if (mut == NUM_MUTATIONS)
        return me;

    const tileidx_t tile = get_mutation_tile(mut);
    if (tile)
        me->add_tile(tile_def(tile + mutation_max_levels(mut) - 1));

    return me;
}

/**
 * Generate a ?/P menu entry. (ref. _simple_menu_gen()).
 */
static MenuEntry* _passive_menu_gen(char letter, const string &str, string &key)
{
    return _simple_menu_gen(letter, _ja_en(jtrans(str), str), key);
}

/**
 * Generate a ?/T menu entry. (ref. _simple_menu_gen()).
 * 状態の名前は data-durations.txt の [dur] で引く
 */
static MenuEntry* _status_menu_gen(char letter, const string &str, string &key)
{
    return _simple_menu_gen(letter, _ja_en(tagged_jtrans("[dur]", str), str),
                            key);
}

/**
 * Generate a ?/N menu entry. (ref. _simple_menu_gen()).
 * 災厄の名前は mutation.txt の [bane] で引く
 */
static MenuEntry* _bane_menu_gen(char letter, const string &str, string &key)
{
    return _simple_menu_gen(letter, _ja_en(tagged_jtrans("[bane]", str), str),
                            key);
}


/**
 * How should this type be expressed in the prompt string?
 *
 * @return The 'type', with the first instance of the 'symbol' found &
 *          replaced with an uppercase version surrounded by parens
 *          e.g. "monster", 'm' -> "(M)onster"
 */
string LookupType::prompt_string() const
{
    string prompt_str = lowercase_string(type);
    const size_t symbol_pos = prompt_str.find(tolower_safe(symbol));
    ASSERT(symbol_pos != string::npos);

    prompt_str.replace(symbol_pos, 1, make_stringf("(%c)", toupper_safe(symbol)));
    return prompt_str;
}

/**
 * A suffix to be appended to the provided search string when looking for
 * db info.
 *
 * @return      An appropriate suffix for types that need them (e.g.
 *              " cards"); otherwise "".
 */
string LookupType::suffix() const
{
    if (flags & lookup_type::db_suffix)
        return " " + type;
    return "";
}

/**
 * Get a list of string corresponding to the given regex.
 */
vector<string> LookupType::matching_keys(string regex) const
{
    vector<string> key_list;

    if (no_search())
        key_list = simple_key_fetch();
    else if (regex.size() == 1 && supports_glyph_lookup())
        key_list = glyph_fetch(regex[0]);
    else
        key_list = get_desc_keys(regex);

    if (recap != nullptr)
        (*recap)(key_list);

    return key_list;
}

static string _mons_desc_key(monster_type type)
{
    // No "the Royal Jelly".
    string name = remove_prepended_the(mons_type_name(type, DESC_PLAIN));
    if (mons_species(type) == MONS_SERPENT_OF_HELL)
        return name + " " + serpent_of_hell_flavour(type);
    return name;
}

/**
 * Build a menu listing the given keys, and allow the player to interact
 * with them.
 */
void LookupType::display_keys(vector<string> &key_list) const
{
    DescMenu desc_menu(MF_SINGLESELECT | MF_ANYPRINTABLE | MF_ALLOW_FORMATTING
                | MF_USE_TWO_COLUMNS | MF_ARROWS_SELECT | MF_INIT_HOVER,
            toggleable_sort());
    desc_menu.set_tag("description");

    // XXX: ugh
    const bool doing_mons = type == "monster";
    vector<monster_info> monster_list(key_list.size());
    int letter_i = 0;
    for (unsigned int i = 0, size = key_list.size(); i < size; i++)
    {
        const char letter = index_to_letter(letter_i % 52);
        string &key = key_list[i];
        // XXX: double ugh
        auto *entry = doing_mons
            ? _monster_menu_gen(letter, key_to_menu_str(key), monster_list[i])
            : make_menu_entry(letter, key);

        if (!entry)
            continue;
        desc_menu.add_entry(entry);
        letter_i++;
    }

    desc_menu.sort();

    desc_menu.on_single_selection = [this, doing_mons](const MenuEntry& item)
    {
        ASSERT(item.hotkeys.size() >= 1);

        string key;

        if (doing_mons)
        {
            monster_info* mon = (monster_info*) item.data;
            key = _mons_desc_key(mon->type);
        }
        else
            key = *((string*) item.data);

        describe(key);
        return true;
    };

    while (true)
    {
        desc_menu.show();
        if (toggleable_sort() && desc_menu.getkey() == CONTROL('S'))
            desc_menu.toggle_sorting();
        else
            break;
    }
}

/**
 * Generate a description menu entry for the given key.
 *
 * @param letter    The letter with which the entry should be labeled.
 * @param key       The key for the entry.
 * @return          A pointer to a new MenuEntry object.
 */
MenuEntry* LookupType::make_menu_entry(char letter, string &key) const
{
    ASSERT(menu_gen);
    return menu_gen(letter, key_to_menu_str(key), key);
}

/**
 * Turn a DB string into a nice menu title.
 *
 * @param key       The key in question. (E.g. "blade card").
 * @return          A nicer string. (E.g. "Blade").
 */
string LookupType::key_to_menu_str(const string &key) const
{
    string str = uppercase_first(key);
    // perhaps we should assert this?
    strip_suffix(str, suffix());
    return str;
}

/**
 * Handle describing & interacting with a given key.
 * @return the last key pressed.
 */
int LookupType::describe(const string &key, bool exact_match) const
{
    const string footer
        = exact_match ? make_stringf(jtransc("This entry is an exact match "
                                "for '%s'. To see non-exact matches, press "
                                "space."), key.c_str())
        : "";
    return describer(key, suffix(), footer);
}

/**
 * Describe the thing with the given name.
 *
 * @param key           The name of the thing in question.
 * @param suffix        A suffix to trim from the key when making the title.
 * @param footer        A footer to append to the end of descriptions.
 * @param extra_info    Extra info to append to the database description.
 * @return              The keypress the user made to exit.
 */
static int _describe_key(const string &key, const string &suffix,
                         string footer, const string &extra_info,
                         const tile_def *tile = nullptr,
                         const string &title_j = "")
{
    describe_info inf;
    inf.quote = getQuoteString(key);

    const string desc = getLongDescription(key);

    inf.body << desc << extra_info;
    inf.title = [&]() {
        string title = key;
        strip_suffix(title, suffix);
        title = uppercase_first(title);
        // 見出しは日本語名(英語名を併記)。title_j がなければ辞書で引く
        return _ja_en(title_j.empty() ? jtrans(title) : title_j, title);
    }();
    inf.footer = footer;

    return show_description(inf, tile);
}

/**
 * Describe the thing with the given name.
 *
 * @param key       The name of the thing in question.
 * @param suffix    A suffix to trim from the key when making the title.
 * @param footer    A footer to append to the end of descriptions.
 * @return          The keypress the user made to exit.
 */
static int _describe_generic(const string &key, const string &suffix,
                             string footer)
{
    return _describe_key(key, suffix, footer, "");
}

/**
 * Describe the status with the given name. (ref. _describe_generic())
 * 見出しの状態名は data-durations.txt の [dur] で引く。
 */
static int _describe_status(const string &key, const string &suffix,
                            string footer)
{
    string name = key;
    strip_suffix(name, suffix);
    return _describe_key(key, suffix, footer, "", nullptr,
                         tagged_jtrans("[dur]", name));
}

static map<monster_type, monster_type> draconian_job_to_colour =
{
    { MONS_DRACONIAN_STORMCALLER,   MONS_WHITE_DRACONIAN },
    { MONS_DRACONIAN_MONK,          MONS_GREEN_DRACONIAN },
    { MONS_DRACONIAN_SHIFTER,       MONS_PURPLE_DRACONIAN },
    { MONS_DRACONIAN_ANNIHILATOR,   MONS_YELLOW_DRACONIAN },
    { MONS_DRACONIAN_KNIGHT,        MONS_BLACK_DRACONIAN },
    { MONS_DRACONIAN_SCORCHER,      MONS_RED_DRACONIAN }
};

/**
 * Describe & allow examination of the monster with the given name.
 *
 * @param key       The name of the monster in question.
 * @param suffix    A suffix to trim from the key when making the title.
 * @param footer    A footer to append to the end of descriptions.
 * @return          The keypress the user made to exit.
 */
static int _describe_monster(const string &key, const string &suffix,
                             string footer)
{
    const monster_type mon_num = _mon_by_name(key);
    ASSERT(mon_num != MONS_PROGRAM_BUG);
    // Don't attempt to get more information on ghost demon
    // monsters, as the ghost struct has not been initialised, which
    // will cause a crash. Similarly for zombified monsters, since
    // they require a base monster.
    if (mons_is_ghost_demon(mon_num) || mons_class_is_zombified(mon_num))
        return _describe_generic(key, suffix, footer);

    monster_type base_type = MONS_NO_MONSTER;
    if (mons_is_draconian_job(mon_num))
    {
        // Classed draconians have a fixed colour per job since 0.28
        const auto colour_it = draconian_job_to_colour.find(mon_num);
        if (colour_it != draconian_job_to_colour.end())
            base_type = colour_it -> second;
        else
        {
            // Might be better to show all possible combinations rather than
            // picking one at random as this does?
            base_type = random_draconian_monster_species();
        }
    }
    monster_info mi(mon_num, base_type);
    // Avoid slime creature being described as "buggy"
    if (mi.type == MONS_SLIME_CREATURE)
        mi.slime_size = 1;
    mi.props[FAKE_MON_KEY] = true;
    return describe_monster(mi, footer);
}


/**
 * Describe the spell with the given name.
 *
 * @param key       The name of the spell in question.
 * @param suffix    A suffix to trim from the key when making the title.
 * @param footer    A footer to append to the end of descriptions.
 * @return          The keypress the user made to exit.
 */
static int _describe_spell(const string &key, const string &suffix,
                             string /*footer*/)
{
    const string spell_name = key.substr(0, key.size() - suffix.size());
    const spell_type spell = spell_by_name(spell_name, true);
    ASSERT(spell != SPELL_NO_SPELL);
    describe_spell(spell);
    return 0;
}

static int _describe_skill(const string &key, const string &suffix,
                             string /*footer*/)
{
    const string skill_name = key.substr(0, key.size() - suffix.size());
    const skill_type skill = skill_from_name(skill_name.c_str());
    describe_skill(skill);
    return 0;
}

static int _describe_ability(const string &key, const string &suffix,
                             string /*footer*/)
{
    const string abil_name = key.substr(0, key.size() - suffix.size());
    const ability_type abil = ability_by_name(abil_name.c_str());
    describe_ability(abil);
    return 0;
}

/**
 * Describe the card with the given name.
 *
 * @param key       The name of the card in question.
 * @param suffix    A suffix to trim from the key when making the title.
 * @param footer    A footer to append to the end of descriptions.
 * @return          The keypress the user made to exit.
 */
static int _describe_card(const string &key, const string &suffix,
                           string footer)
{
    const string card_name = key.substr(0, key.size() - suffix.size());
    const card_type card = name_to_card(card_name);
    ASSERT(card != NUM_CARDS);
#ifdef USE_TILE
    tile_def tile = tile_def(TILEG_NEMELEX_CARD);
    return _describe_key(key, suffix, footer, which_decks(card) + "\n", &tile,
                         tagged_jtrans("[card]", card_name));
#else
    return _describe_key(key, suffix, footer, which_decks(card) + "\n",
                         nullptr, tagged_jtrans("[card]", card_name));
#endif
}

/**
 * Describe the cloud with the given name.
 *
 * @param key       The name of the cloud in question.
 * @param suffix    A suffix to trim from the key when making the title.
 * @param footer    A footer to append to the end of descriptions.
 * @return          The keypress the user made to exit.
 */
static int _describe_cloud(const string &key, const string &suffix,
                           string footer)
{
    const string cloud_name = key.substr(0, key.size() - suffix.size());
    const cloud_type cloud = cloud_name_to_type(cloud_name);
    ASSERT(cloud != NUM_CLOUD_TYPES);
#ifdef USE_TILE
    cloud_info fake_cloud_info;
    fake_cloud_info.type = cloud;
    const tileidx_t idx = tileidx_cloud(fake_cloud_info);
    tile_def tile = tile_def(idx);
    return _describe_key(key, suffix, footer, extra_cloud_info(cloud), &tile,
                         tagged_jtrans("[cloud]", cloud_name));
#else
    return _describe_key(key, suffix, footer, extra_cloud_info(cloud),
                         nullptr, tagged_jtrans("[cloud]", cloud_name));
#endif
}

/**
 * Describe the item with the given name.
 *
 * @param key       The name of the item in question.
 * @param suffix    A suffix to trim from the key when making the title.
 * @param footer    A footer to append to the end of descriptions.
 * @return          The keypress the user made to exit.
 */
static int _describe_item(const string &key, const string &suffix,
                           string /*footer*/)
{
    const string item_name = key.substr(0, key.size() - suffix.size());
    item_def item;
    if (!get_item_by_exact_name(item, item_name.c_str()))
    {
        const int unrand_idx = extant_unrandart_by_exact_name(item_name);
        if (!unrand_idx)
        {
            ui::error(make_stringf("Unable to get item '%s' by name", key.c_str()));
            return 0;
        }
        _make_item_fake_unrandart(item, unrand_idx);
    }
    describe_item_popup(item);
    return 0;
}

static int _describe_feature(const string &key, const string &suffix,
                             string /*footer*/)
{
    const string feat_name = key.substr(0, key.size() - suffix.size());
    const dungeon_feature_type feat = feat_by_desc(feat_name);
    describe_feature_type(feat);
    return 0;
}

/**
 * Describe the god with the given name.
 *
 * @param key       The name of the god in question.
 * @return          0.
 */
static int _describe_god(const string &key, const string &/*suffix*/,
                          string /*footer*/)
{
    const god_type which_god = str_to_god(key);
    ASSERT(which_god != GOD_NO_GOD);
    describe_god(which_god);

    return 0; // no exact matches for gods, so output doesn't matter
}

static string _branch_transit_runes(branch_type br)
{
    if (br != BRANCH_VAULTS && br != BRANCH_ZOT)
        return "";

    const bool exit = br == BRANCH_VAULTS;
    const int num_runes = br == BRANCH_ZOT ? ZOT_ENTRY_RUNES : 1;
    // 英語では "%sed"(exit/enter)と複数形の s を埋め込んでいるが、
    // 訳は出る・入るで全文を分ける
    return "\n\n" + make_stringf(exit
        ? jtransc("This branch can only be exited while carrying at least %d "
                  "rune(s) of Zot.")
        : jtransc("This branch can only be entered while carrying at least %d "
                  "rune(s) of Zot."),
        num_runes);
}

static string _branch_depth(branch_type br)
{
    string desc;
    const int depth = branches[br].numlevels;

    // Abyss depth is explained in the description.
    if (br == BRANCH_ABYSS)
    {
        desc = "\n" + make_stringf(jtransc("(If you entered the Abyss now, "
                            "you could be pulled as deep as Abyss:%d.)"),
                            abyss_default_depth(true));
    }
    if (depth > 1 && br != BRANCH_ABYSS)
    {
        desc = "\n\n" + make_stringf(br == BRANCH_ZIGGURAT
                                     ? jtransc("This portal is %d levels deep.")
                                     : jtransc("This branch is %d levels deep."),
                                     depth);
    }

    return desc;
}

static string _branch_location(branch_type br)
{
    string desc;
    const branch_type parent = branches[br].parent_branch;
    const int min = branches[br].mindepth;
    const int max = branches[br].maxdepth;

    // Ziggurat locations are explained in the description.
    if (parent != NUM_BRANCHES && br != BRANCH_ZIGGURAT)
    {
        // 日本語では「〜の第%d階層」と分岐名が先に来るので、
        // 分岐名を先に渡す形の文を引く
        const string parent_name = branch_name_j(branches[parent].longname);
        desc = "\n\n";
        if (min == max)
        {
            if (branches[parent].numlevels == 1)
            {
                desc += make_stringf(jtransc("The entrance to this branch can "
                                             "be found in %s."),
                                     parent_name.c_str());
            }
            else
            {
                desc += make_stringf(jtransc("The entrance to this branch can "
                                             "be found in %s, on level %d."),
                                     parent_name.c_str(), min);
            }
        }
        else
        {
            desc += make_stringf(jtransc("The entrance to this branch can be "
                                         "found in %s, between levels %d and "
                                         "%d."),
                                 parent_name.c_str(), min, max);
        }
    }

    return desc;
}

static string _branch_subbranches(branch_type br)
{
    string desc;
    vector<string> subbranch_names;

    for (branch_iterator it; it; ++it)
        if (it->parent_branch == br && !branch_is_unfinished(it->id))
            subbranch_names.push_back(branch_name_j(it->longname));

    // Lair's random branches are explained in the description.
    if (!subbranch_names.empty() && br != BRANCH_LAIR)
    {
        const string names = comma_separated_line(begin(subbranch_names),
                                                  end(subbranch_names),
                                                  "、", "、");
        desc += "\n\n";
        desc += make_stringf(subbranch_names.size() > 1
                             ? jtransc("This branch contains the entrances to %s.")
                             : jtransc("This branch contains the entrance to %s."),
                             names.c_str());
    }

    return desc;
}

/**
 * Describe the branch with the given name.
 *
 * @param key       The name of the branch in question.
 * @param suffix    A suffix to trim from the key when making the title.
 * @param footer    A footer to append to the end of descriptions.
 * @return          The keypress the user made to exit.
 */
static int _describe_branch(const string &key, const string &suffix,
                            string footer)
{
    const string branch_name = key.substr(0, key.size() - suffix.size());
    const branch_type branch = branch_by_shortname(branch_name);
    ASSERT(branch != NUM_BRANCHES);

    string info = "";
    const string noise_desc = branch_noise_desc(branch);
    if (!noise_desc.empty())
        info += "\n\n" + noise_desc;

    info += _branch_location(branch)
            + _branch_transit_runes(branch)
            + _branch_depth(branch)
            + _branch_subbranches(branch)
            + "\n\n"
            + branch_rune_desc(branch, false);

    tile_def tile = tile_def(tileidx_branch(branch));
    return _describe_key(key, suffix, footer, info, &tile,
                         branch_name_j(branches[branch].longname));
}

static int _describe_mutation(const string &key, const string &suffix,
                              string /*footer*/)
{
    const string mutation_name = key.substr(0, key.size() - suffix.size());
    const mutation_type mutation = mutation_from_name(mutation_name.c_str(),
                                                      false);
    if (mutation == NUM_MUTATIONS) // oops! someone messed up!
    {
        ui::error(make_stringf("Unable to get '%s' by name", key.c_str()));
        return 0;
    }
    describe_mutation(mutation);
    return 0;
}

static int _describe_bane(const string &key, const string &suffix,
                              string /*footer*/)
{
    const string bane_name = key.substr(0, key.size() - suffix.size());
    const bane_type bane = bane_from_name(bane_name.c_str());
    if (bane == NUM_BANES)
    {
        ui::error(make_stringf("Unable to get '%s' by name", key.c_str()));
        return 0;
    }
    describe_bane(bane);
    return 0;
}

/// All types of ?/ queries the player can enter.
static const vector<LookupType> lookup_types = {
    LookupType('M', "monster", _recap_mon_keys, _monster_filter,
               _get_monster_keys, nullptr, nullptr,
               _describe_monster, lookup_type::toggleable_sort),
    LookupType('S', "spell", _recap_spell_keys, _spell_filter,
               nullptr, nullptr, _spell_menu_gen,
               _describe_spell, lookup_type::db_suffix),
    LookupType('K', "skill", nullptr, nullptr,
               nullptr, _get_skill_keys, _skill_menu_gen,
               _describe_skill, lookup_type::none),
    LookupType('A', "ability", _recap_ability_keys, _ability_filter,
               nullptr, nullptr, _ability_menu_gen,
               _describe_ability, lookup_type::db_suffix),
    LookupType('C', "card", nullptr, nullptr,
               nullptr, _get_card_keys, _card_menu_gen,
               _describe_card, lookup_type::db_suffix),
    LookupType('I', "item", _recap_item_keys, _item_filter,
               item_name_list_for_glyph, nullptr, _item_menu_gen,
               _describe_item, lookup_type::none),
    LookupType('F', "feature", _recap_feat_keys, _feature_filter,
               nullptr, nullptr, _feature_menu_gen,
               _describe_feature, lookup_type::none),
    LookupType('G', "god", nullptr, nullptr,
               nullptr, _get_god_keys, _god_menu_gen,
               _describe_god, lookup_type::none),
    LookupType('B', "branch", nullptr, nullptr,
               nullptr, _get_branch_keys, _branch_menu_gen,
               _describe_branch, lookup_type::disable_sort),
    LookupType('L', "cloud", nullptr, nullptr,
               nullptr, _get_cloud_keys, _cloud_menu_gen,
               _describe_cloud, lookup_type::db_suffix),
    LookupType('P', "passive", nullptr, _passive_filter,
               nullptr, nullptr, _passive_menu_gen,
               _describe_generic, lookup_type::db_suffix),
    LookupType('T', "status", nullptr, _status_filter,
               nullptr, nullptr, _status_menu_gen,
               _describe_status, lookup_type::db_suffix),
    LookupType('U', "mutation", nullptr, _mutation_filter,
               nullptr, nullptr, _mut_menu_gen,
               _describe_mutation, lookup_type::db_suffix),
    LookupType('N', "bane", nullptr, _bane_filter,
               nullptr, nullptr, _bane_menu_gen,
               _describe_bane, lookup_type::db_suffix),
};

/**
 * Build a mapping from LookupTypes' symbols to the objects themselves.
 */
static map<char, const LookupType*> _build_lookup_type_map()
{
    ASSERT(lookup_types.size() == NUM_LOOKUP_HELP_TYPES);
    map<char, const LookupType*> lookup_map;
    for (auto &lt : lookup_types)
        lookup_map[lt.symbol] = &lt;
    return lookup_map;
}
static const map<char, const LookupType*> _lookup_types_by_symbol
    = _build_lookup_type_map();

/// Return the display name (lowercase) for the given lookup type.
string lookup_help_type_name(lookup_help_type lht)
{
    ASSERT(lht >= 0 && lht < NUM_LOOKUP_HELP_TYPES);
    return lookup_types[lht].type;
}

/// Return the hotkey character for the given lookup type.
char lookup_help_type_shortcut(lookup_help_type lht)
{
    ASSERT(lht >= 0 && lht < NUM_LOOKUP_HELP_TYPES);
    return lookup_types[lht].symbol;
}

/**
 * Prompt the player for a search string for the given lookup type.
 *
 * @param lookup_type  The LookupType in question (e.g. monsters, items...)
 * @param err[out]     Will be set to a non-empty string if the user failed to
 *                     provide a string.
 * @return             A search string, if one was provided; else "".
 */
static string _prompt_for_regex(const LookupType &lookup_type, string &err)
{
    const string type = lowercase_string(lookup_type.type);
    // 種類の名前(「モンスター」など)。英語の複数形・冠詞は付けない
    const string type_j = tagged_jtrans("[lookup type]", type);
    const string extra = lookup_type.supports_glyph_lookup() ?
        make_stringf(jtransc(" Enter a single letter to list %s displayed by "
                             "that symbol."), type_j.c_str())
        : lookup_type.type == "spell"
            ? jtrans(" Preface with '@' to search by school.")
        : "";
    const string prompt = make_stringf(
         jtrans_notrimc("Describe %s; partial names and regexps are fine.%s\n"
                        "Describe what? "),
         type_j.c_str(), extra.c_str());

    char buf[80];
    if (msgwin_get_line(prompt, buf, sizeof(buf)) || buf[0] == '\0')
    {
        err = "Okay, then.";
        return "";
    }

    const string regex = strlen(buf) == 1 ? buf : trimmed_string(buf);
    return regex;
}

static bool _exact_lookup_match(const LookupType &lookup_type,
                                const string &regex)
{
    if (lookup_type.no_search())
        return false; // no search, no exact match

    if (lookup_type.supports_glyph_lookup() && regex.size() == 1)
        return false; // glyph search doesn't have the concept

    if (lookup_type.filter_forbid && (*lookup_type.filter_forbid)(regex, ""))
        return false; // match found, but incredibly illegal to display

    return !getLongDescription(regex + lookup_type.suffix()).empty();
}

/**
 * Check if the provided keylist is invalid; if so, return the reason why.
 *
 * @param key_list      The list of keys to be checked before display.
 * @param type          The singular name of the things being listed.
 * @param regex         The search term that was used to fetch this list.
 * @param by_symbol     Whether the search is by regex or by glyph.
 * @return              A reason why the list is invalid, if it is
 *                      (e.g. "No monsters with symbol '👻'."),
 *                      or the empty string if the list is valid.
 */
static string _keylist_invalid_reason(const vector<string> &key_list,
                                      const string &type,
                                      const string &regex,
                                      bool by_symbol)
{
    // 種類の名前は日本語(複数形にしない)
    const string type_j = tagged_jtrans("[lookup type]", type);

    if (key_list.empty())
    {
        if (by_symbol)
        {
            return make_stringf(jtransc("No %s with symbol '%s'."),
                                type_j.c_str(), regex.c_str());
        }
        return make_stringf(jtransc("No matching %s for search string '%s'."),
            type_j.c_str(), regex.c_str());
    }

    // we're good!
    return "";
}

static void _show_type_response(string response)
{
    // possibly overkill, but this renders very reliably
    formatted_scroller fs(FS_EASY_EXIT);
    fs.set_more();
    fs.add_text(response);
    fs.show();
}

bool find_description_of_type(lookup_help_type lht)
{
    ASSERT(lht >= 0 && lht < NUM_LOOKUP_HELP_TYPES);
    string response;
    bool done = lookup_types[lht].find_description(response);
    if (!response.empty() && response != "Okay, then.") // TODO: ...
        _show_type_response(response);
    return done;
}

/**
 * Run an iteration of ?/ for the given lookup type.
 *
 * @param response[out]   A response to input, to print before the next iter.
 * @return                true if the ?/ loop should continue
 *                        false if it should return control to the caller
 */
bool LookupType::find_description(string &response) const
{
    const bool want_regex = !no_search();
    const string regex = want_regex ?
                         _prompt_for_regex(*this, response) :
                         "";

    if (!response.empty())
        return true;

    // not actually sure how to trigger this branch...
    if (want_regex && regex.empty())
    {
        response = jtrans("Description must contain at least one non-space.");
        return true;
    }

    // Try to get an exact match first.
    const bool exact_match = _exact_lookup_match(*this, regex);

    vector<string> key_list = matching_keys(regex);

    const bool by_symbol = supports_glyph_lookup() && regex.size() == 1;
    response = _keylist_invalid_reason(key_list, lowercase_string(type),
                                       regex, by_symbol);
    if (!response.empty())
        return true;

    if (key_list.size() == 1)
    {
        describe(key_list[0]);
        return true;
    }

    if (exact_match && describe(regex, true) != ' ')
        return true;

    if (!(flags & lookup_type::disable_sort))
        sort(key_list.begin(), key_list.end());

    display_keys(key_list);
    return true;
}

class LookupHelpMenu : public Menu
{
public:
    class LookupHelpMenuEntry : public MenuEntry
    {
    public:
        LookupHelpMenuEntry(lookup_help_type lht)
        // 表示は日本語の種類名。lookup_help_type_name() は英語のまま
        : MenuEntry(tagged_jtrans("[lookup type]",
                                  lookup_help_type_name(lht)),
                    MEL_ITEM, 1, tolower(lookup_help_type_shortcut(lht))),
          typ(lht)
        {
            // TODO: tiles!
        }

        lookup_help_type typ;
    };

    LookupHelpMenu(command_type where_from=CMD_NO_CMD)
        : Menu(MF_SINGLESELECT | MF_ALLOW_FORMATTING
                | MF_ARROWS_SELECT | MF_WRAP),
          back_cmd(where_from)
    {
        action_cycle = Menu::CYCLE_NONE;
        menu_action  = Menu::ACT_EXECUTE;
        set_title(new MenuEntry(jtrans("Lookup information about:"), MEL_TITLE));
        on_single_selection = [](const MenuEntry& item)
        {
            const LookupHelpMenuEntry *lhme
                        = dynamic_cast<const LookupHelpMenuEntry *>(&item);
            if (!lhme)
                return false; // back button
            find_description_of_type(lhme->typ);
            return true;
        };
    }

    void fill_entries()
    {
        clear();
        // XX `?/esc` doesn't go back to the main help menu, possibly should
        const string back = back_cmd == CMD_GAME_MENU
                                ? jtrans("Back to game menu")
                                : jtrans("Exit help lookup");
        auto back_button = new MenuEntry(back, MEL_ITEM, 1, CK_ESCAPE);
        if (back_cmd != CMD_NO_CMD)
            back_button->add_tile(tileidx_command(back_cmd));

        for (int i = FIRST_LOOKUP_HELP_TYPE; i < NUM_LOOKUP_HELP_TYPES; ++i)
            add_entry(new LookupHelpMenuEntry((lookup_help_type)i));

        add_entry(new MenuEntry("", MEL_SUBTITLE));
        add_entry(back_button);
    }

    vector<MenuEntry *> show(bool reuse_selections = false) override
    {
        fill_entries();
        cycle_hover();
        return Menu::show(reuse_selections);
    }

private:
    command_type back_cmd;
};

void keyhelp_query_descriptions(command_type where_from)
{
    rng::generator rng(rng::UI);

    // force a sequence point
    {
        LookupHelpMenu m(where_from);
        m.show();
    }
}

// =====================================================================
// 日本語版: スポイラー用のデータの書き出し(-spoiler-json)
//
// ゲーム内検索(?/)の全項目の説明を、画面を開かずに受け取って JSON に書く。
// 文はゲームの説明画面と同じ(言語の設定どおり)。ほかに、一覧の表に使う
// 数値(モンスター・種族・職業・支部・出現表など)を構造のまま書く。
// サイトを作る道具は ja-tools/spoiler/ にある。
// =====================================================================

static JsonNode *_sp_str(const string &s)
{
    return json_mkstring(s.c_str());
}

// タイルの番号を、画像シートの名前と切り出す位置にする
static JsonNode *_sp_tile(tileidx_t idx)
{
#ifdef USE_TILE
    idx &= TILE_FLAG_MASK;
    if (!idx)
        return json_mknull();

    const char *sheet;
    const tile_info *ti;
    if (idx < TILE_FLOOR_MAX)
        sheet = "floor", ti = &tile_floor_info(idx);
    else if (idx < TILE_WALL_MAX)
        sheet = "wall", ti = &tile_wall_info(idx);
    else if (idx < TILE_FEAT_MAX)
        sheet = "feat", ti = &tile_feat_info(idx);
    else if (idx < TILE_MAIN_MAX)
        sheet = "main", ti = &tile_main_info(idx);
    else if (idx < TILEP_PLAYER_MAX)
        sheet = "player", ti = &tile_player_info(idx);
    else if (idx < TILEG_GUI_MAX)
        sheet = "gui", ti = &tile_gui_info(idx);
    else if (idx < TILEI_ICONS_MAX)
        sheet = "icons", ti = &tile_icons_info(idx);
    else
        return json_mknull();

    JsonNode *t = json_mkobject();
    json_append_member(t, "sheet", json_mkstring(sheet));
    json_append_member(t, "w", json_mknumber(ti->width));
    json_append_member(t, "h", json_mknumber(ti->height));
    json_append_member(t, "ox", json_mknumber(ti->offset_x));
    json_append_member(t, "oy", json_mknumber(ti->offset_y));
    json_append_member(t, "sx", json_mknumber(ti->sx));
    json_append_member(t, "sy", json_mknumber(ti->sy));
    json_append_member(t, "ex", json_mknumber(ti->ex));
    json_append_member(t, "ey", json_mknumber(ti->ey));
    return t;
#else
    UNUSED(idx);
    return json_mknull();
#endif
}

static JsonNode *_sp_entry(const string &key, const string &title,
                           const string &body, const string &quote,
                           tileidx_t tile)
{
    JsonNode *e = json_mkobject();
    json_append_member(e, "key", _sp_str(key));
    json_append_member(e, "title", _sp_str(title));
    json_append_member(e, "body", _sp_str(body));
    json_append_member(e, "quote", _sp_str(quote));
    json_append_member(e, "tile", _sp_tile(tile));
    return e;
}

// 説明の画面(show_description)を開く代わりに中身を受け取る
static bool _sp_capture(const LookupType &lt, const string &key,
                        spoiler_capture &cap)
{
    cap = spoiler_capture();
    spoiler_desc_capture = &cap;
    lt.describe(key);
    spoiler_desc_capture = nullptr;
    return cap.captured;
}

static JsonNode *_sp_monster_entry(const string &key)
{
    const monster_type mc = _mon_by_name(key);
    // 色の決まっていない職業付きのドラコニアンは赤で代表させる
    monster_type base_type = MONS_NO_MONSTER;
    if (mons_is_draconian_job(mc))
    {
        const auto colour_it = draconian_job_to_colour.find(mc);
        if (colour_it != draconian_job_to_colour.end())
            base_type = colour_it->second;
        else
            base_type = MONS_RED_DRACONIAN;
    }
    monster_info mi(mc, base_type);
    if (mi.type == MONS_SLIME_CREATURE)
        mi.slime_size = 1;
    mi.props[FAKE_MON_KEY] = true;

    describe_info inf;
    bool has_stat_desc = false;
    get_monster_db_desc(mi, inf, has_stat_desc);

    JsonNode *e = _sp_entry(key, inf.title, inf.body.str(), inf.quote,
                            tileidx_monster(mi));

    const monsterentry *me = get_monster_data(mc);
    json_append_member(e, "name_ja", _sp_str(mons_type_name_j(mc, DESC_PLAIN)));
    json_append_member(e, "name_en", _sp_str(mons_type_name(mc, DESC_PLAIN)));
    string glyph;
    glyph += me->basechar;
    json_append_member(e, "glyph", _sp_str(glyph));
    json_append_member(e, "colour", _sp_str(colour_to_str(me->colour)));
    json_append_member(e, "hd", json_mknumber(me->HD));
    json_append_member(e, "hp10", json_mknumber(me->avg_hp_10x));
    json_append_member(e, "ac", json_mknumber(me->AC));
    json_append_member(e, "ev", json_mknumber(me->ev));
    json_append_member(e, "will", json_mknumber(mons_class_willpower(mc, mc)));
    json_append_member(e, "speed", json_mknumber(me->speed));
    json_append_member(e, "unique", json_mkbool(mons_is_unique(mc)));
    json_append_member(e, "holiness", json_mknumber((int)me->holiness));
    return e;
}

static JsonNode *_sp_spell_entry(const string &key, const string &suffix)
{
    const spell_type spell = spell_by_name(key.substr(0, key.size()
                                                      - suffix.size()), true);
    if (spell == SPELL_NO_SPELL)
        return nullptr;
    describe_info inf;
    get_spell_desc(spell, inf);
    JsonNode *e = _sp_entry(key, inf.title, inf.body.str(), inf.quote,
                            tileidx_spell(spell));
    json_append_member(e, "name_ja", _sp_str(spell_title_j(spell)));
    json_append_member(e, "name_en", _sp_str(spell_title(spell)));
    json_append_member(e, "level", json_mknumber(spell_difficulty(spell)));
    JsonNode *schools = json_mkarray();
    for (const auto bit : spschools_type::range())
        if (spell_typematch(spell, bit))
        {
            json_append_element(schools,
                                _sp_str(spelltype_long_name_j(bit)));
        }
    json_append_member(e, "schools", schools);
    json_append_member(e, "player", json_mkbool(is_player_book_spell(spell)));
    return e;
}

static JsonNode *_sp_item_entry(const string &key)
{
    item_def item;
    bool unrand = false;
    if (!get_item_by_exact_name(item, key.c_str()))
    {
        const int unrand_idx = extant_unrandart_by_exact_name(key);
        if (!unrand_idx || !_make_item_fake_unrandart(item, unrand_idx))
            return nullptr;
        unrand = true;
    }
    JsonNode *e = _sp_entry(key, item.name(DESC_A, false, true),
                            get_item_description(item), "",
                            tileidx_item(item));
    json_append_member(e, "name_ja", _sp_str(jtrans(key)));
    json_append_member(e, "base", _sp_str(base_type_string(item.base_type)));
    json_append_member(e, "sub", json_mknumber(item.sub_type));
    json_append_member(e, "unrand", json_mkbool(unrand));
    if (item.base_type == OBJ_WEAPONS)
    {
        json_append_member(e, "skill",
                           _sp_str(skill_name_j(item_attack_skill(item))));
        json_append_member(e, "dam", json_mknumber(property(item, PWPN_DAMAGE)));
        json_append_member(e, "hit", json_mknumber(property(item, PWPN_HIT)));
        json_append_member(e, "delay", json_mknumber(property(item, PWPN_SPEED)));
    }
    else if (item.base_type == OBJ_ARMOUR)
    {
        json_append_member(e, "ac", json_mknumber(property(item, PARM_AC)));
        json_append_member(e, "evp", json_mknumber(property(item, PARM_EVASION)));
    }
    return e;
}

static JsonNode *_sp_lookup()
{
    JsonNode *all = json_mkobject();
    for (const LookupType &lt : lookup_types)
    {
        JsonNode *arr = json_mkarray();
        // 空の正規表現は何にも一致せず、1 文字だと記号での検索になるので ".*"
        vector<string> keys = lt.matching_keys(".*");
        sort(keys.begin(), keys.end());
        keys.erase(unique(keys.begin(), keys.end()), keys.end());
        for (const string &key : keys)
        {
            JsonNode *e = nullptr;
            const monster_type mc = lt.type == "monster" ? _mon_by_name(key)
                                                         : MONS_NO_MONSTER;
            if (lt.type == "monster"
                && !mons_is_ghost_demon(mc) && !mons_class_is_zombified(mc))
            {
                e = _sp_monster_entry(key);
            }
            else if (lt.type == "spell")
                e = _sp_spell_entry(key, lt.suffix());
            else if (lt.type == "item")
                e = _sp_item_entry(key);
            else if (lt.type == "god")
            {
                const god_type god = str_to_god(key);
                e = _sp_entry(key, god_name_j(god, true), "", "",
                              tileidx_feature_base(altar_for_god(god)));
                json_append_member(e, "name_ja", _sp_str(god_name_j(god)));
                JsonNode *tabs = json_mkarray();
                for (const string &tab : god_spoiler_tabs(god))
                    json_append_element(tabs, _sp_str(tab));
                json_append_member(e, "tabs", tabs);
            }
            else
            {
                spoiler_capture cap;
                if (_sp_capture(lt, key, cap))
                    e = _sp_entry(key, cap.title, cap.body, cap.quote, cap.tile);
            }
            if (e)
                json_append_element(arr, e);
            else
            {
                fprintf(stderr, "spoiler: skipped %s '%s'\n", lt.type.c_str(),
                        key.c_str());
            }
        }
        json_append_member(all, lt.type.c_str(), arr);
    }
    return all;
}

static JsonNode *_sp_skills()
{
    JsonNode *arr = json_mkarray();
    for (skill_type sk = SK_FIRST_SKILL; sk < NUM_SKILLS; ++sk)
    {
        if (is_removed_skill(sk))
            continue;
        JsonNode *e = json_mkobject();
        json_append_member(e, "en", _sp_str(skill_name(sk)));
        json_append_member(e, "ja", _sp_str(skill_name_j(sk)));
        json_append_element(arr, e);
    }
    return arr;
}

static JsonNode *_sp_species()
{
    JsonNode *arr = json_mkarray();
    for (species_type sp : species::get_all_species())
    {
        if (!species::is_starting_species(sp))
            continue;
        const species_def &sd = get_species_def(sp);
        JsonNode *e = json_mkobject();
        json_append_member(e, "abbrev", _sp_str(sd.abbrev));
        json_append_member(e, "name_en", _sp_str(species::name(sp)));
        json_append_member(e, "name_ja", _sp_str(species_name_j(sp)));
        json_append_member(e, "tile",
                           _sp_tile(tileidx_monster_base(sd.monster_species, 0)));
        // desc はキャラクター作成の画面の説明、lore は検索で出る由来の文
        json_append_member(e, "desc",
                           _sp_str(getGameStartDescription(species::name(sp))));
        json_append_member(e, "lore",
                           _sp_str(getLongDescription(species::name(sp))));
        json_append_member(e, "quote",
                           _sp_str(getQuoteString(species::name(sp))));
        json_append_member(e, "xp", json_mknumber(sd.xp_mod));
        json_append_member(e, "hp", json_mknumber(sd.hp_mod));
        json_append_member(e, "mp", json_mknumber(sd.mp_mod));
        json_append_member(e, "wl", json_mknumber(sd.wl_mod));
        json_append_member(e, "str", json_mknumber(sd.s));
        json_append_member(e, "int", json_mknumber(sd.i));
        json_append_member(e, "dex", json_mknumber(sd.d));
        json_append_member(e, "how_often", json_mknumber(sd.how_often));
        JsonNode *lstats = json_mkarray();
        for (stat_type st : sd.level_stats)
            json_append_element(lstats, json_mknumber(st));
        json_append_member(e, "level_stats", lstats);
        json_append_member(e, "size", _sp_str(tagged_jtrans("[size]",
                                              get_size_adj(sd.size))));

        JsonNode *apt = json_mkobject();
        for (skill_type sk = SK_FIRST_SKILL; sk < NUM_SKILLS; ++sk)
        {
            if (is_removed_skill(sk))
                continue;
            const int a = species_apt(sk, sp);
            json_append_member(apt, skill_name(sk),
                               a == UNUSABLE_SKILL ? json_mknull()
                                                   : json_mknumber(a));
        }
        json_append_member(e, "apt", apt);

        JsonNode *muts = json_mkarray();
        for (const level_up_mutation &lum : sd.level_up_mutations)
        {
            JsonNode *m = json_mkobject();
            json_append_member(m, "xl", json_mknumber(lum.xp_level));
            json_append_member(m, "name", _sp_str(mutation_name(lum.mut)));
            json_append_member(m, "level", json_mknumber(lum.mut_level));
            json_append_member(m, "desc",
                               _sp_str(mutation_desc(lum.mut, lum.mut_level)));
            json_append_element(muts, m);
        }
        json_append_member(e, "muts", muts);

        JsonNode *fake = json_mkarray();
        for (const string &s : sd.verbose_fake_mutations)
            json_append_element(fake, _sp_str(jtrans(s)));
        json_append_member(e, "fake_muts", fake);

        JsonNode *jobs = json_mkarray();
        for (job_type j : sd.recommended_jobs)
            json_append_element(jobs, _sp_str(job_name_j(j)));
        json_append_member(e, "jobs", jobs);
        json_append_element(arr, e);
    }
    return arr;
}

static JsonNode *_sp_jobs()
{
    JsonNode *arr = json_mkarray();
    for (int ji = 0; ji < NUM_JOBS; ++ji)
    {
        const job_type j = static_cast<job_type>(ji);
        if (!is_starting_job(j))
            continue;
        const job_def &jd = get_job_def(j);
        JsonNode *e = json_mkobject();
        json_append_member(e, "abbrev", _sp_str(jd.abbrev));
        json_append_member(e, "name_en", _sp_str(jd.name));
        json_append_member(e, "name_ja", _sp_str(job_name_j(j)));
        json_append_member(e, "desc", _sp_str(getGameStartDescription(jd.name)));
        json_append_member(e, "quote", _sp_str(getQuoteString(jd.name)));
        json_append_member(e, "str", json_mknumber(jd.s));
        json_append_member(e, "int", json_mknumber(jd.i));
        json_append_member(e, "dex", json_mknumber(jd.d));
        json_append_member(e, "wchoice", json_mknumber(static_cast<int>(jd.wchoice)));
        JsonNode *eq = json_mkarray();
        for (const string &s : jd.equipment)
            json_append_element(eq, _sp_str(s));
        json_append_member(e, "equipment", eq);
        JsonNode *sk = json_mkarray();
        for (const auto &p : jd.skills)
        {
            JsonNode *s = json_mkobject();
            // SK_WEAPON は「始めに選ぶ武器の技能」を表す仮の値で、名前は無い
            if (p.first == SK_WEAPON)
                json_append_member(s, "skill", _sp_str("(weapon)"));
            else
            {
                json_append_member(s, "skill", _sp_str(skill_name(p.first)));
                json_append_member(s, "ja", _sp_str(skill_name_j(p.first)));
            }
            json_append_member(s, "level", json_mknumber(p.second));
            json_append_element(sk, s);
        }
        json_append_member(e, "skills", sk);
        JsonNode *lib = json_mkarray();
        for (spell_type s : jd.library)
            json_append_element(lib, _sp_str(spell_title_j(s)));
        json_append_member(e, "spells", lib);
        JsonNode *spec = json_mkarray();
        for (species_type s : jd.recommended_species)
            json_append_element(spec, _sp_str(species_name_j(s)));
        json_append_member(e, "species", spec);
        json_append_element(arr, e);
    }
    return arr;
}

static JsonNode *_sp_books()
{
    JsonNode *arr = json_mkarray();
    for (int b = 0; b < NUM_FIXED_BOOKS; ++b)
    {
        if (item_type_removed(OBJ_BOOKS, b))
            continue;
        item_def book;
        book.base_type = OBJ_BOOKS;
        book.sub_type = b;
        book.quantity = 1;
        JsonNode *e = json_mkobject();
        json_append_member(e, "name_ja",
                           _sp_str(book.name(DESC_PLAIN, false, true)));
        JsonNode *sp = json_mkarray();
        for (spell_type s : spellbook_template(static_cast<book_type>(b)))
            json_append_element(sp, _sp_str(spell_title(s)));
        json_append_member(e, "spells", sp);
        json_append_element(arr, e);
    }
    return arr;
}

static JsonNode *_sp_branches()
{
    JsonNode *arr = json_mkarray();
    for (branch_iterator it; it; ++it)
    {
        JsonNode *e = json_mkobject();
        json_append_member(e, "id", json_mknumber(it->id));
        json_append_member(e, "short_en", _sp_str(it->shortname));
        json_append_member(e, "short_ja", _sp_str(branch_name_j(it->shortname)));
        json_append_member(e, "long_ja", _sp_str(branch_name_j(it->longname)));
        json_append_member(e, "abbrev", _sp_str(it->abbrevname));
        json_append_member(e, "parent", json_mknumber(it->parent_branch));
        json_append_member(e, "mindepth", json_mknumber(it->mindepth));
        json_append_member(e, "maxdepth", json_mknumber(it->maxdepth));
        json_append_member(e, "levels", json_mknumber(it->numlevels));
        json_append_member(e, "absdepth", json_mknumber(it->absdepth));
        json_append_member(e, "runes", json_mknumber(it->runes.size()));

        JsonNode *pops = json_mkarray();
        for (int kind = 0; kind < 3; ++kind)
            for (const pop_entry &pe : branch_population(it->id, kind))
            {
                JsonNode *p = json_mkarray();
                json_append_element(p, _sp_str(mons_type_name(pe.value,
                                                              DESC_PLAIN)));
                json_append_element(p, json_mknumber(pe.minr));
                json_append_element(p, json_mknumber(pe.maxr));
                json_append_element(p, json_mknumber(pe.rarity));
                json_append_element(p, json_mknumber(pe.distrib));
                json_append_element(p, json_mknumber(kind));
                json_append_element(pops, p);
            }
        json_append_member(e, "population", pops);
        json_append_element(arr, e);
    }
    return arr;
}

void write_spoiler_json(const string &filename)
{
    JsonWrapper json(json_mkobject());
    json_append_member(json.node, "version", _sp_str(Version::Long));
    json_append_member(json.node, "lookup", _sp_lookup());
    json_append_member(json.node, "skills", _sp_skills());
    json_append_member(json.node, "species", _sp_species());
    json_append_member(json.node, "jobs", _sp_jobs());
    json_append_member(json.node, "books", _sp_books());
    json_append_member(json.node, "branches", _sp_branches());

    FILE *f = fopen_u(filename.c_str(), "wb");
    if (!f)
        end(1, true, "Cannot write %s", filename.c_str());
    const string s = json.to_string();
    fwrite(s.data(), 1, s.size(), f);
    fclose(f);
}
