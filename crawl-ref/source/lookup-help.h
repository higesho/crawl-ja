/**
 * @file
 * @brief Let the player search for descriptions of monsters, items, etc.
 **/

#pragma once

#include <string>

#include "command-type.h"
#include "lookup-help-type.h"

using std::string;

void keyhelp_query_descriptions(command_type where_from=CMD_NO_CMD);

string lookup_help_type_name(lookup_help_type lht);
char lookup_help_type_shortcut(lookup_help_type lht);
bool find_description_of_type(lookup_help_type lht);

// 日本語版: スポイラー用のデータ(ゲーム内検索の全項目ほか)を JSON で書き出す
void write_spoiler_json(const string &filename);
