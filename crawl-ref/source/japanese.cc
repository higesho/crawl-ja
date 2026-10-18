/**
 * @file
 * @brief Functions and data structures dealing with the syntax,
 *        morphology, and orthography of the Japanese language.
**/

#include "AppHdr.h"

#include "japanese.h"

#include "database.h"
#include "english.h"
#include "item-def.h"
#include "item-name.h"
#include "jobs.h"
#include "options.h"
#include "player.h"
#include "religion.h"
#include "skills.h"
#include "species.h"
#include "spl-util.h"
#include "stringutil.h"

/// language = ja のとき true。日本語向けに処理を分けるときに使う。
bool jtrans_enabled()
{
    return Options.language == lang_t::JA;
}

const string conjugate_verb_j(const string &verb, bool /* ignore */)
{
    return verb_j(verb);
}

const string decline_pronoun_j(gender_type gender, pronoun_type variant)
{
    ASSERT_RANGE(gender, 0, NUM_GENDERS);
    ASSERT_RANGE(variant, 0, NUM_PRONOUN_CASES);

    const char *variant_tag = "buggy";
    switch (variant)
    {
    case PRONOUN_SUBJECTIVE: variant_tag = "subj"; break;
    case PRONOUN_POSSESSIVE: variant_tag = "poss"; break;
    case PRONOUN_REFLEXIVE:  variant_tag = "refl"; break;
    case PRONOUN_OBJECTIVE:  variant_tag = "obj"; break;
    default: break;
    }

    const char *gender_tag = "buggy";
    switch (gender)
    {
    case GENDER_NEUTER:  gender_tag = "neuter"; break;
    case GENDER_MALE:    gender_tag = "male"; break;
    case GENDER_FEMALE:  gender_tag = "female"; break;
    case GENDER_YOU:     gender_tag = "you"; break;
    case GENDER_NEUTRAL: gender_tag = "neutral"; break;
    default:
        return make_stringf("buggy pronoun: %d, %d", gender, variant);
    }

    return tagged_jtrans(make_stringf("[gender %s %s]", gender_tag,
                                      variant_tag),
                         decline_pronoun(gender, variant));
}

const char * general_counter_suffix(const int size)
{
    if (size <= 9)
        return "つ";
    else
        return "個";
}

string jconj_verb(const string& verb, jconj conj)
{
    string v = verb;

    switch (conj)
    {
        // 必要に応じて随時追加
    case JCONJ_IRRE:  break;
    case JCONJ_CONT:  break;
    case JCONJ_TERM:  break;
    case JCONJ_ATTR:  break;
    case JCONJ_HYPO:  break;
    case JCONJ_IMPR:  break;
    case JCONJ_PERF:
        v = replace_all(v, "立てる", "立てた");
        v = replace_all(v, "鳴く", "鳴いた");
        v = replace_all(v, "放つ", "放った");
        v = replace_all(v, "吠える", "吠えた");
        break;
    case JCONJ_PASS:
        v = replace_all(v, "を追放した", "が追放された"); // _takenote_kill_verb()
        v = replace_all(v, "を中立化した", "が中立化させられた");
        v = replace_all(v, "を隷属させた", "が隷属させられた");
        v = replace_all(v, "をスライムに変えた", "がスライムに変えられた");
        v = replace_all(v, "を殺した", "が殺された");
        break;
    case JCONJ_PRES:
        v = replace_all(v, "した", "する");
        v = replace_all(v, "泳いだ", "泳ぐ");
        v = replace_all(v, "唱えた", "唱える");
        v = replace_all(v, "使った", "使う");
        v = replace_all(v, "を上げた", "を上げる");
        v = replace_all(v, "叫んだ", "叫ぶ");
        v = replace_all(v, "鳴いた", "鳴く");
        break;
    case JCONJ_PAST:
        v = replace_all(v, "いる", "いた");
        break;
    }

    return v;
}

/*
 * english.cc/thing_do_grammar()の代替
 * 日本語には冠詞がないので、DESC_NONE 以外はそのまま返す。
 */
string thing_do_grammar_j(description_level_type dtype, bool /*add_stop*/,
                          bool force_article, string desc)
{
    if (dtype == DESC_PLAIN || !force_article)
        return desc;

    if (dtype == DESC_NONE)
        return "";
    return desc;
}

/**
 * アイテムの助数詞を返す。
 * 辞書の "[counter]<種別名>"(例: "[counter]hand axe")を優先し、なければ
 * "[counter class]<分類名>"(例: "[counter class]Weapons")、
 * それもなければ「個」。
 */
string counter_suffix(const item_def &item)
{
    const string sub = lowercase_string(sub_type_string(item, true));
    if (!sub.empty() && jtrans_has_key("[counter]" + sub))
        return jtrans("[counter]" + sub);

    const string base = base_type_string(item);
    if (jtrans_has_key("[counter class]" + base))
        return jtrans("[counter class]" + base);

    return "個";
}

const string jpluralise(const string &name, const string &prefix,
                        const string &suffix)
{
    return prefix + jtrans(name) + suffix;
}

/*
 * english.cc/apply_description()の代替
 */
string apply_description_j(description_level_type desc, const string &name,
                           int quantity, bool /*in_words*/)
{
    switch (desc)
    {
    case DESC_A:
        return quantity > 1 ? make_stringf("%d %s", quantity, jtransc(name))
                            : jtrans(name);
    case DESC_YOUR:
        return jtrans("your ") + jtrans(name);
    case DESC_THE:
    case DESC_PLAIN:
    default:
        return jtrans(name);
    }
}

string get_desc_quantity_j(const int quant, const int total, string whose)
{
    if (total == quant)
        return whose;
    else if (quant == 1)
        return whose + "のうちの一つ";
    else if (quant == 2)
        return whose + "のうちの二つ";
    else if (quant >= total * 3 / 4)
        return whose + "のほとんど";
    else
        return whose + "のうちいくつか";
}

/**
 * 0～27の範囲の数値を漢数字変換(ヒドラ系モンスター用)
 */
string jnumber_for_hydra_heads(int heads)
{
    ASSERT((0 < heads) && (heads <= 27)); // for Lernaean hydra

    const string nums[10] = {"", "一", "二", "三", "四", "五", "六", "七",
                             "八", "九"};
    string num;

    if (heads >= 20)
        num += nums[heads / 10];
    if (heads >= 10)
        num += "十";
    num += nums[heads % 10];

    return num;
}

string spell_title_j(spell_type spell)
{
    return tagged_jtrans("[spell]", spell_title(spell));
}

string skill_name_j(skill_type skill)
{
    return tagged_jtrans("[skill]", skill_name(skill));
}

string species_name_j(species_type species, bool adjective)
{
    if (adjective)
        return tagged_jtrans("[spec adj]",
                             species::name(species, species::SPNAME_ADJ));
    return tagged_jtrans("[spec name]", species::name(species));
}

string job_name_j(job_type job)
{
    return tagged_jtrans("[job]", get_job_name(job));
}

string god_name_j(god_type god, bool long_name)
{
    // ジーヴァの長い名前は "Jiyva <ランダムな名前> the Shapeless" なので、
    // data-gods.txt の "%s the Shapeless" に「ジーヴァ・<ランダムな名前>」を
    // 入れて組み立てる(ランダムな名前は英字のまま)
    if (god == GOD_JIYVA && long_name)
    {
        string name = jtrans(god_name_jiyva(false));
        if (!you.jiyva_second_name.empty())
            name += "・" + you.jiyva_second_name;
        return make_stringf(jtransc("%s the Shapeless"), name.c_str());
    }
    return jtrans(god_name(god, long_name));
}
