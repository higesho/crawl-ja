/**
 * @file
 * @brief Functions used to print information about gods.
 **/

#include "AppHdr.h"

#include "describe-god.h"

#include <iomanip>

#include "act-iter.h"
#include "ability.h"
#include "branch.h"
#include "cio.h"
#include "database.h"
#include "decks.h"
#include "describe.h"
#include "english.h"
#include "env.h"
#include "god-abil.h"
#include "god-companions.h"
#include "god-conduct.h"
#include "god-passive.h"
#include "god-type.h"
#include "items.h"
#include "item-name.h"
#include "japanese.h"
#include "libutil.h"
#include "menu.h"
#include "message.h"
#include "religion.h"
#include "skills.h"
#include "spl-util.h"
#include "stringutil.h"
#include "tag-version.h"
#include "terrain.h"
#include "tilepick.h"
#include "unicode.h"
#include "xom.h"

using namespace ui;

static int _piety_level(int piety)
{
    return (piety >= piety_breakpoint(5)) ? 7 :
           (piety >= piety_breakpoint(4)) ? 6 :
           (piety >= piety_breakpoint(3)) ? 5 :
           (piety >= piety_breakpoint(2)) ? 4 :
           (piety >= piety_breakpoint(1)) ? 3 :
           (piety >= piety_breakpoint(0)) ? 2 :
           (piety >                    0) ? 1
                                          : 0;
}

static int _gold_level()
{
    return (you.gold >= 50000) ? 7 :
           (you.gold >= 10000) ? 6 :
           (you.gold >=  5000) ? 5 :
           (you.gold >=  1000) ? 4 :
           (you.gold >=   500) ? 3 :
           (you.gold >=   100) ? 2 : 1;
}

static int _invocations_level()
{
    int invo = you.skills[SK_INVOCATIONS];
    return (invo == 27) ? 7 :
           (invo >= 24) ? 6 :
           (invo >= 20) ? 5 :
           (invo >= 16) ? 4 :
           (invo >= 12) ? 3 :
           (invo >= 8)  ? 2
                        : 1;
}

int god_favour_rank(god_type which_god)
{
    if (which_god == GOD_GOZAG)
        return _gold_level();
    else if (which_god == GOD_USKAYAW)
        return _invocations_level();
    else
        return _piety_level(you.raw_piety);
}

static string _describe_favour(god_type which_god)
{
    if (player_under_penance())
    {
        const int penance = you.penance[which_god];
        return jtrans((penance >= 50) ? "Godly wrath is upon you!" :
                      (penance >= 20) ? "You've transgressed heavily! Be penitent!" :
                      (penance >=  5) ? "You are under penance."
                                      : "You should show more discipline.");
    }

    // 訳は jtrans/xom.txt("a plaything of Xom." など)
    if (which_god == GOD_XOM)
        return jtrans(describe_xom_favour());


    const string godname = god_name_j(which_god);
    switch (god_favour_rank(which_god))
    {
        case 7:  return make_stringf(jtransc("A prized avatar of %s."), godname.c_str());
        case 6:  return make_stringf(jtransc("A favoured servant of %s."), godname.c_str());
        case 5:

            if (you_worship(GOD_DITHMENOS))
                return make_stringf(jtransc("A glorious shadow in the eyes of %s."), godname.c_str());
            else
                return make_stringf(jtransc("A shining star in the eyes of %s."), godname.c_str());

        case 4:

            if (you_worship(GOD_DITHMENOS))
                return make_stringf(jtransc("A rising shadow in the eyes of %s."), godname.c_str());
            else
                return make_stringf(jtransc("A rising star in the eyes of %s."), godname.c_str());

        case 3:  return make_stringf(jtransc("%s is pleased with you."), godname.c_str());
        case 2:  return make_stringf(jtransc("%s is aware of your devotion."), godname.c_str());
        default: return make_stringf(jtransc("%s is noncommittal."), godname.c_str());
    }
}

