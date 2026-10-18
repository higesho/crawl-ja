/**
 * @file
 * @brief Notetaking stuff
**/

#include "AppHdr.h"

#include "notes.h"

#include <iomanip>
#include <sstream>
#include <vector>

#include "branch.h"
#include "database.h"
#include "english.h"
#include "hiscores.h"
#include "item-prop.h" // gem_adj
#include "japanese.h"
#include "message.h"
#include "mutation.h"
#include "options.h"
#include "religion.h"
#include "skills.h"
#include "spl-util.h"
#include "state.h"
#include "stringutil.h"
#include "tag-version.h"
#include "unicode.h"
#include "view.h"

#define NOTES_VERSION_NUMBER 1002

vector<Note> note_list;
int last_screen_turn = -1;

static bool _is_highest_skill(int skill)
{
    for (int i = 0; i < NUM_SKILLS; ++i)
    {
        if (i == skill)
            continue;
        if (you.skills[i] >= you.skills[skill])
            return false;
    }
    return true;
}

static bool _is_noteworthy_hp(int hp, int maxhp)
{
    return hp > 0 && Options.note_hp_percent
           && hp <= (maxhp * Options.note_hp_percent) / 100;
}

static int _dungeon_branch_depth(uint8_t branch)
{
    if (branch >= NUM_BRANCHES)
        return -1;
    return brdepth[branch];
}

static bool _is_noteworthy_dlevel(level_id place)
{
    branch_type branch = place.branch;
    int lev = place.depth;

    // Entering the Abyss is noted a different way, since we care mostly about
    // the cause.
    if (branch == BRANCH_ABYSS)
        return lev == _dungeon_branch_depth(branch);

    // These get their note in the .des files.
    if (branch == BRANCH_WIZLAB || branch == BRANCH_TROVE)
        return false;

    // Other portal levels are always interesting.
    if (!is_connected_branch(branch))
        return true;

    return lev == _dungeon_branch_depth(branch)
           || branch == BRANCH_DUNGEON && (lev % 5) == 0
           || branch != BRANCH_DUNGEON && lev == 1;
}

// Is a note worth taking?
// This function assumes that game state has not changed since
// the note was taken, e.g. you.* is valid.
static bool _is_noteworthy(const Note& note)
{
    // Always noteworthy.
    if (note.type == NOTE_XP_LEVEL_CHANGE
        || note.type == NOTE_LEARN_SPELL
        || note.type == NOTE_GET_GOD
        || note.type == NOTE_GOD_GIFT
        || note.type == NOTE_GET_MUTATION
        || note.type == NOTE_LOSE_MUTATION
        || note.type == NOTE_PERM_MUTATION
        || note.type == NOTE_GET_ITEM
        || note.type == NOTE_ID_ITEM
        || note.type == NOTE_ACQUIRE_ITEM
        || note.type == NOTE_BUY_ITEM
        || note.type == NOTE_DONATE_MONEY
        || note.type == NOTE_SEEN_MONSTER
        || note.type == NOTE_DEFEAT_MONSTER
        || note.type == NOTE_POLY_MONSTER
        || note.type == NOTE_USER_NOTE
        || note.type == NOTE_MESSAGE
        || note.type == NOTE_LOSE_GOD
        || note.type == NOTE_PENANCE
        || note.type == NOTE_MOLLIFY_GOD
        || note.type == NOTE_DEATH
        || note.type == NOTE_XOM_REVIVAL
        || note.type == NOTE_SEEN_FEAT
        || note.type == NOTE_PARALYSIS
        || note.type == NOTE_VEXED
        || note.type == NOTE_RECRUITED_APOSTLE
        || note.type == NOTE_ALLY_DEATH
        || note.type == NOTE_FEAT_MIMIC
        || note.type == NOTE_OFFERED_SPELL
        || note.type == NOTE_ANCESTOR_TYPE
        || note.type == NOTE_FOUND_UNRAND
        || note.type == NOTE_ZOT_TOUCHED
        || note.type == NOTE_DREAMSHARD
        || note.type == NOTE_GEM_LOST
        || note.type == NOTE_GAIN_LIFE
        || note.type == NOTE_LOSE_LIFE
        || note.type == NOTE_FLED_CHALLENGE
        || note.type == NOTE_INFERNAL_MARK
        || note.type == NOTE_GET_BANE
        || note.type == NOTE_LOSE_BANE
        || note.type == NOTE_TESSERACT_ACTIVATED)
    {
        return true;
    }

    // Never noteworthy, hooked up for fun or future use.
    if (note.type == NOTE_MP_CHANGE
        || note.type == NOTE_MAXHP_CHANGE
        || note.type == NOTE_MAXMP_CHANGE)
    {
        return false;
    }

    // Xom effects are only noteworthy if the option is true.
    if (note.type == NOTE_XOM_EFFECT)
        return Options.note_xom_effects;

    // HP noteworthiness is handled in its own function.
    if (note.type == NOTE_HP_CHANGE
        && !_is_noteworthy_hp(note.first, note.second))
    {
        return false;
    }

    // Skills are always noteworthy in order to construct the skill_gains table
    // in the chardump. The options to control display are used in
    // Note::hidden().
    if (note.type == NOTE_GAIN_SKILL || note.type == NOTE_LOSE_SKILL)
        return true;

    if (note.type == NOTE_DUNGEON_LEVEL_CHANGE)
        return _is_noteworthy_dlevel(note.place);

    for (const Note &oldnote : note_list)
    {
        if (oldnote.type != note.type)
            continue;

        const Note& rnote(oldnote);
        switch (note.type)
        {
        case NOTE_PIETY_RANK:
            if (rnote.first == note.first && rnote.second == note.second)
                return false;
            break;

        case NOTE_HP_CHANGE:
            // Not if we have a recent warning
            // unless we've lost half our HP since then.
            if (note.turn - rnote.turn < 5
                && note.first * 2 >= rnote.first)
            {
                return false;
            }
            break;

        default:
            mpr(jtrans("Buggy note passed: unknown note type"));
            // Return now, rather than give a "Buggy note passed" message
            // for each note of the matching type in the note list.
            return true;
        }
    }
    return true;
}

