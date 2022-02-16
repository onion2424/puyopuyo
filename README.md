
# 勝手にぷよ

「次へ」を押すと勝手に１手置いてくれるアプリです

機能は追加していく予定ですが、今は単純に連鎖を眺めるためのものです

# DEMO

![勝手にぷよ 2022-01-24 20-52-51_2](https://user-images.githubusercontent.com/28826492/150789795-23cbc933-f857-449a-8d8e-f94479f32f92.gif)

# Description

次の手を決めるのにネクスト、ネクストネクストのツモを見ています（全手知った上で組立てている訳ではありません）

配ツモは６４手均等の６４手ループです

ぷよぷよフィーバー以降に準拠し、１４段目に置いたツモは消えます

ぷよぷよ用語で言う"回し"を意識していないので、実際のゲームでは置けない位置に置くことがあります。

# Requirement

実行ファイルが置いてあります（windowsなら動くと思います）

https://github.com/onion2424/puyopuyo/releases

ビルドする場合は"Desktop Development with C++" がインストールされているVisualStudio上でお願いします

# Usage

「次へ」で１手進む

「１手戻る」で１手戻る

「リセット」でフィールドがリセット

「ツモ変更」で配ツモが変更  ※１手戻るの実装が雑のため併用不可

# Note

  仕組みが気になる方は以下の２つの素晴らしいスライドを参考にしてください
  
  [ぷよぷよAI 人類打倒に向けて](https://www.slideshare.net/mayahjp/puyoai-gpw2015)
  
  [ぷよぷよAIの新しい探索法](https://www.slideshare.net/takapt0226/ai-52214222)

  ２つとも素晴らしいスライドなので見て欲しいですが、面倒な方のための軽い説明
  
  ```
  連鎖のシュミレートはビット演算で行える
  
  次に置く手、見えているネクスト,ネクストネクストの後のツモをランダムに作成して仮想の連鎖を構築させて
  うまく連鎖した時の初手を反映させることで上手に組んでくれるようになる
  ```

# Author

* たまねぎ
* 所属 : 未定
* ss0302winning@gmail.com

# License

"勝手にぷよ" is under [MIT license](https://en.wikipedia.org/wiki/MIT_License).


# Readme template 

https://cpp-learning.com/readme/