// The various titles granted by the god of your choice. Note that Xom
// doesn't use piety the same way as the other gods, so these are just
// placeholders.
static const char *divine_title[][8] =
{
    // No god.
    {"Buglet",             "Firebug",               "Bogeybug",                 "Bugger",
        "Bugbear",            "Bugged One",            "Giant Bug",                "Lord of the Bugs"},

    // Zin.
    {"Blasphemer",         "Anchorite",             "Apologist",                "Pious",
        "Devout",             "Orthodox",              "Immaculate",               "Bringer of Law"},

    // The Shining One.
    {"Honourless",         "Acolyte",               "Righteous",                "Unflinching",
        "Holy Warrior",       "Exorcist",              "Demon Slayer",             "Bringer of Light"},

    // Kikubaaqudgha -- death scholar theme.
    {"Tormented",          "Purveyor of Pain",       "Pupil of Sorrows",        "Merchant of Misery",
     "Scholar of Souls",   "Artisan of Death",       "Demagogue of Despair",    "Lord of Darkness"},

    // Yredelemnul -- fervent death knight theme.
    {"Traitor",            "Torchbearer",            "Despoiler",               "Black Crusader",
     "Fallen @Genus@",     "Harbinger of Doom",      "Inexorable Tide",         "Bringer of Blasphemy"},

    // Xom.
    {"Toy",                "Toy",                   "Toy",                      "Toy",
        "Toy",                "Toy",                   "Toy",                      "Toy"},

    // Vehumet -- battle mage theme.
    {"Meek",               "Sorcerer's Apprentice", "Scholar of Destruction",   "Caster of Ruination",
        "Traumaturge",        "Battlemage",            "Warlock",                  "Luminary of Lethal Lore"},

    // Okawaru -- battle theme.
    {"Coward",             "Struggler",             "Combatant",                "@Genus@-At-Arms",
        "Knight",             "Myrmidon",             "Warmonger",                "Victor of a Thousand Battles"},

    // Makhleb -- chaos theme.
    {"Orderly",            "Spawn of Chaos",        "Disciple of Destruction",  "Fanfare of Bloodshed",
        "Fiendish",           "Demolition @Genus@",    "Pandemonic",               "Champion of Chaos"},

    // Sif Muna -- generalist scholarly theme.
    {"Ignorant",           "Disciple",              "Student",                  "Adept",
        "Scribe",             "Scholar",               "Sage",                     "Genius of the Arcane"},

    // Trog -- anger theme.
    {"Puny",               "Troglodyte",            "Angry Troglodyte",         "Frenzied",
        "@Genus@ of Prey",    "Rampant",               "Wild @Genus@",             "Bane of Scribes"},

    // Nemelex Xobeh -- alluding to Tarot and cards.
    {"Unlucky @Genus@",    "Pannier",               "Jester",                   "Fortune-Teller",
        "Soothsayer",         "Magus",                 "Cardsharp",                "Hand of Fortune"},

    // Elyvilon.
    {"Sinner",                "Practitioner",       "Comforter",             "Caregiver",
        "Mender",           "Pacifist",                "Purifying @Genus@",        "Bringer of Life"},

    // Lugonu -- distortion theme.
    {"Pure",               "Abyss-Baptised",        "Unweaver",                 "Distorting @Genus@",
        "Agent of Entropy",   "Schismatic",            "Envoy of Void",            "Corrupter of Planes"},

    // Beogh -- messiah theme.
    {"Apostate",           "Convert",               "Proselytiser",             "Priest",
        "Missionary",         "Evangelist",            "Unifier",                  "Messiah"},

    // Jiyva -- slime and jelly theme.
    {"Scum",               "Squelcher",             "Ooze",                     "Jelly",
        "Slime Creature",     "Dissolving @Genus@",    "Blob",                     "Royal Jelly"},

    // Fedhas Madash -- nature theme.
    {"@Walking@ Fertiliser", "Fungal",              "Green @Genus@",            "Cultivator",
        "Fruitful",           "Photosynthesist",       "Green Death",              "Force of Nature"},

    // Cheibriados -- slow theme
    {"Hasty",              "Sluggish @Genus@",      "Deliberate",               "Unhurried",
     "Contemplative",         "Epochal",               "Timeless",                 "@Adj@ Aeon"},

    // Ashenzari -- divination theme
    {"Star-crossed",       "Cursed",                "Initiated",                "Seer",
        "Oracle",            "Illuminatus",            "Prince of Secrets",        "Omniscient"},

    // Dithmenos -- darkness theme
    {"Conspicuous",         "Nocturnal",            "Bump in the Night",        "Thespian",
        "Tenebrous",          "Puppetmaster",          "@Walking@ Midnight",       "Who Hides the Stars"},

    // Gozag -- entrepreneur theme
    {"Profligate",         "Pauper",                "Entrepreneur",             "Capitalist",
        "Rich",               "Opulent",               "Tycoon",                   "Plutocrat"},

    // Qazlal -- natural disaster theme
    {"Unspoiled",          "@Adj@ Mishap",          "Lightning Rod",            "@Adj@ Disaster",
        "Eye of the Storm",   "@Adj@ Catastrophe",     "@Adj@ Cataclysm",          "End of an Era"},

    // Ru -- enlightenment theme
    {"Sleeper",           "Questioner",             "Initiate",                 "Seeker of Truth",
        "@Walker@ of the Path","Lifter of the Veil",     "Transcendent",     "Drop of Water"},

#if TAG_MAJOR_VERSION == 34
    // Pakellas -- inventor theme
    {"Reactionary",       "Apprentice",             "Inquisitive",              "Experimenter",
        "Inventor",           "Pioneer",               "Brilliant",                "Grand Gadgeteer"},
#endif

    // Uskayaw -- reveler theme
    {"Prude",             "Wallflower",             "Party-goer",              "Dancer",
        "Impassioned",        "Rapturous",             "Ecstatic",                "Rhythm of Life and Death"},

    // Hepliaklqana -- memory/ancestry theme
    {"Damnatio Memoriae",       "Hazy",             "@Adj@ @Child@",              "Storyteller",
        "Brooding",           "Anamnesiscian",               "Grand Scion",                "Unforgettable"},

    // Wu Jian -- animal/chinese martial arts monk theme
    {"Wooden Rat",          "Young Dog",             "Young Crane",              "Young Tiger",
        "Young Dragon",     "Red Sash",               "Golden Sash",              "Sifu"},

    // Ignis -- fire/candles theme
    {"Extinguished",          "Last Ember",             "Glowing Coal",              "Thurifer",
        "Hearthfire",     "Furnace",               "Raging Flame",              "Inferno"},
};
COMPILE_CHECK(ARRAYSZ(divine_title) == NUM_GODS);

string god_title(god_type which_god, species_type which_species, int piety)
{
    string title;
    if (player_under_penance(which_god))
        title = divine_title[which_god][0];
    else if (which_god == GOD_USKAYAW)
        title = divine_title[which_god][_invocations_level()];
    else if (which_god == GOD_GOZAG)
        title = divine_title[which_god][_gold_level()];
    else
        title = divine_title[which_god][_piety_level(piety)];

    // 日本語: 称号を [title] で訳し(data-skills.txt)、置換する語も日本語にする
    // (data-species.txt の [spec adj] [spec genus] [walking] [walker] [child])。
    // skills.cc の skill_title() はこの結果をそのまま使う。
    const string walk = species::walking_title(which_species);
    const map<string, string> replacements =
    {
        { "Adj", tagged_jtrans("[spec adj]",
                     species::name(which_species, species::SPNAME_ADJ)) },
        { "Genus", tagged_jtrans("[spec genus]",
                       species::name(which_species, species::SPNAME_GENUS)) },
        { "Walking", tagged_jtrans("[walking]", walk) },
        { "Walker", tagged_jtrans("[walker]", walk) },
        { "Child", tagged_jtrans("[child]", species::child_name(which_species)) },
        { "Orc", jtrans(species::orc_name(which_species)) },
    };

    return replace_keys(tagged_jtrans("[title]", title), replacements);
}

// 表示幅で揃える(setw はバイト数で数えるので日本語では崩れる)
static string _pad_to_width(const string &s, int width)
{
    const int w = strwidth(s);
    return w >= width ? s + " " : s + string(width - w, ' ');
}

