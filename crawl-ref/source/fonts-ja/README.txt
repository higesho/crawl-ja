日本語版で同梱する日本語フォント

BIZUDGothic-Regular.ttf
  BIZ UDゴシック Regular(Morisawa)。SIL Open Font License 1.1(OFL.txt)。
  出どころ: https://github.com/googlefonts/morisawa-biz-ud-gothic
           の fonts/ttf/BIZUDGothic-Regular.ttf(2026-10-19 取得)
  SHA-256: 7d2b48d84ef4e65c9f85bcf65fb1fb41c92b134be641fab6a7141d597c9a92b0

使い道
  Android 版(make ANDROID=1 android)で APK の assets/dat/tiles に入れ、
  起動画面が作る init ファイルで tile_font_*_file に指定する。
  Android の端末のフォント(Noto Sans CJK)は新しい端末では CFF2 形式で、
  同梱の freetype 2.5.3 では読めないため、TrueType の輪郭を持つこのフォントを使う。
  ライセンス文は assets/docs/license/BIZUDGothic-OFL.txt として一緒に入れる。

  Windows 版は Windows に標準で入っている BIZ UDゴシック(C:/Windows/Fonts)を
  使っているので、このファイルは使っていない。
