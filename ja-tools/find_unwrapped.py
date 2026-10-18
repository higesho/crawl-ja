"""jtrans で包まれていない、プレイヤーに見えそうな英文リテラルを洗い出す。

`jtools.py check` は jtrans で包んだキーの未訳しか見ないので、そもそも
包まれていない英文はこれで探す。結果は候補(目安)なので、一つずつ
ソースを読んで確かめること。

リポジトリのルートで実行する:
  python -I ja-tools/find_unwrapped.py [オプション] [FILE...]

  FILE を省略すると全ソース(*.cc, *.h)。除外ファイルは EXCLUDE を参照。
  -a, --all      辞書に同じキーがある候補(間接的に包まれているらしいもの)も出す
  -w, --words    1 語だけのリテラルも出す(メニューの見出しなどの表示文脈のみ)
  -s, --summary  ファイルごとの件数だけ出す
  -v, --vars     別の見方: 表示関数(mpr など)に渡す変数に、同じ関数の少し前で
                 包まれていない英文を代入している所を出す
  -x FILE        除外するファイルを追加(何度でも)

出力: ファイル:行  [文脈]  "英文"
  文脈は、リテラルを囲む呼び出しの関数名(mpr, MenuEntry, set_title など)。
  `=` は代入、`return` は戻り値、`?:` は三項演算子、`init` は配列・構造体の初期化。

除外する文脈: dprf などのデバッグ出力、ASSERT/die/end、#ifdef DEBUG/WIZARD の中、
props のキー([] の中、props.* の引数)、== / != の比較、#include/#define の KEY、
Lua の呼び出し、ファイル名らしい文字列、識別子らしい文字列(空白なし)、
死因(aux_source)・blame・変異の理由・マイルストーン(スコアやセーブに残る)、
DB の検索キーを渡す関数(getSpeakString など)。

辞書にある英文は、別の箇所で包んでいるか、変数経由で包んでいる(jtrans(msg))
ことが多いので既定では出さない(mpr など表示関数の直接の引数なら出す)。
データ表(*-data.h)の名前は表示側で [タグ] 付きで引くので、`[タグ]英文` の
キーがあれば訳ありとみなす。
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import jtools  # noqa: E402

sys.stdout.reconfigure(encoding='utf-8')
sys.stderr.reconfigure(encoding='utf-8')

SRC = jtools.SRC

# 対象外のファイル(前方一致・完全一致)
EXCLUDE_PREFIX = ('wiz-', 'dbg-', 'l-', 'test', 'ctest', 'fuzz')
EXCLUDE_FILES = {'tileweb.cc', 'tileweb.h', 'tags.cc', 'tags.h', 'lang-fake.cc',
                 'lang-fake.h', 'wizard.cc', 'wizard.h'}

# リテラルがこの関数の引数にあれば包み済みとみなす
WRAP_FUNCS = set(jtools.JTRANS_FUNCS) | set(jtools.TAGGED_FUNCS) | set(
    jtools.TAG_MACROS)
WRAP_RE = re.compile(r'\b(\w+_jc?|' + '|'.join(sorted(WRAP_FUNCS)) + r')\s*\(')

# この関数の引数のリテラルは見ない(デバッグ・内部エラー・キー)
IGNORE_CALLS = {
    'dprf', 'dprintf', 'ASSERT', 'ASSERTM', 'die', 'die_noline', 'end',
    'fprintf', 'printf', 'sprintf', 'snprintf', 'fputs', 'puts', 'perror',
    'debug_dump_item', 'debug_dump_mon', 'mprf_nolog', 'save_game',
    'lua_pushstring', 'lua_getfield', 'lua_setfield', 'lua_pushliteral',
    'luaL_error', 'luaL_argerror', 'luaL_checkudata', 'luaL_newmetatable',
    'luaL_openlib', 'luaL_register', 'clua_register_metatable', 'lua_getglobal',
    'lua_setglobal', 'LUAWRAP', 'LUAFN', 'LUARET1', 'luaterp_', 'execfile',
    'callfn', 'callbooleanfn', 'callmbooleanfn', 'callmaybefn', 'fnwrap',
    'exists', 'erase', 'get_int', 'get_bool', 'get_string', 'get_vector',
    'get_table', 'get_float', 'get_short', 'get_byte', 'get_coord',
    'get_monster', 'get_item', 'get_level_id', 'get_int64', 'get_flags',
    'starts_with', 'ends_with', 'strip_suffix', 'strip_tag', 'strip_tag_prefix',
    'strip_number_tag', 'strstr', 'strncmp', 'strcmp', 'strcasecmp', 'stricmp',
    'find', 'rfind', 'find_first_of', 'find_last_of', 'find_first_not_of',
    'contains', 'count', 'replace_all', 'replace_all_of', 'split_string',
    'getLongDescription', 'getShortDescription', 'getQuoteString',
    'getSpeakString', 'getRandNameString', 'getWeightedSpeechString',
    'getMiscString', 'getHelpString', 'getGameStartDescription',
    'getHintString', 'getFAQ_Question', 'getFAQ_Answer', 'getRandMonNameString',
    'getResistsDescription', 'datafile_path', 'catpath', 'fopen_u',
    'file_exists', 'dir_exists', 'lock_file_handle', 'unlink_u', 'rename_u',
    'read_init_file', 'read_option_line', 'read_options', 'add_option',
    'BoolGameOption', 'ColourGameOption', 'CursesGameOption', 'IntGameOption',
    'StringGameOption', 'ListGameOption', 'MultipleChoiceGameOption',
    'ColourThresholdOption', 'TileColGameOption', 'opt_colour_names',
    'mark_milestone', 'ouch', 'hurt', 'set_hurt', 'set_new_level',
    'xom_is_stimulated', 'did_god_conduct', 'set_dungeon_events',
    'add_cast_property', 'ghost_spell', 'tile_check', 'tile_name',
    'tile_draw_floor', 'tile_index', 'tileidx_', 'bind_key', 'key_to_command',
    'command_to_key', 'name_to_command', 'zap_by_name', 'str_to_ench',
    'str_to_mons', 'str_to_species', 'str_to_job', 'str_to_god',
    'str_to_branch', 'str_to_feature', 'str_to_shoptype', 'str_to_monster',
    'str_to_skill', 'dungeon_feature_by_name', 'get_monster_by_name',
    'spell_by_name', 'skill_from_name', 'mutation_from_name', 'branch_by_abbrevname',
    'branch_by_shortname', 'get_species_by_abbrev', 'get_job_by_abbrev',
    'environment_var', 'getenv', 'setenv', 'webtiles_send', 'json_write_string',
    'json_open_object', 'json_open_array', 'json_close_object', 'json_close_array',
    'write_message', 'json_write_int', 'json_write_bool', 'json_write_null',
    'json_write_name', 'json_treat_as_empty', 'jsonfield', 'JsonNode',
    'tiles_send', 'send_message', 'push_ui_layout', 'pop_ui_layout',
    'ui_state_change', 'msgwin_set_temporary', 'print_hint', 'take_note_debug',
    'fake_msg', 'lua_error', 'dlua_', 'crawl_state', 'make_name',
    'record_char_stat', 'add_milestone', 'update_whereis', 'dump_section',
    'is_wizard', 'ghost_demon', 'regex_match', 'pattern_match', 'text_pattern',
    'stringize_glyph', 'unwind_var', 'process_command', 'set_ident_flags',
    'game_options', 'opt_name', 'FixedVector', 'TRY_CAST', 'COMPILE_CHECK',
    'static_assert', 'map_def', 'mapdef', 'dgn_event', 'get_uniq_map_tags',
    'you_worship', 'mon_enchant', 'debuglog', 'log', 'logline',
    'add_stat', 'achieve', 'fill_dump_section', 'dump_item_origin',
    'lev_mon_tags', 'item_kind', 'base_type_string', 'parse_spells',
    'parse_level_id', 'keyed_mapspec', 'set_feature', 'add_feature',
    'mutate', 'temp_mutate', 'perma_mutate', 'malmutate', 'delete_mutation',
    'is_valid', 'check_jtrans', 'printint', 'add_tag', 'has_tag', 'tag',
    # DB の検索キー・辞書を引く関数
    '_get_xom_speech', 'XOM_SPEECH', 'getShoutString', 'hands_act',
    'tutorial_msg', 'print_hint',
    # 原因・blame(セーブ・スコアに残る)
    'mons_add_blame', 'miscast_effect', 'paralyse', 'poison_player', 'banish',
    'set_non_actor_summoner', 'create_malign_gateway', 'internal_ouch',
    # 内部エラー・ログ
    'DEBUG_ENERGY_USE_REF', 'check_mkdir', 'sysfail', 'fail', 'FAIL',
    '_ghost_dprf', 'unsafe_path', 'unsafe_path_f', 'bad_level_id',
    'bad_level_id_f', 'bad_map_marker', 'bad_map_marker_f',
    'dgn_veto_exception', '__android_log_print', 'error',
    'item_name_en', 'name_en',
}
# 呼び出し名の前方一致で見ないもの
IGNORE_PREFIX = ('lua', 'clua', 'dlua', 'json_', 'tile', 'debug_', 'dump_',
                 'wiz', 'str_to_', '_dump', '_debug', 'DUMP', 'LUA', 'dbg',
                 'webtiles', 'TILE', 'Tile', 'ASSERT', '_ASSERT')

# 1 語のリテラルも候補にする表示文脈
DISPLAY_CALLS = {
    'mpr', 'mprf', 'mprf_nocap', 'mpr_nojoin', 'simple_monster_message',
    'simple_god_message', 'god_speaks', 'yesno', 'yes_or_no', 'cprintf',
    'textcolour_printf', 'MenuEntry', 'ToggleableMenuEntry', 'set_title',
    'add_entry', 'set_more', 'set_text', 'Text', 'formatted_string',
    'parse_string', 'cgotoxy_print', 'msgwin_prompt', 'prompt_for_string',
    'msgwin_get_line', 'msgwin_get_line_autohist', 'cancellable_get_line',
    'show_description', 'describe_info', 'title', 'prompt', 'add_text',
    'add_raw_text', 'set_prompt', 'set_title_text', 'make_stringf',
    'TextItem', 'MenuItem', 'MenuDescriptor', 'set_highlighter', 'Popup',
    'add_item', 'add_button', 'add_entry_to_menu', 'show_popup',
}

COLOUR_TAG = re.compile(r'</?[a-z_A-Z0-9 ]*>')
FMT = re.compile(r'%[-+ #0-9.*]*(?:l|ll|z|h)?[sdifcuxXeEgGp%]')
FILE_LIKE = re.compile(r'^[\w./\\-]+\.(?:txt|des|lua|cc|h|rc|png|cs|lst|json|'
                       r'db|txt\.\w+|ttf|js|html|css)$', re.I)


def english_like(s, single_ok):
    t = COLOUR_TAG.sub(' ', s)
    t = FMT.sub(' ', t)
    t = re.sub(r'@\w+@|\$[A-Za-z_]+', ' ', t)
    if re.search(r'[^\x00-\x7f]', t):
        return False  # 既に日本語
    words = re.findall(r"[A-Za-z][A-Za-z'-]*", t)
    if not words:
        return False
    if FILE_LIKE.match(s.strip()):
        return False
    if re.fullmatch(r'\s*[\w:.\[\]-]+\s*', t) and ('_' in t or '::' in t):
        return False  # 識別子
    # 単語 2 つ以上(空白区切り)
    real = [w for w in words if len(w) >= 2]
    if len(real) >= 2 and re.search(r'[A-Za-z]\S*\s+\S*[A-Za-z]', t):
        return True
    if single_ok and real and len(real[0]) >= 3:
        return True
    return False


def skip_regions(code):
    """#ifdef DEBUG* / WIZARD などの中を空白に置き換える。"""
    lines = code.split('\n')
    stack = []  # (このブロックを飛ばすか, 条件がデバッグ系か)
    out = []
    for ln in lines:
        s = ln.strip()
        m = re.match(r'#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)', s)
        skipping = any(x[0] for x in stack)
        if m:
            kw, cond = m.group(1), m.group(2)
            dbg = bool(re.search(r'DEBUG|WIZARD|DGAMELAUNCH_DEBUG|USE_TILE_WEB|ARENA_VERBOSE',
                                 cond)) and 'NDEBUG' not in cond
            if kw in ('if', 'ifdef'):
                stack.append([dbg and '!' not in cond, dbg])
            elif kw == 'ifndef':
                stack.append([False, dbg])
            elif kw == 'else':
                if stack:
                    stack[-1][0] = (not stack[-1][0]) if stack[-1][1] else False
            elif kw == 'elif':
                if stack:
                    stack[-1] = [dbg, dbg]
            elif kw == 'endif':
                if stack:
                    stack.pop()
            out.append(' ' * len(ln))
            continue
        out.append(' ' * len(ln) if skipping else ln)
    return '\n'.join(out)