static string _describe_item_curse(const item_def& item)
{
    if (!item.props.exists(CURSE_KNOWLEDGE_KEY))
        return jtrans("[curse]None");

    const CrawlVector &curses = item.props[CURSE_KNOWLEDGE_KEY].get_vector();

    if (curses.empty())
        return jtrans("[curse]None");

    // curse_name() は日本語を返す
    return comma_separated_fn(curses.begin(), curses.end(),
            curse_name, "、", "、");
}

static string _describe_ash_skill_boost()
{
    ostringstream desc;
    desc.setf(ios::left);
    desc << "<white>";
    desc << _pad_to_width(jtrans("Bound item"), 40);
    desc << _pad_to_width(jtrans("Curse bonuses"), 30);
    desc << "</white>\n";

    vector<item_def*> eq = you.equipment.get_slot_items(SLOT_ALL_EQUIPMENT, true);
    for (item_def* item : eq)
    {
        if (item->cursed())
        {
            const bool meld = item_is_melded(*item);
            desc << (meld ? "<darkgrey>" : "<lightred>");
            desc << _pad_to_width(item->name(DESC_QUALNAME, true, false, false), 40);
            desc << _pad_to_width(meld ? jtrans("[ash bound]melded")
                                       : _describe_item_curse(*item), 30);
            desc << (meld ? "</darkgrey>" : "</lightred>");
            desc << "\n";
        }
    }

    return desc.str();
}

typedef pair<int, string> ancestor_upgrade;

static const map<monster_type, vector<ancestor_upgrade> > ancestor_data =
{
    { MONS_ANCESTOR_KNIGHT,
      { { 1,  "Flail" },
        { 1,  "Shield" },
        { 1,  "Chain mail (+AC)" },
        { 15, "Broad axe (flame)" },
        { 19, "Tower shield (reflect)" },
        { 19, "Haste" },
        { 24, "Broad axe (speed)" },
      }
    },
    { MONS_ANCESTOR_BATTLEMAGE,
      { { 1,  "Quarterstaff" },
        { 1,  "Throw Frost" },
        { 1,  "Stone Arrow" },
        { 1,  "Increased melee damage" },
        { 15, "Bolt of Magma" },
        { 19, "Lajatang (freeze)" },
        { 19, "Haste" },
        { 24, "Lehudib's Crystal Spear" },
      }
    },
    { MONS_ANCESTOR_HEXER,
      { { 1,  "Dagger (drain)" },
        { 1,  "Slow" },
        { 1,  "Confuse" },
        { 15, "Paralyse" },
        { 19, "Mass Confusion" },
        { 19, "Haste" },
        { 24, "Quick blade (antimagic)" },
      }
    },
};

/// Build & return a table of Hep's upgrades for your chosen ancestor type.
static string _describe_ancestor_upgrades()
{
    if (!you.props.exists(HEPLIAKLQANA_ALLY_TYPE_KEY))
        return "";

    string desc;
    const monster_type ancestor =
        static_cast<monster_type>(you.props[HEPLIAKLQANA_ALLY_TYPE_KEY].get_int());
    const vector<ancestor_upgrade> *upgrades = map_find(ancestor_data,
                                                        ancestor);

    if (upgrades)
    {
        desc = jtrans_notrim("Ancestor Upgrades:\n\n")
               + "<white>" + jtrans_notrim("XL              Upgrade\n")
               + "</white>";
        for (auto &entry : *upgrades)
        {
            // 強化内容の訳は jtrans/describe-god.txt の [ancestor upgrade]
            desc += make_stringf("%s%2d              %s%s\n",
                                 you.experience_level < entry.first
                                     ? "<darkgrey>" : "",
                                 entry.first,
                                 tagged_jtransc("[ancestor upgrade]",
                                                entry.second),
                                 you.experience_level < entry.first
                                     ? "</darkgrey>" : "");
        }
    }

    // XXX: maybe it'd be nice to let you see other ancestor types'...?
    return desc;
}

// from dgn-overview.cc
extern map<branch_type, set<level_id> > stair_level;

/**
 * Populate a provided vector with a list of bribable branches which are known
 * to the player.
 *
 * @param[out] targets      A list of bribable branches.
 */
static void _list_bribable_branches(vector<branch_type> &targets)
{
    for (branch_iterator it; it; ++it)
    {
        const branch_type br = it->id;
        if (!gozag_branch_bribable(br))
            continue;

        // Only list the Hells once.
        if (is_hell_subbranch(br))
            continue;

        // If you don't know the branch exists, don't list it;
        // this mainly plugs info leaks about Lair branch structure.
        if (!stair_level.count(br))
            continue;

        targets.push_back(br);
    }
}

/**
 * Describe the current options for Gozag's bribe branch ability.
 *
 * @return      A description of branches' bribe status.
 */
static string _describe_branch_bribability()
{
    string ret = jtrans_notrim("You can bribe the following branches of the dungeon:\n");
    vector<branch_type> targets;
    _list_bribable_branches(targets);

    int width = 0;
    for (branch_type br : targets)
        width = max(width, strwidth(branch_name_j(branches[br].shortname)));

    for (branch_type br : targets)
    {
        string line = " ";
        line += branch_name_j(branches[br].shortname);
        line += string(max(1, width + 3 - strwidth(line)), ' ');

        if (!branch_bribe[br])
            line += jtrans("not bribed");
        else
            line += make_stringf(jtransc("[bribe]$%d"), branch_bribe[br]);

        ret += line + "\n";
    }

    return ret;
}

static inline void _add_par(formatted_string &desc, const string &str)
{
    if (!str.empty())
        desc += formatted_string::parse_string(trimmed_string(str) + "\n\n");
}

/**
 * Describe the causes of the given god's wrath.
 *
 * @param which_god     The god in question.
 * @return              A description of the actions that cause this god's
 *                      wrath.
 */
