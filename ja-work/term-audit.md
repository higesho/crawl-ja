# 訳語の揺れの監査(第 8 陣・2026-10-19)

名前のデータ辞書 `jtrans/data-*.txt` の訳を正として、説明文・台詞 DB
(`dat/descript/ja/*.txt`、`dat/database/ja/*.txt`)と jtrans 辞書の訳文を照合した結果。

## 調べ方

1. data-* のキー(モンスター・アイテム・固定アーティファクト・神はタグなし、呪文 `[spell]`、
   技能 `[skill]`、種族 `[spec name]`、職業 `[job]`、支部 `[branch]`、地形 `[feature]`、
   雲 `[cloud]`、変身 `[form]`、ビーム `[zap]`)を英語名の一覧にした。
2. DB の各項目の「# en:」の英文と、jtrans の各キーの英文から、その英語名を最長一致で拾った
   (大文字始まりの固有名は大文字小文字を区別、それ以外は 2 語以上の名前とモンスター名。
   `@…@` の置換記号と `<…>` のタグの中は除外)。日本語の訳文に正の訳(『』・空白・中黒を除いて比較)
   が含まれていないものを候補にした(1,171 件)。候補ごとに訳文の中で正の訳に最も近い部分を
   自動で抜き出して、揺れか、一般語としての用法か、置換記号で別項目を引いているだけかを目で仕分けた。
3. 逆向きに、全訳文のカタカナ語を data-* の訳のカタカナ語と比べ、似ているが違う表記
   (ワカド/ウカド型)を拾った。
4. data-* どうしで同じ英語(大文字小文字を無視)に違う訳が付いているもの(202 組)を出した。
   大半は助数詞・支部の略称・形容詞タグ・称号など意図的な訳し分け。

一般語としての用法(例: "a tiny floating eye" を「小さく輝く浮遊する眼球」、"the dungeon" を
「ダンジョン」)、引用文(quotes.txt の英語のままの項目)、ランダム名の語彙は揺れとして扱っていない。

## 直したもの(66 件)

「# en:」のコメント行とキー行は変えず、訳文の行だけを置き換えた。`dbtools.py status` の結果
(訳あり 5,618 / 未訳 182)と `jtools.py dups`(重複 0)は変化なし。

### data-* 辞書(20 件)

| ファイル | キー | 旧 → 新 | 正とした訳 |
|---|---|---|---|
| data-monsters | barachi | バラキ → バラチ | `[spec name]Barachi`、STATUS の新種族名 |
| data-monsters | djinni | ジン → ジンニー | `[spec name]Djinni`(種族のモンスター) |
| data-monsters | [name adj]air elementalist | 大気の精霊使いの → 風の精霊使いの | `[job]Air Elementalist` |
| data-monsters | [name adj]fire elementalist | 炎の精霊使いの → 火の精霊使いの | `[job]Fire Elementalist` |
| data-monsters | [name adj]earth elementalist | 大地の精霊使いの → 地の精霊使いの | `[job]Earth Elementalist` |
| data-species | caustic breath | 腐食のブレス → 腐蝕のブレス | `[spell]Caustic Breath` |
| data-species | galvanic breath | 電撃のブレス → 電流のブレス | `[spell]Galvanic Breath`(ability.txt も電流) |
| data-species | breathe sticky flame splash | 粘着炎の吐きかけ → 焼夷の炎の吐きかけ | `[spell]Sticky Flame`・`[zap]sticky flame` |
| data-species | You can breathe blasts of explosive embers. | 燃えさし → 残り火 | `[zap]explosive embers` |
| data-items | medusa talisman | メドゥーサのタリスマン → メデューサのタリスマン | `[form]Medusa` |
| data-forms | You can turn into living lightning. | 生きた稲妻 → 生ける稲妻 | `[zap]living lightning` |
| data-forms | a swarm of vampire bats. | 吸血蝙蝠の群れ → 吸血コウモリの群れ | `[zap]swarm of vampire bats`、モンスター vampire bat |
| data-forms | Burstshroom damage | 爆裂茸 → 破裂キノコ | モンスター burstshroom |
| data-forms | Your melee attacks cause explosive burstshrooms to grow. | 爆裂茸 → 破裂キノコ | 同上 |
| data-forms | Frigid Wall Dmg | 凍壁ダメージ → 極寒の壁ダメージ | `[feature]frigid wall` |
| data-forms | siphon essence | 精気吸収 → 精髄の吸収 | `[spell]Siphon Essence`、`[ability]Siphon Essence` |
| data-forms | rust breath | 錆の息 → 錆のブレス | `[spell]Rust Breath`、`[ability]Breathe Rust` |
| data-durations | You are making an unholy racket with your haunted armour. | 呪われた鎧 → 取り憑かれた鎧 | モンスター haunted armour |
| data-durations | You are spewing a noxious bog. | 有毒な沼 → 毒沼 | `[feature]a toxic bog` |
| data-zaps | [zap]orb of destruction | 破壊の発露 → 破壊の爆裂球 | `[spell]Orb of Destruction`(0.19-ja の zap も同じ) |

