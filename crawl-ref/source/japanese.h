/**
 * @file
 * @brief Functions and data structures dealing with the syntax,
 *        morphology, and orthography of the Japanese language.
**/

#pragma once

#include <string>

#include "description-level-type.h"
#include "gender-type.h"
#include "pronoun-type.h"

using std::string;

struct item_def;

enum jconj
{
    JCONJ_IRRE, // 未然形
    JCONJ_CONT, // 連用形
    JCONJ_TERM, // 終止形
    JCONJ_ATTR, // 連体形
    JCONJ_HYPO, // 仮定形
    JCONJ_IMPR, // 命令形
    JCONJ_PERF, // 完了形
    JCONJ_PASS, // 受動態
    JCONJ_PRES, // 現在形
    JCONJ_PAST, // 過去形
};

bool jtrans_enabled();

const string conjugate_verb_j(const string &verb, bool plural);
const string decline_pronoun_j(gender_type gender, pronoun_type variant);
const char * general_counter_suffix(const int size);
string jconj_verb(const string& verb, jconj conj);
string thing_do_grammar_j(description_level_type dtype, bool add_stop,
                          bool force_article, string desc);
string counter_suffix(const item_def &item);
const string jpluralise(const string &name, const string &prefix,
                        const string &suffix = "");
string apply_description_j(description_level_type desc, const string &name,
                           int quantity = 1, bool in_words = false);
string get_desc_quantity_j(const int quant, const int total, string whose);
string jnumber_for_hydra_heads(int heads);