static string _describe_god_wrath_causes(god_type which_god)
{
    if (which_god == GOD_RU)
        return ""; // no wrath
    vector<god_type> evil_gods;
    vector<god_type> chaotic_gods;
    for (god_iterator it; it; ++it)
    {
        const god_type god = *it;
        if (is_evil_god(god))
            evil_gods.push_back(god);
        else if (is_chaotic_god(god)) // intentionally not including evil!
            chaotic_gods.push_back(god);
        // XXX: refactor this if any god hates chaotic but not evil gods
    }

    auto god_name_ja = [](god_type god) { return god_name_j(god); };

    switch (which_god)
    {
        case GOD_SHINING_ONE:
        case GOD_ELYVILON:
            return make_stringf(jtransc("%s forgives followers for abandonment; "
                                        "however, those who later take up the "
                                        "worship of an evil god will be "
                                        "punished. (%s are evil gods.)"),
                                god_name_jc(which_god),
                                comma_separated_fn(begin(evil_gods), end(evil_gods),
                                                   god_name_ja, "、", "、").c_str());

        case GOD_ZIN:
            return make_stringf(jtransc("%s forgives followers for abandonment; "
                                        "however, those who later take up the "
                                        "worship of an evil or chaotic god will "
                                        "be scourged. (%s are evil, and %s are "
                                        "chaotic.)"),
                                god_name_jc(which_god),
                                comma_separated_fn(begin(evil_gods), end(evil_gods),
                                                   god_name_ja, "、", "、").c_str(),
                                comma_separated_fn(begin(chaotic_gods),
                                                   end(chaotic_gods),
                                                   god_name_ja, "、", "、").c_str());
        default:
            return make_stringf(jtransc("%s does not appreciate abandonment, "
                                        "and will call down fearful punishments "
                                        "on disloyal followers!"),
                                god_name_jc(which_god));
    }
}

/**
 * Print a description of the given god's dislikes & wrath effects.
 *
 * @param which_god     The god in question.
 */
static formatted_string _god_wrath_description(god_type which_god)
{
    formatted_string desc;

    _add_par(desc, get_god_dislikes(which_god));
    _add_par(desc, _describe_god_wrath_causes(which_god));
    _add_par(desc, getLongDescription(god_name(which_god) + " wrath"));

    if (which_god != GOD_RU) // Permanent wrath.
    {
        const bool long_wrath = initial_wrath_penance_for(which_god) > 30;
        _add_par(desc, make_stringf(jtransc(long_wrath
                        ? "%s's wrath lasts for a relatively long duration."
                        : "%s's wrath lasts for a relatively short duration."),
                                    god_name_jc(which_god)));
    }

    return desc;
}

static formatted_string _beogh_extra_description()
{
    formatted_string desc;

    for (int i = 1; i <= get_num_apostles(); ++i)
        _add_par(desc, apostle_short_description(i));

    if (you.duration[DUR_BEOGH_CAN_RECRUIT])
    {
        _add_par(desc, "-----------------------------------\n");
        _add_par(desc, apostle_short_description(0));
    }

    return desc;
}

static string _describe_deck_summary()
{
    ostringstream desc;
    desc << jtrans_notrim("Decks of power:\n");
    for (int i = FIRST_PLAYER_DECK; i <= LAST_PLAYER_DECK; i++)
        desc << " " << deck_status((deck_type) i) << "\n";

    string stack = stack_contents();
    if (!stack.empty())
        desc << "\n " << jtrans_notrim("stacked deck: ") << stack << "\n";

    return desc.str();
}

static formatted_string _god_extra_description(god_type which_god)
{
    formatted_string desc;

    switch (which_god)
    {
        case GOD_ASHENZARI:
            desc = formatted_string::parse_string(
                       getLongDescription(god_name(which_god) + " extra"));
            if (have_passive(passive_t::bondage_skill_boost))
            {
                desc.cprintf("\n");
                _add_par(desc, jtrans("Ashenzari supports the following skill groups because of your curses:"));
                _add_par(desc,  _describe_ash_skill_boost());
            }
            break;
        case GOD_BEOGH:
            if (you_worship(GOD_BEOGH))
                desc = _beogh_extra_description();
            break;
        case GOD_GOZAG:
            if (you_worship(GOD_GOZAG))
                _add_par(desc, _describe_branch_bribability());
            break;
        case GOD_HEPLIAKLQANA:
            if (you_worship(GOD_HEPLIAKLQANA))
                desc = formatted_string::parse_string(_describe_ancestor_upgrades());
            break;
        case GOD_NEMELEX_XOBEH:
            if (you_worship(GOD_NEMELEX_XOBEH))
                _add_par(desc, _describe_deck_summary());
            break;
        case GOD_WU_JIAN:
            _add_par(desc, jtrans("Martial attacks:"));
            desc += formatted_string::parse_string(
                        getLongDescription(god_name(which_god) + " extra"));
            break;
        default:
            break;
    }

    return desc;
}

/**
 * Describe miscellaneous information about the given god.
 *
 * @param which_god     The god in question.
 * @return              Info about gods which isn't covered by their powers,
 *                      likes, or dislikes.
 */
static string _get_god_misc_info(god_type which_god)
{
    string info = "";
    skill_type skill = invo_skill(which_god);

    switch (skill)
    {
        case SK_INVOCATIONS:
            break;
        case SK_NONE:
            info += make_stringf(jtransc("%s's powers are not affected by the "
                                         "Invocations skill."),
                                 god_name_jc(which_god));
            break;
        default:
            info += make_stringf(jtransc("%s's powers are based on %s instead "
                                         "of Invocations skill."),
                                 god_name_jc(which_god), skill_name_jc(skill));
            break;
    }

    if (!info.empty())
        info += "\n\n";

    return info;
}

/**
 * Print a detailed description of the given god's likes and powers.
 *
 * @param god       The god in question.
 */
static formatted_string _detailed_god_description(god_type which_god)
{
    formatted_string desc;
    _add_par(desc, getLongDescription(god_name(which_god) + " powers"));
    _add_par(desc, get_god_likes(which_god));
    _add_par(desc, _get_god_misc_info(which_god));
    return desc;
}

/**
 * Describe the given god's level of irritation at the player.
 *
 * Player may or may not be currently under penance.
 *
 * @param which_god     The god in question.
 * @return              A format string, describing the god's ire (or lack of).
 */