def paren_spans(code, pat):
    """pat に一致する呼び出しの ( ... ) の範囲 [(開始, 終了)] を返す。"""
    spans = []
    for m in pat.finditer(code):
        start = m.end() - 1
        depth, i, n = 0, start, len(code)
        while i < n:
            c = code[i]
            if c == '"' or c == "'":
                j = i + 1
                while j < n and code[j] != c and code[j] != '\n':
                    j += 2 if code[j] == '\\' else 1
                i = j + 1
                continue
            if c in '([{':
                depth += 1
            elif c in ')]}':
                depth -= 1
                if depth == 0:
                    break
            i += 1
        spans.append((start, i))
    spans.sort()
    return spans


def in_spans(pos, spans):
    # 単純な線形探索(ファイル単位なので十分速い)
    for a, b in spans:
        if a > pos:
            return False
        if a < pos < b:
            return True
    return False


def enclosing_call(code, pos):
    """pos を囲む最も内側の呼び出し名と、その外側の呼び出し名の一覧。"""
    names = []
    depth = 0
    i = pos - 1
    limit = max(0, pos - 3000)
    while i >= limit:
        c = code[i]
        if c == '"':
            # 文字列の中を飛ばす(後ろから)
            j = i - 1
            while j >= limit and not (code[j] == '"' and code[j - 1] != '\\'):
                j -= 1
            i = j - 1
            continue
        if c in ')]':
            depth += 1
        elif c == '}':
            if depth == 0:
                break
            depth += 1
        elif c in '([':
            if depth == 0:
                if c == '[':
                    names.append('[]')
                else:
                    m = re.search(r'([A-Za-z_][\w:.>-]*)\s*$', code[limit:i])
                    nm = m.group(1) if m else ''
                    nm = re.split(r'::|\.|->', nm)[-1]
                    names.append(nm or '(')
            else:
                depth -= 1
        elif c == '{':
            if depth == 0:
                names.append('{')
                # 初期化子 { ... } の中ならその外も見る
                prev = code[limit:i].rstrip()
                if not prev.endswith((')', 'else', 'do', 'try')) and \
                        not re.search(r'\b(struct|class|enum|namespace)\b[^;{}]*$',
                                      prev):
                    i -= 1
                    continue
                break
            depth -= 1
        elif c == ';' and depth == 0:
            break
        i -= 1
    return names