// ノートの名前・説明(name, desc)は、作った側によって日本語のことも英語の
// こともある(セーブに残る)。英語の部分は表示するときに辞書で訳す
// (death_name_j() は日本語をそのまま返す)。
static string _note_name_j(const string &name)
{
    return death_name_j(name);
}

string Note::describe(bool when, bool where, bool what) const
{
    ostringstream result;

    if (when)
        result << setw(6) << turn << " ";

    if (where)
    {
        result << "| "
               << chop_string(place.describe_j(), MAX_NOTE_PLACE_LEN)
               << " | ";
    }

    if (what)
    {
        const char *nm = name.c_str();
        switch (type)
        {
        case NOTE_HP_CHANGE:
            // [ds] Shortened HP change note from "Had X hitpoints" to
            // accommodate the cause for the loss of hitpoints.
            result << "HP: " << first << "/" << second
                   << " [" << name << "]";
            break;
        case NOTE_XOM_REVIVAL:
            result << jtrans("Xom revived you");
            break;
        case NOTE_MP_CHANGE:
            result << make_stringf(jtransc("Magic: %d/%d"), first, second);
            break;
        case NOTE_MAXHP_CHANGE:
            result << make_stringf(jtransc("Reached %d max health"), first);
            break;
        case NOTE_MAXMP_CHANGE:
            result << make_stringf(jtransc("Reached %d max magic points"), first);
            break;
        case NOTE_XP_LEVEL_CHANGE:
            result << make_stringf(jtransc("Reached XP level %d. %s"), first, nm);
            break;
        case NOTE_DUNGEON_LEVEL_CHANGE:
            if (!desc.empty())
                result << jtrans(desc);
            else
            {
                result << make_stringf(jtransc("Entered %s"),
                                       place.describe_j(true, true).c_str());
            }
            break;
        case NOTE_LEARN_SPELL:
            // 引数: 呪文のレベル, 呪文
            result << make_stringf(jtransc("Learned a level %d spell: %s"),
                        spell_difficulty(static_cast<spell_type>(first)),
                        spell_title_jc(static_cast<spell_type>(first)));
            break;
        case NOTE_GET_GOD:
            result << make_stringf(jtransc("Became a worshipper of %s"),
                        god_name_j(static_cast<god_type>(first), true).c_str());
            break;
        case NOTE_LOSE_GOD:
            result << make_stringf(jtransc("Fell from the grace of %s"),
                        god_name_jc(static_cast<god_type>(first)));
            break;
        case NOTE_PENANCE:
            result << make_stringf(jtransc("Was placed under penance by %s"),
                        god_name_jc(static_cast<god_type>(first)));
            break;
        case NOTE_MOLLIFY_GOD:
            result << make_stringf(jtransc("Was forgiven by %s"),
                        god_name_jc(static_cast<god_type>(first)));
            break;
        case NOTE_GOD_GIFT:
            result << make_stringf(jtransc("Received a gift from %s"),
                        god_name_jc(static_cast<god_type>(first)));
            if (!name.empty())
                result << " (" << jtrans(name) << ")";
            break;
        case NOTE_ID_ITEM:
            result << make_stringf(jtransc("Identified %s"), nm);
            if (!desc.empty())
                result << " (" << desc << ")";
            break;
        case NOTE_GET_ITEM:
            result << make_stringf(jtransc("Got %s"), nm);
            if (first != 0) // gems
            {
                const int turns = (first + 9) / 10;
                result << make_stringf(jtransc("(with %d turns to spare)"),
                                       turns);
            }
            break;
        case NOTE_ACQUIRE_ITEM:
            result << make_stringf(jtransc("Acquired %s"), nm);
            break;
        case NOTE_BUY_ITEM:
            result << make_stringf(jtransc("Bought %s for %d gold pieces"),
                                   nm, first);
            break;
        case NOTE_DONATE_MONEY:
            result << make_stringf(jtransc("Donated %d gold pieces to Zin"),
                                   first);
            break;
        case NOTE_GAIN_SKILL:
            result << make_stringf(jtransc("Skill %s reached level %d"),
                        skill_name_jc(static_cast<skill_type>(first)), second);
            break;
        case NOTE_LOSE_SKILL:
            result << make_stringf(jtransc("Reduced skill %s to level %d"),
                        skill_name_jc(static_cast<skill_type>(first)), second);
            break;
        case NOTE_SEEN_MONSTER:
            result << make_stringf(jtransc("Encountered %s"),
                                   _note_name_j(name).c_str());
            break;
        case NOTE_DEFEAT_MONSTER:
        {
            // desc は英語の動詞("killed" など。mon-death.cc)
            const string verb = tagged_jtrans(second ? "[kill verb passive]"
                                                     : "[kill verb]", desc);
            const string who = _note_name_j(name);
            if (second)
            {
                result << make_stringf(jtransc("%s (ally) was %s"),
                                       who.c_str(), verb.c_str());
            }
            else
            {
                // 「〜を倒した」
                result << make_stringf(tagged_jtransc("[note]", "%s %s"),
                                       who.c_str(), verb.c_str());
            }
            break;
        }
        case NOTE_POLY_MONSTER:
            result << make_stringf(jtransc("%s changed into %s"),
                                   _note_name_j(name).c_str(),
                                   _note_name_j(desc).c_str());
            break;
        case NOTE_PIETY_RANK:
            // 引数: 神, ★(日本語の語順)
            result << make_stringf(jtransc("Reached %s piety under %s"),
                        god_name_jc(static_cast<god_type>(first)),
                        string(second, '*').c_str());
            break;
        case NOTE_GET_MUTATION:
            result << jtrans("Gained mutation:") << " "
                   << mutation_desc(static_cast<mutation_type>(first),
                                    second == 0 ? 1 : second);
            if (!name.empty())
                result << " [" << _note_name_j(name) << "]";
            break;
        case NOTE_LOSE_MUTATION:
            result << jtrans("Lost mutation:") << " "
                   << mutation_desc(static_cast<mutation_type>(first),
                                    second == 3 ? 3 : second+1);
            if (!name.empty())
                result << " [" << _note_name_j(name) << "]";
            break;
        case NOTE_PERM_MUTATION:
            result << jtrans("Mutation became permanent:") << " "
                   << mutation_desc(static_cast<mutation_type>(first),
                                    second == 0 ? 1 : second);
            if (!name.empty())
                result << " [" << _note_name_j(name) << "]";
            break;
        case NOTE_GET_BANE:
            result << jtrans("Gained bane:") << " "
                   << bane_name(static_cast<bane_type>(first));
            if (!name.empty())
                result << " [" << _note_name_j(name) << "]";
            break;
        case NOTE_LOSE_BANE:
            result << jtrans("Lost bane:") << " "
                   << bane_name(static_cast<bane_type>(first));
            break;
        case NOTE_DEATH:
            result << name;
            break;
        case NOTE_USER_NOTE:
            result << Options.user_note_prefix << name;
            break;
        case NOTE_MESSAGE:
            result << jtrans(name);
            break;
        case NOTE_SEEN_FEAT:
            result << make_stringf(jtransc("Found %s"), nm);
            break;
        case NOTE_FEAT_MIMIC:
            result << make_stringf(jtransc("%s was a mimic."), nm);
            break;
        case NOTE_XOM_EFFECT:
            result << "XOM: " << jtrans(name);
#if defined(DEBUG_XOM) || defined(NOTE_DEBUG_XOM)
            // If debugging, also take note of piety and tension.
            result << " (piety: " << first;
            if (second >= 0)
                result << ", tension: " << second;
            result << ")";
#endif
            break;
        case NOTE_PARALYSIS:
        {
            const float turns = first / 10.0;
            ostringstream t;
            t << setprecision(2) << turns;
            result << make_stringf(jtransc("Paralysed by %s for %s turns"),
                                   _note_name_j(name).c_str(), t.str().c_str());
        }
            break;
        case NOTE_VEXED:
            result << make_stringf(jtransc("Vexed by %s for %d turns"),
                                   _note_name_j(name).c_str(), first);
            break;
        case NOTE_RECRUITED_APOSTLE:
            result << make_stringf(jtransc("Anointed %s the %s as your apostle"),
                                   nm, desc.c_str());
            break;
        case NOTE_ALLY_DEATH:
            result << make_stringf(jtransc("Your ally %s died"), nm);
            break;
        case NOTE_OFFERED_SPELL:
            result << make_stringf(jtransc("Offered knowledge of %s by Vehumet."),
                        spell_title_jc(static_cast<spell_type>(first)));
            break;
        case NOTE_ANCESTOR_TYPE:
            result << make_stringf(jtransc("Remembered your ancestor %s as %s"),
                                   hepliaklqana_ally_name().c_str(), nm);
            break;
#if TAG_MAJOR_VERSION == 34
        case NOTE_ANCESTOR_SPECIALIZATION:
            result << make_stringf(jtransc("Remembered your ancestor %s %s"),
                                   hepliaklqana_ally_name().c_str(),
                                   jtrans(name).c_str());
            break;
        case NOTE_ANCESTOR_DEATH:
            result << make_stringf(jtransc("Remembered your ancestor %s's %s death"),
                                   hepliaklqana_ally_name().c_str(),
                                   jtrans(name).c_str());
            break;
#endif
          case NOTE_FOUND_UNRAND:
            result << make_stringf(jtransc("Found %s"), nm);
            break;
        case NOTE_ZOT_TOUCHED:
            result << make_stringf(jtransc("Touched by the power of Zot (MHP %d -> %d)"),
                                   first, second);
            break;
        case NOTE_DREAMSHARD:
            result << jtrans("Saved by the dreamshard amulet");
            break;
        case NOTE_GEM_LOST:
            result << make_stringf(jtransc("Lost the %s through the power of Zot."),
                        jtrans(string(gem_adj(static_cast<gem_type>(first)))
                               + " gem").c_str());
            break;
        case NOTE_GAIN_LIFE:
            result << make_stringf(jtransc("Gained a life (%d lives remaining)"),
                                   first);
            break;
        case NOTE_LOSE_LIFE:
            result << make_stringf(jtransc("Lost a life (%d lives remaining)"),
                                   first);
            break;
        case NOTE_FLED_CHALLENGE:
            result << jtrans("Fled from a divine trial");
            break;
        case NOTE_INFERNAL_MARK:
            result << make_stringf(jtransc("Branded self with the %s"),
                                   jtrans(name).c_str());
            break;
        case NOTE_TESSERACT_ACTIVATED:
            result << jtrans("Activated a boundless tesseract");
            break;
        default:
            result << jtrans("Buggy note description: unknown note type");
            break;
        }
    }

    if (type == NOTE_SEEN_MONSTER || type == NOTE_DEFEAT_MONSTER)
    {
        if (what && first == MONS_PANDEMONIUM_LORD)
            result << jtrans("(the pandemonium lord)");
    }
    return result.str();
}