static string _raw_penance_message(god_type which_god)
{
    const int penance = you.penance[which_god];

    // Give more appropriate message for the good gods.
    if (penance > 0 && is_good_god(which_god))
    {
        if (is_good_god(you.religion))
            return jtrans("%s is ambivalent towards you.");
        if (!god_hates_your_god(which_god))
        {
            return jtrans("%s is almost ready to forgive your sins.");
                 // == "Come back to the one true church!"
        }
    }

    const int initial_penance = initial_wrath_penance_for(which_god);
    // could do some math tricks to turn this into a table, but it seems fiddly
    if (penance > initial_penance * 3 / 4)
        return jtrans("%s's wrath is upon you!");
    if (penance > initial_penance / 2)
        return jtrans("%s well remembers your sins.");
    if (penance > initial_penance / 4)
        return jtrans("%s's wrath is beginning to fade.");
    if (penance > 0)
    {
        if (which_god == GOD_IGNIS)
            return jtrans("%s' wrath will not burn much longer.");
        return jtrans("%s is almost ready to forgive your sins.");
    }
    return jtrans("%s is neutral towards you.");
}

/**
 * Describe the given god's level of irritation at the player.
 *
 * Player may or may not be currently under penance.
 *
 * @param which_god     The god in question.
 * @return              A description of the god's ire (or lack thereof).
 */
static string _god_penance_message(god_type which_god)
{
    // message は訳済みの書式(%s 1 つ)
    const string message = _raw_penance_message(which_god);
    return make_stringf(message.c_str(), god_name_jc(which_god));
}

/**
 * Print a description of the powers & abilities granted to the player by the
 * given god. If player worships the god, the currently available powers are
 * highlighted.
 *
 * @param which_god     The god in question.
 */