### descript/ja(32 件)

| ファイル | キー | 旧 → 新 |
|---|---|---|
| monsters | ugly thing | 醜いもの → 醜きもの |
| monsters | very ugly thing | ひどく醜いもの → 甚だしく醜きもの |
| monsters | black bear | 黒い熊 → 黒熊 |
| monsters | raiju | 生きた稲妻 → 生ける稲妻 |
| monsters | bone dragon | 耐え難い苦悶 → 耐えがたい苦悶(雲の名前) |
| monsters | merfolk avatar | 溺死者の魂 → 水死者の魂(drowned soul) |
| items | protean talisman | メドゥーサ → メデューサ |
| items | horn of Geryon | 地獄の玄関口 → 地獄の入り口(Vestibule of Hell) |
| items | box of beasts | 生きた稲妻 → 生ける稲妻 |
| ability | Summon Divine Warrior ability | ディーヴァ → デーヴァ |
| tutorial | tutorial5 dungeon_overview | 怒れるトログ → 激怒せるトログ(Trog the Wrathful) |
| tutorial | tutorial3 wand_digging | 採掘のワンド → 掘削のワンド |
| clouds | bats cloud | 吸血蝙蝠の群れ → 吸血コウモリの群れ |
| status | Swarm status | 吸血蝙蝠の群れ → 吸血コウモリの群れ |
| status | Bog status | 有毒な沼 → 毒沼 |
| features | disjunction haloed | 転移のエネルギー → 転位のエネルギー(`[cloud]translocational energy`) |
| features | Zonguldrok's gravestone | ゾングルドロク → ゾングルドルック |
| spells | Shock / Electrical Bolt / Forge Lightning Spire spell | 電気の矢 → 電光の矢(`[zap]bolt of electricity`)3 件 |
| spells | Awaken Flesh spell | 肉塊の山 → 肉の山(pile of flesh) |
| spells | Hurl Torchlight spell | 陰鬱な松明の光 → 陰影の松明の光(`[zap]umbral torchlight`) |
| spells | Forge Monarch Bomb spell | 親玉爆弾 → 君主爆弾(2 か所。第 6 陣の統一の漏れ) |
| spells | Spectral Cloud spell | 幽体の霧 → 霊体の霧(spectral mist) |
| spells | Eringya's Noxious Bog spell | 有毒な沼 → 毒沼 |
| spells | Venom Bolt spell | 毒の矢 → 猛毒の矢(`[zap]bolt of poison`) |
| spells | Shadow Shard / Shadow Shot / Shadow Turret spell | 固めた影 → 硬化した影(shard of hardened shadow)3 件 |
| spells | Launch Sporangium spell | 腐蝕性の胞子嚢 → 焼酸の胞子嚢(caustic sporangium) |
| spells | Quicksilver Bolt spell | 魔力消去のエネルギーの矢 → 魔法解除の矢(`[zap]bolt of dispelling energy`) |
| mutations | Mark of Haemoclasm mutation | 血肉の雨 → 血の雨(`[zap]rain of gore`) |
| branches | Volcano | ファイアジャイアント → 炎の巨人 |

