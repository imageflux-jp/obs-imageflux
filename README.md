# OBS Plugin For ImageFlux Live Streaming

[![GitHub tag (latest SemVer)](https://img.shields.io/github/tag/imageflux-jp/obs-imageflux.svg)](https://github.com/imageflux-jp/obs-imageflux)
[![Actions Status](https://github.com/imageflux-jp/obs-imageflux/actions/workflows/main.yml/badge.svg)](https://github.com/imageflux-jp/obs-imageflux/actions)

[OBS Studio](https://obsproject.com/ja) (Open Broadcaster Software Studio) から、[ImageFlux Live Streaming](https://imageflux.sakura.ad.jp/livestreaming/) に配信する機能を追加するプラグインです。  
OBS Studioの持つ豊富なレイアウト編集機能、音声編集機能、ウインドウキャプチャ機能を使用した高度な配信をすることが可能です。  

## 対象動作環境

OBS Studio 29.1.2  
Windows 11 x64  

## インストール方法

* インストーラを使用してインストール

* または、zipファイルをダウンロードして解凍したものをOBS Studioのプラグインフォルダにコピー  
C:\Program Files\obs-studio\obs-plugins\64bit\obs-imageflux.dll

## アンインストール方法

C:\Users\(username)\AppData\Roaming\obs-studio\plugin_config\rtmp-services  
をフォルダごと削除

* インストーラを使用した場合はコントロールパネルからアンインストール

* OBS Studioのプラグインフォルダにコピーした場合は、obs-imageflux.dll, obs-imageflux.pdbをエクスプローラで削除  
C:\Program Files\obs-studio\obs-plugins\64bit\obs-imageflux.dll

## プラグインの旧バージョンからのアップデート方法

C:\Users\(username)\AppData\Roaming\obs-studio\plugin_config\rtmp-services  
をフォルダごと削除し、OBS Studioを終了し、再度実行してください。

## 使い方

1. プラグインをインストール

2. OBS Studioを起動し、設定ボタンを押す
<img width="145" alt="obs-imageflux-010" src="https://user-images.githubusercontent.com/59855953/213332536-6f0ed08c-3062-4e5a-b7cc-edfa375e2c8a.png">

3. 「配信」メニューを選択し、「サービス」の先頭の「ImageFlux Live Streaming」を選択する。  
もし「サービス」一覧の中に「ImageFlux Live Streaming」がない場合は、OBS Studioをいったん終了し、再度実行する  
「ストリームキー」は使用しないので、一文字以上何かを入力する 例: dummy  
<img width="738" alt="obs-imageflux-020" src="https://user-images.githubusercontent.com/59855953/213332558-2a5e0719-2fd8-4600-a6c2-b721faa35d5d.png">

4. 「出力」メニューを選択し、「出力モード」を「基本」から「詳細」に切り替える  
「配信」タブ内の「エンコーダ」を「ImageFlux Live Streaming (WebRTC)」に切り替える  
画面下部の「エンコーダ設定」にImageFlux Live Streaming専用の設定が表示される  
「Singaling URL」「Channel ID」にImageFlux Live Streaming APIで取得した所定の値をペーストする  
「Video Codec」は「VP8」を推奨  
<img width="738" alt="obs-imageflux-030" src="https://user-images.githubusercontent.com/59855953/213332590-279c77d8-6f88-4fc1-ba27-db2627c9935f.png">

5. 「音声」メニューを選択し、「サンプルレート」が「48kHz」、「チャンネル」が「ステレオ」になっていることを確認する  
他の値でも動くかもしれないが、未確認  
<img width="738" alt="obs-imageflux-040" src="https://user-images.githubusercontent.com/59855953/213332762-d329b1d3-30b3-4f77-b61c-be2282f53ebd.png">

6. 「映像」メニューを選択し、「基本 (キャンバス) 解像度」が「1280x720」、「出力 (スケーリング) 解像度」が「1280x720」、「FPS共通値」が「30」を推奨  
他の値でも動くが、未確認  
<img width="738" alt="obs-imageflux-050" src="https://user-images.githubusercontent.com/59855953/213332822-fd0d0604-27e5-435f-b3ca-19241ef5f97b.png">

7. 「設定」ダイアログ右下の「OK」ボタンを押して設定を保存し、ダイアログを閉じる

8. 「配信開始」ボタンを押し、配信を開始する。10-20秒程度待ち、目的のWebサイトで動画が見れることを確認する
<img width="145" alt="obs-imageflux-060" src="https://user-images.githubusercontent.com/59855953/213332877-ff58110a-dbc4-4b29-a1d3-e17f0e31faa6.png">

9. 「配信終了」ボタンを押し、配信を終了する
<img width="145" alt="obs-imageflux-070" src="https://user-images.githubusercontent.com/59855953/213332886-101cdcc4-cb51-4f92-b628-50211db007fb.png">

## ビルド方法

1. Windows 11 development environment 等を利用し、Visual Studio 2022を準備する  

2. PowerShell v7をインストールし、その他のコマンドラインツールもインストールする
```
winget install Microsoft.Powershell
winget install Git.Git
winget install Kitware.CMake
winget install 7zip.7zip
```

3. 7-zipを環境変数PATHに含める  
```
C:\Program Files\7-Zip
```

4. PowerShellで署名なしのスクリプトの実行を許可する  
Settings > Privacy & security > For developers > PowerShell  
Change execution polity to allow local PowerShell scripts to run without signing...  
"Apply" ボタンを押し、PowerShellを再起動  

5. 本ソフトウェアのコードをクローンし、ビルドコマンドを実行すると、依存ファイル一式をダウンロードし、ビルドを行う  
```
mkdir obs && cd obs
git clone https://github.com/imageflux-jp/obs-imageflux
cd obs-imageflux
.github/scripts/Build-Windows.ps1
```
利用しているライブラリは [buildspec.json](buildspec.json) を書き換えることで更新することができる  

6. 2回目以降は --SkipDeps をつけるとビルド時間を短縮できる  
```
.github/scripts/Build-Windows.ps1 --SkipDeps
```

7. できあがったファイルをOBSのプラグインフォルダにコピーし、動作確認を行う  
```
copy release/obs-plugins/64bit/obs-imageflux.dll C:\Program Files\obs-studio\obs-plugins\64bit\obs-imageflux.dll
```

8. デバッガで処理をステップ実行したい場合は、OBS Studio本体の開発環境を整えてから、pluginsフォルダ下に本プロジェクトを配置し、同時にビルドを行う  


## ライセンス

本ソフトウェアのライセンスはGPL v2です。  

## コントリビューションについて

PR・issueで送られた全てのコードはGPL v2で提供されたとみなします。  

## 内部で明示的に使用しているソフトウェア

* OBS Studio: [GPL v2](https://github.com/obsproject/obs-studio/blob/master/COPYING) / [web](https://github.com/obsproject/obs-studio) / [github](https://github.com/obsproject/obs-studio)
* Sora C++ SDK: [Apache LICENSE 2.0](https://github.com/shiguredo/sora-cpp-sdk/blob/develop/LICENSE) / [github](https://github.com/shiguredo/sora-cpp-sdk)
* libwebrtc: [WebRTC License](https://webrtc.github.io/webrtc-org/license/software/) / [web](https://webrtc.org/) / [github](https://github.com/shiguredo-webrtc-build/webrtc-build)
* Boost: [Boost Software License 1.0](https://www.boost.org/LICENSE_1_0.txt) / [web](https://www.boost.org/)

本プロジェクトは [obs-plugintemplate](https://github.com/obsproject/obs-plugintemplate) の派生物です。  