static formatted_string _describe_god_powers(god_type which_god)
{
    formatted_string desc;

    int piety = you_worship(which_god) ? you.piety() : 0;

    desc.textcolour(LIGHTGREY);
    const string header = jtrans("Granted powers:");
    const string cost   = jtrans("(Cost)");
    desc.cprintf("\n\n%s%*s%s\n", header.c_str(),
            max(1, 80 - strwidth(header) - strwidth(cost)),
            "", cost.c_str());

    bool have_any = false;

    // set default color here, so we don't have to set in multiple places for
    // always available passive abilities
    if (!you_worship(which_god))
        desc.textcolour(DARKGREY);
    else
        desc.textcolour(god_colour(which_god));

    // mv: Some gods can protect you from harm.
    // The god isn't really protecting the player - only sometimes saving
    // their life.
    if (god_gives_passive(which_god, passive_t::protect_from_harm)
        || god_gives_passive(which_god, passive_t::lifesaving))
    {
        have_any = true;

        string how;

        if (god_gives_passive(which_god, passive_t::lifesaving))
        {
            how = jtrans((piety >= piety_breakpoint(5)) ? "[god how]carefully" :
                         (piety >= piety_breakpoint(3)) ? "[god how]often" :
                         (piety >= piety_breakpoint(1)) ? "[god how]sometimes"
                                                        : "[god how]occasionally");
        }
        else
            how = jtrans((piety >= piety_breakpoint(5)) ? "[god how]sometimes"
                                                        : "[god how]occasionally");

        desc.cprintf(jtrans_notrimc("%s %sguards your life.\n"),
                god_name_jc(which_god),
                how.c_str());
    }

    switch (which_god)
    {
    case GOD_BEOGH:
    {
        have_any = true;
        if (have_passive(passive_t::convert_orcs))
            desc.textcolour(god_colour(which_god));
        else
            desc.textcolour(DARKGREY);

        if (piety >= piety_breakpoint(5))
            desc.cprintf("%s", jtrans_notrimc("Orcs frequently recognise you as Beogh's chosen one.\n"));
        else
            desc.cprintf("%s", jtrans_notrimc("Orcs sometimes recognise you as one of their own.\n"));
    }
    break;

    case GOD_ZIN:
    {
        have_any = true;
        string how = jtrans(
            (piety >= piety_breakpoint(5)) ? "[god how]always" :
            (piety >= piety_breakpoint(3)) ? "[god how]often" :
            (piety >= piety_breakpoint(1)) ? "[god how]sometimes" :
                                             "[god how]occasionally");

        desc.cprintf(jtrans_notrimc("%s %s shields you from chaos.\n"),
                god_name_jc(which_god), how.c_str());

        how = jtrans(
            (piety >= piety_breakpoint(5)) ? "[god how]often" :
            (piety >= piety_breakpoint(3)) ? "[god how]sometimes" :
            (piety >= piety_breakpoint(1)) ? "[god how]occasionally" :
                                             "[god how]rarely");
        desc.cprintf(jtrans_notrimc("%s %s shields you from Hell.\n"),
                god_name_jc(which_god), how.c_str());
        break;
    }

    case GOD_SHINING_ONE:
    {
        have_any = true;
        desc.cprintf(jtrans_notrimc("%s prevents you from stabbing unaware foes.\n"),
                god_name_jc(which_god));

        const int halo_size = you_worship(which_god) ? you.halo_radius() : -1;
        if (halo_size < 0)
            desc.textcolour(DARKGREY);
        else
            desc.textcolour(god_colour(which_god));
        desc.cprintf("%s", jtrans_notrimc(
                halo_size > 5 ? "You radiate a large righteous aura, and foes "
                                "within it are easier to hit.\n" :
                halo_size > 3 ? "You radiate a righteous aura, and foes "
                                "within it are easier to hit.\n"
                              : "You radiate a small righteous aura, and foes "
                                "within it are easier to hit.\n"));

        if (piety >= piety_breakpoint(1))
            desc.textcolour(god_colour(which_god));
        else
            desc.textcolour(DARKGREY);
        const string how = jtrans(
            (piety >= piety_breakpoint(5)) ? "[god how]completely" :
            (piety >= piety_breakpoint(3)) ? "[god how]mostly" :
                                             "[god how]partially");
        desc.cprintf(jtrans_notrimc("%s %s shields you from negative energy.\n"),
                god_name_jc(which_god), how.c_str());
        break;
    }

    case GOD_JIYVA:
        have_any = true;
        desc.cprintf("%s", jtrans_notrimc("Jellies are peaceful and will consume items off the floor.\n"));
        desc.cprintf("%s", jtrans_notrimc("Jiyva prevents you from harming jellies.\n"));

        if (have_passive(passive_t::jelly_regen))
            desc.textcolour(god_colour(which_god));
        else
            desc.textcolour(DARKGREY);
        desc.cprintf("%s", jtrans_notrimc(
                     piety >= piety_breakpoint(5)
                     ? "Your health and magic regeneration is very greatly accelerated.\n" :
                     piety >= piety_breakpoint(3)
                     ? "Your health and magic regeneration is greatly accelerated.\n"
                     : "Your health and magic regeneration is accelerated.\n"));
        break;

    case GOD_CHEIBRIADOS:
        have_any = true;
        if (have_passive(passive_t::stat_boost))
            desc.textcolour(god_colour(which_god));
        else
            desc.textcolour(DARKGREY);
        desc.cprintf(jtrans_notrimc(
                piety >= piety_breakpoint(5) ? "%s greatly slows your movement.\n" :
                piety >= piety_breakpoint(2) ? "%s slows your movement.\n"
                                             : "%s slightly slows your movement.\n"),
                god_name_jc(which_god));
        desc.cprintf(jtrans_notrimc("%s supports your attributes. (+%d)\n"),
                god_name_jc(which_god),
                chei_stat_boost(piety));
        break;

    case GOD_VEHUMET:
        have_any = true;
        if (const int numoffers = you.vehumet_gifts.size())
        {
            const string offer = numoffers == 1
                               ? spell_title_j(*you.vehumet_gifts.begin())
                               : jtrans("some of Vehumet's most lethal spells");
            desc.cprintf(jtrans_notrimc("You can memorise %s.\n"), offer.c_str());
        }
        else if (!you.has_mutation(MUT_INNATE_CASTER))
        {
            desc.textcolour(DARKGREY);
            desc.cprintf("%s", jtrans_notrimc("You can memorise some of Vehumet's spells.\n"));
        }
        break;

    case GOD_YREDELEMNUL:
        // TODO: Vary the text depending on the size of the umbra.
        desc.cprintf("%s", jtrans_notrimc("You are surrounded by an umbra.\n"
                     "Foes that die within your umbra may be raised as undead servants.\n"));
        break;

    case GOD_HEPLIAKLQANA:
        // Frailty occurs even under penance post-abandonment, so we can't put
        // this in the usual god_powers block.
        have_any = true;
    {
        const auto textcol = have_passive(passive_t::frail) ? god_colour(which_god) : DARKGREY;
        // We need to set textcolour before each line so that it'll display
        // correctly in webtiles. (It works fine locally regardless.)
        // Feature request: not this.
        desc.textcolour(textcol);
        desc.cprintf("%s", jtrans_notrimc("Your life essence is reduced. (-10% HP)\n"));
    }
        break;

    default:
        break;
    }

    for (const auto& power : get_god_powers(which_god))
    {
        // hack: don't mention the necronomicon alone unless it
        // wasn't already mentioned by the other description
        if (power.abil == ABIL_KIKU_GIFT_CAPSTONE_SPELLS
            && !you.has_mutation(MUT_NO_GRASPING))
        {
            continue;
        }
        // Skip over Makhleb's brand options after the first one, since
        // only the first one has an associated god ability.
        if (power.abil == ABIL_MAKHLEB_BRAND_SELF_2
            || power.abil == ABIL_MAKHLEB_BRAND_SELF_3)
        {
            continue;
        }
        have_any = true;

        if (you_worship(which_god)
            && (power.rank <= 0
                || power.rank == 7 && can_do_capstone_ability(which_god)
                || piety_rank(piety) >= power.rank)
            && (!player_under_penance()
                || power.rank == -1))
        {
            desc.textcolour(god_colour(which_god));
        }
        else
            desc.textcolour(DARKGREY);

        // XXX: I don't like this, but there's no other obvious way to
        //      slot the destruction upgrade mutations in at the right piety
        //      point in the list for them.
        if (which_god == GOD_MAKHLEB && power.rank == 4)
        {
            desc.textcolour(god_colour(which_god));
            if (you.has_mutation(MUT_MAKHLEB_DESTRUCTION_GEH))
                desc.cprintf("%s", jtrans_notrimc("Your Destruction is augmented by the power of Gehenna.\n"));
            else if (you.has_mutation(MUT_MAKHLEB_DESTRUCTION_COC))
                desc.cprintf("%s", jtrans_notrimc("Your Destruction is augmented by the power of Cocytus.\n"));
            else if (you.has_mutation(MUT_MAKHLEB_DESTRUCTION_TAR))
                desc.cprintf("%s", jtrans_notrimc("Your Destruction is augmented by the power of Tartarus.\n"));
            else if (you.has_mutation(MUT_MAKHLEB_DESTRUCTION_DIS))
                desc.cprintf("%s", jtrans_notrimc("Your Destruction is augmented by the power of Dis.\n"));
            else
            {
                desc.textcolour(DARKGREY);
                desc.cprintf("%s", jtrans_notrimc("Your Destruction will be augmented by one of the Four Hells.\n"));
            }

            continue;
        }

        if (power.abil == ABIL_MAKHLEB_BRAND_SELF_1
            && !makhleb_mark_name().empty())
        {
                desc.textcolour(god_colour(which_god));
                // makhleb_mark_name() は変異名(英語)。訳は data-mutations.txt 側
                desc.cprintf(jtrans_notrimc("You have been branded with the %s.\n"),
                             jtransc(makhleb_mark_name()));
                continue;
        }

        string buf = power.general;

        // Skip listing powers with no description (they are intended to be hidden)
        if (buf.length() == 0)
        {
            have_any = false;
            continue;
        }

        // power.general は religion.cc の god_powers の英文。
        // 文の断片(小文字始まり)の訳は「〜こと」の形(religion.cc の
        // "You can now %s." と同じ)。
        if (!isupper(buf[0])) // Complete sentence given?
            buf = make_stringf(jtransc("You can %s."), jtransc(buf));
        else
            buf = jtrans(buf);

        // 複数行の文は最後の行の幅で揃える
        const string::size_type nl = buf.rfind('\n');
        const int desc_len = strwidth(nl == string::npos ? buf
                                                         : buf.substr(nl + 1));

        // make_cost_description() が訳済みでもそうでなくても "なし" を省く
        const string cost_desc = make_cost_description(power.abil);
        string abil_cost = "(" + cost_desc + ")";
        if (cost_desc == "None" || cost_desc == jtrans("[ability cost]None"))
            abil_cost = "";

        desc.cprintf("%s%*s%s\n", buf.c_str(),
                max(1, 80 - desc_len - strwidth(abil_cost)),
                "", abil_cost.c_str());
    }

    if (!have_any)
        desc.cprintf("%s", jtrans_notrimc("[god powers]None.\n"));

    // Show Jiyva's opening of the Slime Pits at the bottom of the list
    // We want this to stay green permanently once the player hits 6*
    if (which_god == GOD_JIYVA)
    {
        if (you.one_time_ability_used[which_god])
            desc.textcolour(god_colour(which_god));
        else
            desc.textcolour(DARKGREY);
        desc.cprintf("%s", jtrans_notrimc("Jiyva will unlock the Slime Pits vaults.\n"));
    }

    return desc;
}