### database/ja(14 件)

| ファイル | キー | 旧 → 新 |
|---|---|---|
| montitle | Gloorx Vloq title | グルオルクス・ヴロック → グロオルクス・ヴロック |
| montitle | Jeremiah title | バラキの夢想家 → バラチの夢想家 |
| graffiti | _graffiti_species_ | バラキ → バラチ |
| graffiti | _graffiti_rumour_ | 死のクオッカ → 死のクワッカ |
| randbook | Air owner | 雲の魔術師 → 雲烟の魔術師(Cloud Mage) |
| help | wiz-monster | ラークシャサ → ラクシャーサ、(とても)醜いもの → (甚だしく)醜きもの、踊る武器 → 宙を舞う武器(2 か所) |
| monspeak | Crypt Donald | 呪いの髑髏に、呪いのつま先 → 呪いの頭蓋骨に、呪いの足指 |
| monspeak | Xom Donald | 踊る武器 → 宙を舞う武器 |
| monspell | Spectral Cloud revenant cast | 幽体の霧 → 霊体の霧 |
| rand_all | _thing_names_ | ジッグラト → ジグラット |
| rand_arm | _armour_animal_ | ヒッポグリフ → ヒポグリフ |

## 直していない揺れ(jtrans/<ソース名>.txt。担当が編集中のため)