bool Note::hidden() const
{
    // Hide skill gains that are not enabled by options.
    if (type == NOTE_GAIN_SKILL || type == NOTE_LOSE_SKILL)
    {
        return !(Options.note_all_skill_levels
                 || second <= 27 && Options.note_skill_levels[second]
                 || Options.note_skill_max && _is_highest_skill(first));
    }
    // Hide gems being shattered by default.
    if (type == NOTE_GEM_LOST)
        return !Options.more_gem_info;
    return false;
}

void Note::check_milestone() const
{
    if (crawl_state.game_is_arena())
        return;

    if (type == NOTE_DUNGEON_LEVEL_CHANGE)
    {
        const int br = place.branch,
                 dep = place.depth;

        // Wizlabs report their milestones on their own.
        if (br != -1 && br != BRANCH_WIZLAB)
        {
            ASSERT_RANGE(br, 0, NUM_BRANCHES);
            string branch = place.describe(true, false);

            if (starts_with(branch, "The "))
                branch[0] = tolower_safe(branch[0]);

            if (dep == 1)
            {
                mark_milestone(br == BRANCH_ZIGGURAT ? "zig.enter" : "br.enter",
                               "entered " + branch + ".", "parent");
            }
            else if (dep == _dungeon_branch_depth(br)
                     || br == BRANCH_ZIGGURAT)
            {
                string level = place.describe(true, true);
                if (starts_with(level, "Level "))
                    level[0] = tolower_safe(level[0]);

                mark_milestone(br == BRANCH_ZIGGURAT ? "zig" : "br.end",
                               "reached " + level + ".");
            }
        }
    }
}

