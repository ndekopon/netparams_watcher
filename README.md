# netparams_watcher

netparamsのバックアップとリストアをするアプリ。  
オマケとして作成・削除に合わせてF19/F20キーを押す機能つき。

## 詳細

リストアする理由については以下のURLを参照。
- [@RizqLtさんのDDoSから復帰する方法に関するツイート(動画つき)](https://twitter.com/RizqLt/status/1631531414230376454)

F19/F20キーを押す機能は、OBS等にホットキー登録すればマッチの開始終了に合わせて録画開始終了が可能。

## インストール方法

Releasesページからzipファイルをダウンロード。  
zipファイルの中身(exe)を適当なフォルダに配置する。

## 使い方

![image](https://user-images.githubusercontent.com/92087784/223420314-405be022-1f43-4f8c-a6ae-806c9c4e504d.png)


Apex Legendsのマッチに入る前に起動しておく。

以下の条件を満たすとRESTOREボタンが押せるようになる。

- バックアップ成功
- netparamsが存在しない

## バックアップ先について

`netparams`がある場所に`backup`フォルダを作ってその中に`netparams_20230101_000000`のようなファイル名で保存する。  
※世代管理して古いものから削除していく予定。

## OBSでの録画開始・停止を割り当てる

設定画面からF19/F20キーを割り当てるのは至難の業なので、設定ファイルを直接編集する。  
編集するファイルは`C:\Users\<ユーザー名>\AppData\Roaming\obs-studio\basic\profiles\<プロファイル名>\basic.ini`となる。  
※ユーザー名、プロファイル名は各自の環境に置き換える。

追記する設定は以下の通り。

```.ini
[Hotkeys]
OBSBasic.StartRecording={"bindings":[{"key":"OBS_KEY_F19"}]}
OBSBasic.StopRecording={"bindings":[{"key":"OBS_KEY_F20"}]}
```

既に`[Hotkeys]`セクションが存在する場合は、同じセクションに追加する。

```.ini
[Hotkeys]
...
(既存の設定)
...
OBSBasic.StartRecording={"bindings":[{"key":"OBS_KEY_F19"}]}
OBSBasic.StopRecording={"bindings":[{"key":"OBS_KEY_F20"}]}
```

## 注意点

このアプリは無保証です。  
復帰やできることや動作を保証するものではありません。


## ライセンス

[MIT license](https://en.wikipedia.org/wiki/MIT_License)