def stmt_prefix(code, pos):
    """リテラルの直前(同じ文の中)の文字列。"""
    i = pos - 1
    while i > 0 and code[i] not in ';{}':
        i -= 1
    return code[i + 1:pos]


LIT_GROUP = re.compile(jtools.LITS)


def scan(path, d, d_untagged, show_all, single):
    with open(path, encoding='utf-8', errors='replace') as fp:
        raw = fp.read()
    code = jtools.strip_comments_keep_len(raw)
    code = skip_regions(code)
    wrapped = paren_spans(code, WRAP_RE)
    res = []
    for m in LIT_GROUP.finditer(code):
        pos = m.start()
        # 'x' の中の " や #include の行を除く
        ls = code.rfind('\n', 0, pos) + 1
        line_text = code[ls:code.find('\n', pos)]
        if re.match(r'\s*#\s*(include|pragma|error|warning)', line_text):
            continue
        if re.match(r'\s*#\s*define\s+\w*(KEY|PROP|NAME_|_TAG|TAG_)', line_text):
            continue
        if code[pos - 1:pos] == "'":
            continue
        s = jtools.unescape(m.group(0))
        names = enclosing_call(code, pos)
        inner = names[0] if names else ''
        disp = any(n in DISPLAY_CALLS for n in names[:3])
        if not english_like(s, single and disp):
            continue
        if in_spans(pos, wrapped):
            continue
        if any(n in IGNORE_CALLS or n.startswith(IGNORE_PREFIX) for n in names):
            continue
        if inner == '[]':
            continue
        before = code[max(0, pos - 4):pos]
        after = code[m.end():m.end() + 4]
        if re.search(r'[=!]=\s*$', before) or re.match(r'\s*[=!]=', after):
            continue
        if 'MSGCH_DIAGNOSTICS' in code[max(0, pos - 200):pos] and \
                inner in ('mprf', 'mpr'):
            continue
        prefix = stmt_prefix(code, pos)
        if re.search(r'\bprops\b|\bprop\b', prefix) and not disp:
            continue
        # 死因(スコアファイルに残る)・blame(セーブに残る)
        if re.search(r'\baux_source\s*=|\bkiller_name\s*=|\bdeath_source_name'
                     r'|\bblame\b', prefix):
            continue
        key = jtools.canon(s)
        indict = key in d or key in d_untagged
        # 表示系の呼び出しの直接の引数なら、辞書にあっても包み忘れ
        if indict and not show_all and inner not in DISPLAY_CALLS:
            continue
        if inner in ('', '{'):
            if re.search(r'\breturn\b[^;]*$', prefix):
                ctx = 'return'
            elif re.search(r'(?<![=!<>])=\s*[^;]*$', prefix):
                ctx = '='
            elif inner == '{':
                ctx = 'init'
            else:
                ctx = '?'
        else:
            ctx = inner
        if re.search(r'\?\s*$', code[max(0, pos - 30):pos]) or \
                re.match(r'\s*:', code[m.end():m.end() + 10]):
            ctx += '?:'
        line = raw.count('\n', 0, pos) + 1
        res.append((line, ctx, s, indict))
    return res


