# emuz80_pico2 ... A Z80 manipulator on Raspberry Pi Pico2

かの名作 EMUZ80 を、PIC18F47Q43 の代わりに Raspberry Pi Pico2 を使って実現します。他のレトロCPU駆動機を実現するための手始め・Pico2 (とくに PIO)のトレーニングも兼ねたプロジェクトです。

## 2026/06/18 取り急ぎ Fork しました  @DragonballEZ
 - dma_rb2350bブランチを元に weact RP2350B-CoreBoard(AE-RP2040ピンコネ変換)用に修正しました。
 - コンパイル済み uf2ファイルあり。

### ボード

<img width=400 src="./img/img1-2.jpg"/>

- SBCボードは [RP2350B_CoreBoard-AE_PCB(変換基板)](https://github.com/kyo-ta04/RP2350B_CoreBoard-AE_PCB) + [WeActStudio RP2350B CoreBoard](https://github.com/WeActStudio/WeActStudio.RP2350BCoreBoard/blob/main/HDK/RP2350B_PINOUT.png) ([回路図](https://github.com/WeActStudio/WeActStudio.RP2350BCoreBoard/blob/main/HDK/RP2350B_SCH.pdf))  です。
- ベースボードは [EMUZ80_RP2040_PCB](https://github.com/tendai22/EMUZ80_RP2040_PCB) です。 -> [オレンジピコショップ](https://store.shopping.yahoo.co.jp/orangepicoshop/pico-a-100.html?sc_i=shopping-pc-web-top-store-rcmitem-itm_lnk)
- 抵抗は 1KΩ、クロックは 50～500Ω (クロック周波数等で変わるようなので、その場で要調整)


### 参考回路図

<img width=400 src="./img/img1-3.jpg"/>

<br>
<br>


### 実行結果

<img width=400 src="./img/img1-1.png"/>

 - Z80クロック 25MHzで ASCIART完走しました、クロックの抵抗は ~~330~~ 510Ω(**わりと不安定**)。  
<br>
<br>


----


Z80 駆動に先立ち、必要な要素機能の動作確認も兼ねて、ROM エミュレータの試作も行いました。その試行結果は、[ROM_EMULATION.md](doc/ROM_EMULATION.md) に記載してあります。ソースコードはブランチ `rom_emulation` を参照ください。

試作基板と回路図はこんな感じです。

<div style="frame">

<img height=250 src="img/001-board.JPEG"/>
<img height=250 src="img/006-schematics.jpg"/>

Pico2 の開発環境を整えて、PIO プログラミングを試行し、WAIT 発生回路、クロックジェネレータ、UARTシミュレーションを作成し、EMUBASIC を動作させ、ASCIIART.BAS を走らせるところまで進めました。

<img height=350 src="img/005-asciiart_12MHzCLK.png"/>

20MHz Z80 で、12.5MHz クロックを供給し、ASCIIART.BAS を実行すると、175secで走り切ります。WAITサイクルが適宜入る(M1サイクル/MemReadサイクルで3Wait追加される)ので、クロック相応よりも遅くなります。ノーウェイト動作のとき、クロック 20MHz で 1分が相場なので、175sec は 6.87MHz 相当になります。

### 状況・使い方

* EMUBASIC 実行ができるようになりました。
* メモリ64kB、Read/Write できます(MREQからのアクセスタイム 260ns)
* Z80 クロックは可変です。
  + 定数書き直して再コンパイル。62.5MHz に対する分周比を浮動小数点数で指定します。
  * クロック 3MHz あたりから WAIT サイクルが入り始めます。
  * クロック 12MHz まで動作確認しています。16MHz では動作しませんでした(WAIT挿入が間に合わず、「すっこ抜ける」)
* シリアルポート機能(メモリマップドI/O)
  + EMUZ80 に合わせてあります。
  + E000H:RO: ステータスレジスタ(bit0: RxRDY, bit1: TxRDY)
  + E001H:RW: データレジスタ
* EMUBASIC ファームを RAM 配列に転送して、Z80 リセット解除・実行開始させます。

* 電源はマイクロUSBケーブルから供給します。
* ファームウェアもマイクロUSB接続で焼きこみます。
* 例の6ピンUSB-シリアルを繋ぎます。3.3V版を使ってください。5V版ダメ絶対。
* 上スイッチが BOOTROM, 下スイッチが RESET です。上スイッチ押しながら下スイッチ押して離すとファーム焼きmodeに突入します。
* 115200bps で TeraTerm を繋いで、RESET(下スイッチ)を押すと EMUBASIC が起動します。
* 文字待ち 1ms, 行待ち 10ms ぐらいで ASCIIART.BAS をアップロードして、RUN すると ASCIIART が描かれます。


開発過程は[メモ](doc/LOG.md)に残しています。

### 契機

私は、Z80 を現代マイクロコントローラの組み合わせで動作させ、古の Z80 プログラムを実行させる試みをいままで行ってきました。出会った試みの中で、EMUZ80 は特に印象深いものがありました。

[EMUZ80](https://vintagechips.wordpress.com/2022/03/05/emuz80_reference/) は電脳伝説さん(@vintegechips)の名作で、Z80 と PIC18F47Q43 の2チップだけで構成したシンプルかつエレガントな作品です。これにインスパイヤされて、多くの人がさまざまなレトロCPUを載せて動かしています。

EMUZ80 は、PIC の CLC(Configurable Logic Cells) を生かした WAIT 発生回路を特徴としています。ただ、内蔵 RAM が小さく、RAM 6kBほどのプログラムしか動かせませんでした。また、Z80 停止以外の処理を PIC ソフトで実現しており、PIC18 自体の実行速度もそれなりなので、1命令あたりの実行時間がそれなりに掛かっていたことを、個人的には少し残念に思っていました。

Raspberry Pi Pico は、Cortex-M3, 125MHz 駆動とそれなりに速く、RAM もふんだんに積んでおり、64kB の配列を確保することも十分可能です。なんといっても、PIO(Programmable I/O)という高速応答を行う仕組みがあるため、WAIT 発生回路を PIO で組むことができれば面白いのにと考えていました。が、自力で道を切り開く根性もなく手つかずのままになっておりました。そんなある日、私のタイムラインにさっそうと流れてきたのが、[このツイート](https://x.com/nf_ban/status/1823367824691052579)です。

<img width=400 src="img/003-great-approach.png"/>

ここに `wait 0, gpio 25` って書いてある。これって MREQ が 0 になるのを待つということだよね。

これを見て、これなら私も勉強すれば PIO 使えるんじゃないかと思ったわけですよ。それで、以前買っておいた [Pimoroni PGA2350](https://shop.pimoroni.com/products/pga2350?variant=42092629229651)を引っ張り出してごそごそと作ったのが、このリポジトリとなります。


