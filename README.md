# netparams_watcher

netparamsのバックアップとリストアをするアプリ。

## 詳細

リストアする理由については以下のURLを参照。
- [@RizqLtさんのDDoSから復帰する方法に関するツイート(動画つき)](https://twitter.com/RizqLt/status/1631531414230376454)

## インストール方法

Releasesページからzipファイルをダウンロード。
zipファイルの中身(exe)を適当なフォルダに配置する。

## 使い方

Apex Legendsのマッチに入る前に起動しておく。以下の条件を満たすとRESTOREボタンが押せるようになる。

- バックアップ成功
- netparamsが存在しない

## バックアップ先について

`netparams`がある場所に`backup`フォルダを作ってその中に`netparams_20230101_000000`のようなファイル名で保存する。
※世代管理して古いものから削除していく予定。


## 注意点

このアプリは無保証です。
復帰できることを保証するものではありません。


## ライセンス

[MIT license](https://en.wikipedia.org/wiki/MIT_License)
