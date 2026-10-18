/**
 * @file
 * database.h
**/

#pragma once

#include <list>
#include <vector>

using std::vector;

#ifdef DB_NDBM
extern "C" {
#   include <ndbm.h>
}
#elif defined(DB_DBH)
extern "C" {
#   define DB_DBM_HSEARCH 1
#   include <db.h>
}
#elif defined(USE_SQLITE_DBM)
#   include "sqldbm.h"
#else
#   error DBM interfaces unavailable!
#endif

#define DPTR_COERCE char *

void databaseSystemInit();
void databaseSystemShutdown();

typedef bool (*db_find_filter)(string key, string body);

string getQuoteString(const string &key);
string getLongDescription(const string &key);

vector<string> getLongDescKeysByRegex(const string &regex,
                                      db_find_filter filter = nullptr);
vector<string> getLongDescBodiesByRegex(const string &regex,
                                        db_find_filter filter = nullptr);

string getGameStartDescription(const string &key);

string getShoutString(const string &monst, const string &suffix = "");
string getSpeakString(const string &key);
string getRandMonNameString(const string &montype);
string getRandNameString(const string &itemtype, const string &suffix = "");
string getHelpString(const string &topic);
string getMiscString(const string &misc, const string &suffix = "");
string getHintString(const string &key);
string getEgoString(const string &key);

vector<string> getAllFAQKeys();
string getFAQ_Question(const string &key);
string getFAQ_Answer(const string &question);

// 日本語訳辞書(dat/database/ja/jtrans/)を引く関数群
string jtrans(const char* key, const bool linefeed = false);
string jtrans(const string &key, const bool linefeed = false);
#define jtransln(x) (jtrans(x, true))
#define jtransc(x) (jtrans(x).c_str())
#define jtranslnc(x) (jtrans(x, true).c_str())
bool jtrans_has_key(const string &key);
string tagged_jtrans(const string &prefix, const string &key, const string &suffix = "");
#define tagged_jtransc(p, k) (tagged_jtrans(p, k).c_str())
string jtrans_notrim(const string &key);
#define jtrans_notrimc(x) (jtrans_notrim(x).c_str())

template<typename C1, typename C2>
void append_container_jtrans(C1& container_base, const C2& container_append)
{
    for (auto val : container_append)
    {
        container_base.push_back(jtrans(val));
    }
}

/**
 * 渡されたシーケンスの各要素を変換しながら適切に繋ぐ
 *
 * [a, b]       -> "aとb"
 * [a, b, c]    -> "aとb、そしてc"
 * [a, b, c, d] -> "aとb、c、そしてd"
 *
 * stringifyにラムダ式を渡すことで各要素への前処理が可能
 *
 * @param start 処理を行うシーケンスの開始イテレータ
 * @param end 処理を行うシーケンスの終端イテレータ
 * @param stringify (string -> string)のラムダ式
 * @param first 1番目と2番目の要素の間の区切り文字
 * @param second n番目とn+1番目の要素の間の区切り文字(n > 2)
 * @param fin last-1番目とlast番目の要素の間の区切り文字
 *          　(first, secondの方を優先)
 */
template <typename Z, typename F>
string to_separated_fn(Z start, Z end, F stringify,
                       const string &first = "と",
                       const string &second = "、",
                       const string &fin = "、そして")
{
    string text;
    for (Z i = start; i != end; ++i)
    {
        if (i != start)
        {
            if (prev(i) == start)
                text += first;
            else if (next(i) != end)
                text += second;
            else
                text += fin;
        }

        text += stringify(*i);
    }
    return text;
}

/**
 * 渡されたシーケンスの各要素を日本語化しながら適切に繋ぐ
 * (to_separated_fn()の特殊版)
 *
 * [a, b]       -> "jtrans(a)とjtrans(b)"
 * [a, b, c]    -> "jtrans(a)とjtrans(b)、そしてjtrans(c)"
 * [a, b, c, d] -> "jtrans(a)とjtrans(b)、jtrans(c)、そしてjtrans(d)"
 *
 * @param start 処理を行うシーケンスの開始イテレータ
 * @param end 処理を行うシーケンスの終端イテレータ
 * @param first 1番目と2番目の要素の間の区切り文字
 * @param second n番目とn+1番目の要素の間の区切り文字(n > 2)
 * @param fin last-1番目とlast番目の要素の間の区切り文字
 *          　(first, secondの方を優先)
 */
template<typename Z>
string to_separated_line(Z start, Z end,
                         const string &first = "と",
                         const string &second = "、",
                         const string &fin = "、そして")
{
    auto to_j = [] (const string &s) { return jtrans(s); };

    return to_separated_fn(start, end, to_j, first, second, fin);
}

// tagged_jtrans shortcuts
#define branch_name_j(br) (tagged_jtrans("[branch]", br))
#define branch_name_jc(br) (branch_name_j(br).c_str())
#define zap_name_j(z) (tagged_jtrans("[zap]", z))
#define zap_name_jc(z) (zap_name_j(z).c_str())
static inline string verb_j(const string &verb, const string &tag = "[verb]")
{
    return tagged_jtrans(tag, verb);
}
#define verb_jc(v) (verb_j(v).c_str())
#define adj_j(a) (tagged_jtrans("[adj]", a))
#define adj_jc(a) (adj_j(a).c_str())
#define adv_j(a) (tagged_jtrans("[adv]", a))
#define adv_jc(a) (adv_j(a).c_str())