void Note::save(writer& outf) const
{
    marshallInt(outf, type);
    marshallInt(outf, turn);
    place.save(outf);
    marshallInt(outf, first);
    marshallInt(outf, second);
    marshallString4(outf, name);
    marshallString4(outf, desc);
    marshallString(outf, screen);
}

void Note::load(reader& inf)
{
    type = static_cast<NOTE_TYPES>(unmarshallInt(inf));
    turn = unmarshallInt(inf);
#if TAG_MAJOR_VERSION == 34
    if (inf.getMinorVersion() < TAG_MINOR_PLACE_UNPACK)
        place = level_id::from_packed_place(unmarshallShort(inf));
    else
#endif
    place.load(inf);
    first  = unmarshallInt(inf);
    second = unmarshallInt(inf);
    unmarshallString4(inf, name);
    unmarshallString4(inf, desc);
#if TAG_MAJOR_VERSION == 34
    if (inf.getMinorVersion() >= TAG_MINOR_MORGUE_SCREENSHOTS)
        screen = unmarshallString(inf);
#else
    screen = unmarshallString(inf);
#endif

}

static bool notes_active = false;

bool notes_are_active()
{
    return notes_active;
}

void take_note(const Note& note, bool force)
{
    if (notes_active && (force || _is_noteworthy(note)))
    {
        note_list.push_back(note);
        note.check_milestone();
    }
}

void activate_notes(bool active)
{
    notes_active = active;
}

void save_notes(writer& outf)
{
    marshallInt(outf, NOTES_VERSION_NUMBER);
    marshallInt(outf, note_list.size());
    for (const Note &note : note_list)
        note.save(outf);
}

void load_notes(reader& inf)
{
    if (unmarshallInt(inf) != NOTES_VERSION_NUMBER)
        return;

    const int num_notes = unmarshallInt(inf);
    for (int i = 0; i < num_notes; ++i)
    {
        Note new_note;
        new_note.load(inf);
        note_list.push_back(new_note);
    }
}

void make_user_note()
{
    char buf[400];
    bool validline = !msgwin_get_line(jtrans_notrim("Enter note: "), buf,
                                      sizeof(buf));
    if (!validline || (!*buf))
        return;
    Note unote(NOTE_USER_NOTE);
    unote.name = buf;
    // Only one screenshot a turn allowed
    if (last_screen_turn != unote.turn)
    {
        last_screen_turn = unote.turn;
        unote.screen = screenshot();
    }
    take_note(unote);
}
