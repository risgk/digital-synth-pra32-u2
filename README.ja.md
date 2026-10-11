# Digital Synth PRA32-U2 v3.12.0

- 2026-10-11 ISGK Instruments
- <https://github.com/risgk/digital-synth-pra32-u2>
- [English README](./README.md) の抄訳です (英語版が正です)。詳しい仕様は英語版を参照してください
    - 英語のドキュメントは、必要に応じて Web ブラウザーなどの自動翻訳を使ってお読みください


## 概要

- **PRA32-U2** は、Raspberry Pi Pico 2/RP2350 用の 4 ボイス・ポリフォニック・シンセサイザーです
    - Chorus FX と Delay/Reverb FX を内蔵し、その後に Output Limiter があります
    - MIDI で制御します -- PRA32-U2 は MIDI 音源モジュールです
    - パラメーターをユーザー・プログラムとフラッシュに書き込む機能があります
    - PRA32-U2 は PRA32-U (Raspberry Pi Pico/RP2040 用) の後継として始まりましたが、今では別のシンセサイザーです
- **I2S DAC** ハードウェア (例えば Pimoroni Pico Audio Pack) が必要です
    - I2S の代わりに PWM Audio も使えます (英語版の [PWM Audio](./README.md#pwm-audio-optional) を参照)
- ビルド済みの UF2 ファイル ("bin" フォルダー内): 改造せずに使うだけなら、Arduino IDE は不要です
    - BOOTSEL ボタンを押しながら Raspberry Pi Pico 2 を PC につなぎ、UF2 ファイルを "RP2350" ドライブにコピーしてください
    - PRA32-U2/M (推奨): "Digital-Synth-PRA32-U2-M-Pimoroni-Pico-Audio-Pack.uf2" は Raspberry Pi Pico 2 と Pimoroni Pico Audio Pack 用です
        - PRA32-U2 の上位互換です: Main Synth (Basic Channel + 0) は PRA32-U2 と同じように動き、Sub Synth と Layering が加わります
        - 注意: MIDI チャンネル 2-4 と 14-16 にも反応します (初期設定)。これらのチャンネルを他の機器に使う場合は、PRA32-U2 を使ってください
    - PRA32-U2: "Digital-Synth-PRA32-U2-Pimoroni-Pico-Audio-Pack.uf2" は Raspberry Pi Pico 2 と Pimoroni Pico Audio Pack 用です
- Arduino IDE と Arduino-Pico (Earle F. Philhower, III 氏による) で改造できます: [改造の準備](#改造の準備) を参照してください (例えば他の I2S DAC、PWM Audio、パネルを使う場合)
- オプション
    - **PRA32-U2/M** (PRA32-U2 Multi-Timbre Edition) も構成できます (下記)
    - 部品を追加することで、**[PRA32-U2/P](./README-PRA32-U2-P.md)** (PRA32-U2 with Panel、英語) と **PRA32-U2/M/P** も構成できます
- **[PRA32-U2 Native](./PRA32-U2-Native/README.md)** (英語): PC 用のソフトウェア・シンセサイザー版 PRA32-U2 (VST3 プラグインとスタンドアロン・アプリケーション)
- **[PRA32-U2 Web](./PRA32-U2-Web/README.md)** (英語): Web ブラウザーで動くソフトウェア・シンセサイザー版 PRA32-U2 (PRA32-U2 Editor に内蔵)

![PRA32-U2 (Pico Audio Pack)](./pra32-u2-pico-audio-pack.jpg)


## ドキュメント (英語)

- [Change History](./PRA32-U2-Change-History.md) (変更履歴)
- [Parameter Guide](./PRA32-U2-Parameter-Guide.md) (パラメーター・ガイド)
    - パラメーター名は、PRA32-U2 Editor とパネルの表示に合わせて英語のままです
- [MIDI Implementation Chart](./PRA32-U2-MIDI-Implementation-Chart.md) (MIDI インプリメンテーション・チャート)
- README の次の節は英語版のみです
    - [Synthesizer Block Diagram](./README.md#synthesizer-block-diagram)、[Filter Diagrams](./README.md#filter-diagrams)、[Wave Table Graphs](./README.md#wave-table-graphs)
    - [Features](./README.md#features) (USB MIDI、UART MIDI、I2S、オーディオ・バッファー、PWM Audio の設定)
    - [Files](./README.md#files)、[Simple Circuit for PWM Audio](./README.md#simple-circuit-for-pwm-audio-optional)、[Customization Examples](./README.md#customization-examples)


## 改造の準備

- **Arduino IDE** をインストールしてください
    - 注意: Update Method: "Default (UF2)" 以外を使うと、スケッチの書き込み中に大きなノイズが出ます
- Arduino-Pico = **Raspberry Pi Pico/RP2040/RP2350** (Earle F. Philhower, III 氏による) コアをインストールしてください
    - 動作確認したバージョンは英語版の [Preparation for Modification](./README.md#preparation-for-modification) を参照してください
- Arduino **MIDI Library** (Francois Best 氏、lathoub 氏による) をインストールしてください
- Arduino IDE の "ツール" メニューで USB Stack: "Adafruit TinyUSB" を選んでください (USB MIDI 用)


## PRA32-U2 Editor

- "pra32-u2-editor.html": PRA32-U2 用のエディター (MIDI コントローラー) アプリケーション、HTML アプリ (Web アプリ)
    - MIDI チャンネルは "MIDI Ch" で選んでください (初期値は 1)
- Web MIDI API に対応した Google Chrome か Microsoft Edge をお勧めします
    - その他の Web ブラウザー (例えば Safari) については、[PRA32-U2 Web](./PRA32-U2-Web/README.md) の "Web browsers" (英語) を参照してください
    - Safari (Mac、iPhone、iPad) と iPhone・iPad のすべての Web ブラウザーは Web MIDI API に対応していないため、
      PRA32-U2 Editor は PRA32-U2 Web (Built-in) でだけ使えます (PRA32-U2 や他の MIDI 機器とはつながりません)
- "MIDI Out" のリストで "PRA32-U2" を選んでください
    - PRA32-U2 なしで [PRA32-U2 Web](./PRA32-U2-Web/README.md) を使うには、"PRA32-U2 Web (Built-in)" を選んでください
- PRA32-U2 Editor を経由すると、プログラム・チェンジの扱いが変わります
    - "MIDI In" から (Editor の MIDI Ch で) 届いたプログラム・チェンジは PRA32-U2 に送らず、Editor がプリセットを読み込んで、そのパラメーターをコントロール・チェンジで送ります
    - #0-15 は Editor のユーザー・プリセット (ブラウザーに保存) で、PRA32-U2 のプログラム #0-15 (Write で書き込んだもの) ではありません
    - #16-31 はファクトリー・プリセットです (PRA32-U2 のプログラム #16-31 と同じ)
    - PRA32-U2 自身のプログラムを選ぶには、Editor を経由せずにプログラム・チェンジを送るか、"Save & Load" の Program Change ボタンを使ってください
- 主な機能
    - ファクトリー・プリセット (#16-31) とユーザー・プリセット (#0-15) の読み込み
    - ユーザー・プリセットはブラウザーに保存され、Store で 10 文字までの名前 (ASCII の英数字と記号) を付けられます
    - JSON ファイルへのエクスポートと、JSON ファイルからのインポート
    - Undo: 直前のプリセット、Randomize、Import の前のパラメーターに戻します (もう一度押すと Redo)
    - Software Keyboard: マウス、タッチ、PC のキーボード (A - K が C - C、W - U が黒鍵、Z / X がオクターブ) で弾けます
        - ノートオンとノートオフのベロシティを入力できます (1-127、初期値は 100 と 64)
        - 押したままマウスを鍵盤の上か下に出すと、ノートが鳴り続けます (Panic で止まります)
        - 鍵盤の下のスライダーで、Pitch Bend (離すと中央に戻ります)、Modulation、Breath Controller、Expression、Sustain Pedal、Channel Pressure を送れます
    - Write: 現在のパラメーターを PRA32-U2 (プログラム #0-15 とフラッシュ) に書き込みます (下記)
- PRA32-U2 Editor を使わない場合 (例えば DAW や MIDI キーボードから)
    - PRA32-U2 は、PRA32-U2 Editor を使わずに MIDI で制御することもできます
    - Editor で作った音を PRA32-U2 に書き込んでおけば、後で Editor なしで呼び出せます
        1. Write: Editor の "Save & Load" にある Write ボタン (#0-15) で、現在のパラメーターを PRA32-U2 のプログラムに書き込みます
           (フラッシュにも書き込まれるので、電源を切っても残ります)。書き込みの前に、Editor に表示されているパラメーターを送り直します
        2. Editor を経由せずに、PRA32-U2 にプログラム・チェンジ #0-15 を直接送ると、そのプログラムを呼び出せます
    - 電源を入れたときのプログラムは #0 です
    - 対応している機能は [MIDI Implementation Chart](./PRA32-U2-MIDI-Implementation-Chart.md) (英語) を参照してください


## PRA32-U2/M (PRA32-U2 Multi-Timbre Edition) (オプション)

- Basic Channel + 0 (初期値 1) が Main Synth、+ 1 から + 3 (初期値 2-4) が Sub Synth 1-3 (Mono) です
    - Sub Synth は、Main Synth が Mono 系のモードのときだけ処理されます
- Layering: Basic Channel - 3 から - 1 (初期値 14-16) で、複数のシンセを同時に制御します
- 詳しくは英語版の [PRA32-U2/M](./README.md#pra32-u2m-pra32-u2-multi-timbre-edition-optional) を参照してください


## ライセンス

- CC0 1.0 です。正式な文面は英語版の [License](./README.md#license) を参照してください