static formatted_string _god_overview_description(god_type which_god)
{
    formatted_string desc;

    // Print god's description.
    const string god_desc = getLongDescription(god_name(which_god));
    desc += trimmed_string(god_desc) + "\n";

    // Title only shown for our own god.
    if (you_worship(which_god))
    {
        // Print title based on piety.
        desc.cprintf("%s", jtrans_notrimc("\nTitle  - "));
        desc.textcolour(god_colour(which_god));

        string title = god_title(which_god, you.species, you.raw_piety);
        desc.cprintf("%s", title.c_str());
    }

    // mv: Now let's print favour as Brent suggested.
    // I know these messages aren't perfect so if you can think up
    // something better, do it.

    desc.textcolour(LIGHTGREY);
    desc.cprintf("%s", jtrans_notrimc("\nFavour - "));
    desc.textcolour(god_colour(which_god));

    if (!you_worship(which_god))
        desc.cprintf("%s", _god_penance_message(which_god).c_str());
    else
        desc.cprintf("%s", _describe_favour(which_god).c_str());
    desc += _describe_god_powers(which_god);
    desc.cprintf("\n\n");

    return desc;
}

static void build_partial_god_ui(god_type which_god, shared_ptr<ui::Popup>& popup, shared_ptr<Switcher>& desc_sw, shared_ptr<Switcher>& more_sw)
{
    formatted_string topline;
    topline.textcolour(god_colour(which_god));
    topline += formatted_string(god_name_j(which_god, true));

    auto vbox = make_shared<Box>(Widget::VERT);
    vbox->set_cross_alignment(Widget::STRETCH);
    auto title_hbox = make_shared<Box>(Widget::HORZ);

#ifdef USE_TILE
    auto icon = make_shared<Image>();
    const tileidx_t idx = tileidx_feature_base(altar_for_god(which_god));
    icon->set_tile(tile_def(idx));
    title_hbox->add_child(std::move(icon));
#endif

    auto title = make_shared<Text>(topline.trim());
    title->set_margin_for_sdl(0, 0, 0, 16);
    title_hbox->add_child(std::move(title));

    title_hbox->set_main_alignment(Widget::CENTER);
    title_hbox->set_cross_alignment(Widget::CENTER);
    vbox->add_child(std::move(title_hbox));

    desc_sw = make_shared<Switcher>();
    more_sw = make_shared<Switcher>();
    desc_sw->current() = 0;
    more_sw->current() = 0;

    const formatted_string descs[4] = {
        _god_overview_description(which_god),
        _detailed_god_description(which_god),
        _god_wrath_description(which_god),
        _god_extra_description(which_god)
    };

    // 画面下の切り替え表示: "[!]: 概要|能力|神罰(|その他)"。
    // 今のページを <w> で強調する。
    const int num_tabs = descs[3].empty() ? 3 : 4;
    const string tab_names[4] =
    {
        jtrans("[god desc tab]Overview"),
        jtrans("[god desc tab]Powers"),
        jtrans("[god desc tab]Wrath"),
        jtrans("[god desc tab]Extra"),
    };
    auto more_line = [&](int cur)
    {
        string line = "[<w>!</w>]: ";
        for (int j = 0; j < num_tabs; j++)
        {
            if (j)
                line += "|";
            line += j == cur ? "<w>" + tab_names[j] + "</w>" : tab_names[j];
        }
        return line;
    };

    for (int i = 0; i < 4; i++)
    {
        const auto &desc = descs[i];
        if (desc.empty())
            continue;

        auto scroller = make_shared<Scroller>();
        auto text = make_shared<Text>(desc.trim());
        text->set_wrap_text(true);
        scroller->set_child(text);
        desc_sw->add_child(std::move(scroller));

        more_sw->add_child(make_shared<Text>(
                formatted_string::parse_string(more_line(i))));
    }

    desc_sw->set_margin_for_sdl(20, 0);
    desc_sw->set_margin_for_crt(1, 0);
    desc_sw->expand_h = false;
#ifdef USE_TILE_LOCAL
    desc_sw->max_size().width = tiles.get_crt_font()->char_width()*80;
#endif
    vbox->add_child(desc_sw);

    vbox->add_child(more_sw);

    popup = make_shared<ui::Popup>(vbox);
}

static const string _god_service_fee_description(god_type which_god)
{
    const int fee = (which_god == GOD_GOZAG) ? gozag_service_fee() : 0;

    if (which_god == GOD_GOZAG)
    {
        if (fee == 0)
        {
            return coinflip() ? jtrans_notrim(" (no fee if you act now)")
                              : jtrans_notrim(" (no fee if you join today)");
        }
        else
            return make_stringf(jtrans_notrimc(" (%d gold; you have %d)"),
                                fee, you.gold);
    }

    return "";
}

#ifdef USE_TILE_WEB
static void _send_god_ui(god_type god, bool is_altar)
{
    tiles.json_open_object();

    const tileidx_t idx = tileidx_feature_base(altar_for_god(god));
    tiles.json_open_object("tile");
    tiles.json_write_int("t", idx);
    tiles.json_write_int("tex", get_tile_texture(idx));
    tiles.json_close_object();

    tiles.json_write_int("colour", god_colour(god));
    tiles.json_write_string("name", god_name_j(god, true));
    tiles.json_write_bool("is_altar", is_altar);

    tiles.json_write_string("description", getLongDescription(god_name(god)));
    if (you_worship(god))
        tiles.json_write_string("title", god_title(god, you.species, you.piety()));
    tiles.json_write_string("favour", you_worship(god) ?
            _describe_favour(god) : _god_penance_message(god));
    tiles.json_write_string("powers_list",
            _describe_god_powers(god).to_colour_string(LIGHTGREY));
    tiles.json_write_string("info_table", "");

    tiles.json_write_string("powers",
            _detailed_god_description(god).to_colour_string(LIGHTGREY));
    tiles.json_write_string("wrath",
            _god_wrath_description(god).to_colour_string(LIGHTGREY));
    tiles.json_write_string("extra",
            _god_extra_description(god).to_colour_string(LIGHTGREY));
    tiles.json_write_string("service_fee",
            _god_service_fee_description(god));
    tiles.push_ui_layout("describe-god", 1);
}
#endif