# 表示関数に変数を渡している呼び出し(mpr(msg), mprf("%s", msg.c_str()) など)
VAR_CALL = re.compile(
    r'\b(mpr|mprf|mprf_nocap|mpr_nojoin|simple_monster_message|'
    r'simple_god_message|god_speaks|yesno|yes_or_no|set_title|set_more|'
    r'set_text|add_entry|MenuEntry|cprintf|msgwin_prompt|prompt_for_string|'
    r'print_formatted_paragraph|message|error)\s*\(')
VAR_SKIP = {'nullptr', 'true', 'false', 'you', 'mons', 'mon', 'god', 'channel',
            'chan', 'ch', 'param', 'i', 'n', 'num', 'count', 'colour', 'col'}


def scan_vars(path, single):
    """表示関数に渡す変数に、同じ関数内で包まれていない英文を代入している所。"""
    with open(path, encoding='utf-8', errors='replace') as fp:
        raw = fp.read()
    code = skip_regions(jtools.strip_comments_keep_len(raw))
    wrapped = paren_spans(code, WRAP_RE)
    calls = paren_spans(code, VAR_CALL)
    res = []
    seen = set()
    for start, end in calls:
        whole = code[start:end + 1]
        name = re.search(r'(\w+)\s*$', code[max(0, start - 40):start]).group(1)
        for vm in re.finditer(r'(?<![\w.>"\'])([A-Za-z_]\w*)'
                              r'(?:\.c_str\(\))?\s*(?=[,)])', whole):
            v = vm.group(1)
            if v in VAR_SKIP or v.isupper() or \
                    v.startswith(('MSGCH_', 'GOD_', 'MSG', 'DESC_')):
                continue
            # 呼び出しの中で v が jtrans に包まれていれば訳される
            if in_spans(start + vm.start(), wrapped):
                continue
            # 関数の先頭(行頭の '{')まで遡って代入を探す
            fstart = code.rfind('\n{', 0, start)
            lo = max(0, fstart, start - 8000)
            seg = code[lo:start]
            for am in re.finditer(r'\b' + re.escape(v) + r'\s*\+?=\s*('
                                  + jtools.LITS + ')', seg):
                pos = lo + am.start(1)
                if in_spans(pos, wrapped):
                    continue
                txt = jtools.unescape(am.group(1))
                if not english_like(txt, single):
                    continue
                line = raw.count('\n', 0, pos) + 1
                if line in seen:
                    continue
                seen.add(line)
                cline = raw.count('\n', 0, start) + 1
                res.append((line, f'{v} -> {name}:{cline}', txt, False))
    return res


