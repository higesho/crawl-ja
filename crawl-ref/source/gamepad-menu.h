/**
 * @file
 * @brief ゲームパッドの割り当て(画面・保存・引き方)。ローカルのタイル版だけ。
 *
 * 設計は ja-tools/GAMEPAD_DESIGN.md §8。
 */

#pragma once

#ifdef USE_TILE_LOCAL

#include "command-type.h"
#include "gamepad.h"

// 枠に割り当てたコマンド。引く順は「画面で変えた分 → rc の pad_bind → 既定
// (単独押しだけ)」。割り当てが無ければ CMD_NO_CMD。
command_type gamepad_bound_command(pad_button button, int mods);

// パッドに割り当てられるコマンドか。メイン画面のコマンドから、内部で使うだけの
// もの(CMD_NO_CMD_DEFAULT、CMD_MOUSE_MOVE、CMD_MOUSE_CLICK)を除いたもの。
// 割り当ての画面・rc の pad_bind・pad_binds.txt の読み込みで共通に使う。
bool gamepad_command_bindable(command_type cmd);

// 割り当ての画面が開いているか。開いている間、パッドは割り当てに関係なく
// 枠のキー(Pad-X など)を出す(押したボタンの枠を開くため)。
bool gamepad_menu_capturing();

// 割り当ての画面を開く(CMD_GAMEPAD_MENU)。
void gamepad_menu();

// 画面で変えた分をマクロと同じ場所の pad_binds.txt から読む(macro_init から)。
void gamepad_load_binds();

#endif