void describe_god(god_type which_god)
{
    if (which_god == GOD_NO_GOD) //mv: No god -> say it and go away.
    {
        mpr(jtrans("You are not religious."));
        return;
    }

    shared_ptr<ui::Popup> popup;
    shared_ptr<Switcher> desc_sw;
    shared_ptr<Switcher> more_sw;
    build_partial_god_ui(which_god, popup, desc_sw, more_sw);

    bool done = false;
    popup->on_keydown_event([&](const KeyEvent& ev) {
        const auto key = ev.key();
        if (key == '!' || key == '^')
        {
            int n = (desc_sw->current() + 1) % desc_sw->num_children();
            desc_sw->current() = more_sw->current() = n;
#ifdef USE_TILE_WEB
                tiles.json_open_object();
                tiles.json_write_int("pane", n);
                tiles.ui_state_change("describe-god", 0);
#endif
            return true;
        }
        if (desc_sw->current_widget()->on_event(ev))
            return true;
        return done = ui::key_exits_popup(key, false);
    });

#ifdef USE_TILE_WEB
    _send_god_ui(which_god, false);
    popup->on_layout_pop([](){ tiles.pop_ui_layout(); });
#endif

    ui::run_layout(popup, done);
}

bool describe_god_with_join(god_type which_god)
{
    const string service_fee = _god_service_fee_description(which_god);

    shared_ptr<ui::Popup> popup;
    shared_ptr<Switcher> desc_sw;
    shared_ptr<Switcher> more_sw;
    build_partial_god_ui(which_god, popup, desc_sw, more_sw);

    for (auto& child : *more_sw)
    {
        Text* label = static_cast<Text*>(child.get());
        formatted_string text = label->get_text();
        text += formatted_string::parse_string(
                    jtrans_notrim("  [<w>J</w>/<w>Enter</w>]: join"));

        // We assume that a player who has enough gold such that
        // the join fee plus accumulated gold overflows knows what this menu
        // does.
        // 日本語では "join" の訳(「入信する」)だけで足りるので空にしている
        const string religion = jtrans_notrim(" [god join]religion");
        if (text.width() + strwidth(service_fee) + strwidth(religion) <= MIN_COLS)
            text += religion;
        if (!service_fee.empty())
            text += service_fee;
        label->set_text(text);
    }

    // States for the state machine
    enum join_step_type {
        SHOW = -1, // Show the usual god UI
        ABANDON, // Ask whether to abandon god, if applicable
    };

    // Add separate text widgets the possible abandon-god prompts;
    // then when a different prompt needs to be shown, we switch to that prompt.
    // This is somewhat brittle, but ensures that the UI doesn't resize when
    // switching between prompts.
    const string abandon_prompt =
        make_stringf(jtransc("Are you sure you want to abandon %s?"),
                god_name_jc(you.religion));
    const string yesno_only_msg = jtrans_notrim(" [Y]es or [n]o only, please.");
    formatted_string prompt_fs(abandon_prompt, channel_to_colour(MSGCH_PROMPT));

    more_sw->add_child(make_shared<Text>(prompt_fs));

    prompt_fs.cprintf("%s", yesno_only_msg.c_str());
    more_sw->add_child(make_shared<Text>(prompt_fs));

    join_step_type step = SHOW;
    bool yesno_only = false;
    bool done = false, join = false;

    // The join-god UI state machine transition function
    popup->on_keydown_event([&](const KeyEvent& ev) {
        const auto keyin = ev.key();

        // Always handle escape and pane-switching keys the same way
        if (ui::key_exits_popup(keyin, false))
            return done = true;
        if (keyin == '!' || keyin == '^')
        {
            int n = (desc_sw->current() + 1) % desc_sw->num_children();
            desc_sw->current() = n;
#ifdef USE_TILE_WEB
            tiles.json_open_object();
            tiles.json_write_int("pane", n);
            tiles.ui_state_change("describe-god", 0);
#endif
            if (step == SHOW)
            {
                more_sw->current() = n;
                return true;
            }
            else
                yesno_only = false;
        }

        // Next, allow child widgets to handle scrolling keys
        // NOTE: these key exceptions are also specified in ui-layouts.js
        if (keyin != 'J' && keyin != CK_ENTER)
            if (desc_sw->current_widget()->on_event(ev))
                return true;

        if (step == ABANDON)
        {
            if (keyin != 'Y' && toupper_safe(keyin) != 'N')
                yesno_only = true;
            else
                yesno_only = false;

            if (toupper_safe(keyin) == 'N')
            {
                canned_msg(MSG_OK);
                return done = true;
            }

            if (keyin == 'Y')
                return done = join = true;
        }
        else if ((keyin == 'J' || keyin == CK_ENTER) && step == SHOW)
        {
            if (you_worship(GOD_NO_GOD))
                return done = join = true;

            step = ABANDON;
        }
        else
            return done = true;

#ifdef USE_TILE_WEB
        tiles.json_open_object();
        string prompt = abandon_prompt + (yesno_only ? yesno_only_msg : "");
        tiles.json_write_string("prompt", prompt);
        tiles.json_write_int("pane", desc_sw->current());
        tiles.ui_state_change("describe-god", 0);
#endif
        if (step == ABANDON)
            more_sw->current() = desc_sw->num_children() + step*2 + yesno_only;
        return true;
    });

#ifdef USE_TILE_WEB
    _send_god_ui(which_god, true);
    popup->on_layout_pop([](){ tiles.pop_ui_layout(); });
#endif

    ui::run_layout(popup, done);

    return join;
}