def main(argv):
    show_all = single = summary = vars_mode = False
    files = []
    extra_ex = set()
    it = iter(argv)
    for a in it:
        if a in ('-a', '--all'):
            show_all = True
        elif a in ('-w', '--words'):
            single = True
        elif a in ('-s', '--summary'):
            summary = True
        elif a in ('-v', '--vars'):
            vars_mode = True
        elif a == '-x':
            extra_ex.add(next(it))
        else:
            files.append(a)
    if files:
        paths = jtools.source_files(files)
    else:
        paths = [p for p in jtools.source_files([])
                 if not os.path.basename(p).startswith(EXCLUDE_PREFIX)
                 and os.path.basename(p) not in EXCLUDE_FILES
                 and os.path.basename(p) not in extra_ex]
    d = jtools.load_dict()
    # [タグ]英文 の形のキー(データ表の名前など)も訳ありとみなす
    d_untagged = {k.split(']', 1)[1].strip() for k in d
                  if k.startswith('[') and ']' in k}
    total = 0
    rows = []
    for p in paths:
        if vars_mode:
            res = scan_vars(p, single)
        else:
            res = scan(p, d, d_untagged, show_all, single)
        if not res:
            continue
        total += len(res)
        rows.append((p, res))
    for p, res in rows:
        name = os.path.basename(p)
        if summary:
            print(len(res), name)
            continue
        for line, ctx, s, indict in res:
            mark = ' (辞書にあり)' if indict else ''
            print(f'{name}:{line}  [{ctx}]  {s!r}{mark}')
    print('合計', total, file=sys.stderr)


if __name__ == '__main__':
    main(sys.argv[1:])