| ファイル | キー | 今の訳 | 正の訳(出典) |
|---|---|---|---|
| ability.txt | You invoke the name of Kikubaaqudgha! | キクバークドゥグハ | キクバークッグァ(data-gods) |
| player.txt | Yredelemnul refuses to let your conquest be stopped… | イェレデルムヌル | イレデレンヌル(data-gods) |
| des.txt | Alistair's Party Mansion | アリステア | アリステイア(`[spell]Alistair's …`) |
| des.txt | A lost soul coalesces out of thin air! | 迷える魂 | 滅亡せる霊魂(data-monsters。下の「判断」も参照) |
| des.txt | The mighty Pandemonium lord %s… / A powerful Pandemonium lord… | パンデモニウムの王 | パンデモニウムの君主 |
| des.txt | [feature]flickering gateway to a bazaar | バザールへ続く… | バザールへと続く門 |
| des.txt | You step on to a pressure plate. | 踏み板 | 足踏みスイッチ |
| spl-cast.txt, spl-summoning.txt(9 件) | …clockwork bee… | 機械仕掛けの蜂 | 時計仕掛けの蜂(data-monsters) |
| spl-cast.txt | You stop channelling your searing ray. | 灼熱の光線 | 焼灼の光線(`[spell]Searing Ray`) |
| spl-cast.txt / spl-damage.txt | You exhale a blast of noxious fumes. / You spew toxic sludge! | 有毒な煙 / 毒の汚泥 | 悪臭の霧 / 有毒な汚泥 |
| spl-cast.txt | You breathe a blast of explosive embers. | 燃えさし | 残り火(data-species は直した) |
| spl-summoning.txt | There's no room for the soul wisp to form! | 魂の欠片 | 魂の鬼火 |
| spl-summoning.txt | %s fragments into a salvo of icicles! | 氷柱の一斉射撃 | 氷柱の斉射 |
| spl-damage.txt / spl-util.txt | spike launcher | 棘の射出装置 / スパイク発射機 | 棘射出機 |
| spl-goditem.txt / status.txt | fortress blast | 城塞の爆風 / 要塞砲撃 | 要塞の爆風 |
| ability.txt | You scatter into a swarm of vampire bats! | 吸血蝙蝠の群れ | 吸血コウモリの群れ |
| ability.txt / god-abil.txt(3 件) | Power Leap | 力強き跳躍 | 強力な跳躍(data-zaps)。能力名として別に決めたなら data-zaps 側を直す |
| ability.txt | [ability]Heal Other | 他者への癒し | 他者の治癒(`[spell]Heal Other`) |
| beam.txt | The orb of electricity explodes! / The ball of acid explodes! | 電撃の球体 / 酸の球 | 電撃の爆裂球 / 酸の爆裂球 |
| beam.txt / mon-info.txt | block of ice | 氷塊 | 氷の塊(data-monsters) |
| beam.txt | [beam name]calcifying dust | 石化の粉塵 | 石化する雲(data-zaps) |
| beam.txt / religion.txt | umbral torchlight | 冥い松明の光 / 陰りの松明の炎 | 陰影の松明(data-zaps) |
| beam.txt | The demonic tree burns… | 魔性の木 | 悪魔の樹 |
| mon-cast.txt(2 件) | A beckoning whirlwind … | 招き寄せる旋風 | 引き寄せの旋風 |
| mon-info.txt | [attr]surrounded by acidic fog | 酸の霧 | 酸性の霧(data-clouds) |
| monster.txt | [body part]front hooves | 前足の蹄 | 前の蹄 |
| directn.txt | [square]is bathed in translocational energy | 転移のエネルギー | 転位のエネルギー(data-clouds) |
| describe.txt | [miscast]summons a nameless horror | 名状しがたい恐怖 | 名状しがたき恐怖 |
| describe.txt | [form table]Frigid Wall Dmg | 凍てつく壁のダメージ | 極寒の壁ダメージ(data-forms は直した) |
| describe.txt | [form table]Bat Swarm Recharge | 蝙蝠の群れ | 蝙蝠群(`[form]Bat Swarm`) |
| describe.txt | [form table]Blinkbolt Dmg / Airstrike Dmg | ブリンクボルト / 空襲 | 雷霆の突進(または能力名の雷化)/ 旋風の一撃 |
| describe-god.txt | Jiyva will unlock the Slime Pits vaults. | スライムの穴 | スライムの穴ぐら |
| hiscores.txt | (glowing shapeshifter) | 輝く変身能力者 | 暴走した変身能力者 |
| potion.txt / evoke.txt | QUAD DAMAGE | 四倍ダメージボーナス / ダメージが4倍 | 四重ダメージ |
| player-notices.txt | (dancing weapon の行) | 踊る武器 | 宙を舞う武器 |
| fight.txt | (toxic bog の行) | 有毒な沼 | 毒沼 |
| xom.txt | Toadstools sprout up around you! / Demonic plants sprout up… | 毒キノコ / 悪魔の植物 | からかさキノコ / 禍々しい形の迷宮草 |
| xom.txt | [xom repel stairs]escape hatch | 脱出用ハッチ | 脱出用のハッチ(下の「判断」も参照) |
| xom.txt | banished to a bazaar | バザー | バザール |
| main.txt | A rune of Zot still resides in this realm… | 何らかのルーン | ゾットのルーン |
| hints.txt / travel.txt | Runed door / runed door | ルーンの刻まれた扉 / ルーンの扉 | ルーンで封印された扉 |

## 判断が要るもの(直していない)

1. **dungeon と Dungeon**: 支部名 `[branch]Dungeon` は「迷宮」だが、説明文・台詞・jtrans の地の文の
   "the Dungeon"(大文字を含む)はほぼすべて「ダンジョン」(約 50 か所)。ゲーム世界全体を指すときは
   「ダンジョン」、支部の名前としては「迷宮」という訳し分けとみなして残した。
2. **腐食/腐蝕**: 両方が広く混在(腐蝕: `[dur]corroded`=装備の腐蝕、`[spell]Corrosive Bolt`=腐蝕の矢、
   `[spell]Caustic Breath` など。腐食: `[zap]caustic explosion`=腐食性の爆発、data-branches・data-mutations・
   items/egos/unrand の説明文など。descript と jtrans で各 20 か所ほど)。常用漢字は「腐食」。どちらに
   揃えるかを決めてから一括で置換したい。今回は data-species のブレス名だけ呪文名(腐蝕)に合わせた。
3. **脱出ハッチ/脱出用のハッチ**: `[feature]escape hatch` は「脱出用のハッチ」だが、data-features の
   他の 2 項目(床・天井のハッチ)と features の説明文、hints・main・mon-behv・stairs が「脱出ハッチ」、
   xom が「脱出用ハッチ」。多数派の「脱出ハッチ」に正の訳を寄せる方が手数が少ない。
4. **扉の名前**: 地形名は「開かれた扉」「閉ざされた扉」だが、features の説明文(地形そのものの説明)、
   hints・tutorial、jtrans/hints・travel は「開いた扉」「閉じた扉」。地の文としては自然なので残した。
5. **蝙蝠/コウモリ**: モンスター(vampire bat=吸血コウモリ、bat=コウモリ)はカタカナ、変身(`[form]bat`=蝙蝠、
   `[form]Bat Swarm`=蝙蝠群)と雲(`[cloud]bats`=蝙蝠の群れ)は漢字。swarm of vampire bats は zap 名に
   合わせてカタカナにしたが、表記をどちらかに決めるとよい。
6. **data-* どうしの訳し分け**(0.19-ja から引き継いだものを含む。意図的かもしれない):
   - hurl damnation: 変異・能力「地獄の鉄槌」/ 呪文「天罰」(0.19-ja のまま)
   - spit poison: 変異「毒液」/ 呪文「毒の唾」(0.19-ja の能力名は「毒の唾液」)
   - condensation shield: 変異「凝結の盾」/ 呪文「大気の盾」
   - see invisible: 変異「霊視」/ 呪文「透明体の視認」
   - orb of destruction: モンスター「破壊の球体」/ 呪文・ビーム「破壊の爆裂球」(0.19-ja のまま)
   - porkalator: 呪文「豚化の光線」/ ビーム「豚化の魔法」(逆の方が自然)
   - seracfall: 呪文「氷塔の崩落」/ ビーム「氷塔崩し」。metal splinters: 呪文「鉄片の奔流」/ ビーム「金属片」
   - poison gas・noxious fumes: 雲「毒ガス」「悪臭」/ ビーム「猛毒のガス」「悪臭の霧」
   - gloom: 雲「暗闇」/ 呪文「薄闇」
   - blinkbolt: 能力・変身の特徴「雷化」/ 呪文「雷霆の突進」
   - sporulate: 変身「胞子を放つ」・特徴「胞子散布」/ 呪文「胞子形成」
   - sticky fireball: ビーム「粘着性の火球」(sticky は他では「焼夷」)
   - dispersal・disjunction: 矢弾のブランド「離散」「分解」/ 呪文「転送」「空間分離」
   - crystal spear: 呪文「レフディブの水晶槍」/ ビーム「水晶の槍」(describe-spells の短縮名・Vehumet の
     神罰の説明文は「水晶槍」)
   - force lance: 呪文「力場の槍」/ 固定アーティファクト「フォースランス」
   - 称号とモンスター名: royal mummy(王家のミイラ / 王族のミイラ)、undying armoury(不滅の武器庫 /
     不滅の武具庫)、laughing skull(笑う髑髏 / 笑う頭蓋骨)、hellbinder(地獄を呼ぶ者 / 地獄を束ねしもの)、
     worldbinder(世界を束ねし者 / ワールドバインダー)、meteorologist(気象の魔術師 / の気象学者)
   - summoner: 職業「召喚術師」/ 称号「召喚師」/ モンスター名の接尾辞「の召喚者」
   - shapeshifter: 職業「変身術師」/ モンスター「変身能力者」(別物なので訳し分けでよい)
7. **lost soul**: モンスター名は 0.19-ja の「滅亡せる霊魂」。des.txt の出現文と wpnnoise の台詞
   ("Among all these lost souls…" 一般語)は「迷える魂」。モンスター名自体を「迷える魂」に改める案もある。
8. **ランダム名の語彙**: rand_arm の _armour_animal_ の Centaur は「ケンタウロス」(モンスター名は
   「セントール」)。神話の一般語として残した。
9. **今回変えたうち確認してほしいもの**: data-monsters の djinni(ジン → ジンニー。種族のモンスター)と
   [name adj] の精霊使い 3 件(職業名に合わせた。彫像の名前「〜の精霊使いの彫像」で使う)、
   data-zaps の orb of destruction(破壊の発露 → 破壊の爆裂球)、data-forms の siphon essence・rust breath
   (変身の特徴名を呪文・能力名に合わせた)。
