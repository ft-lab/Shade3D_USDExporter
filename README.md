# USD Exporter for Shade3D (ベータ)

PixarのUSD(Universal Scene Description)形式 ( https://graphics.pixar.com/usd/docs/index.html )のファイルをエクスポートするプラグインです。    
USDをzip圧縮して1つにしたusdz形式を使用することで、AppleのiOS/iPadOSでARとして3Dモデルを使うことができるようになります。    


## USDについて

PixarのUSDについては、以下のページをご参照ください。    
https://graphics.pixar.com/usd/docs/index.html    

USDはオープンソースのフォーマットになります。    
https://github.com/PixarAnimationStudios/USD    

## USD Exporterの機能

以下の機能があります。    

* Shade3Dから、PixarのUSD形式/圧縮されたUSDZ形式でエクスポート。

## 動作環境

* Windows 7/8/10以降のOS    
* Shade3D ver.15以降で、Standard/Professional版（Basic版では動作しません）  
  Shade3Dの64bit版のみで使用できます。32bit版のShade3Dには対応していません。   

※ 現状は、Windows版のみ対応しています。Mac版は後々ビルドします。    

## プラグインの配置、Shade3Dの起動方法

### プラグインダウンロード

USDを作成するために参照するDLLと関連ファイルと、Shade3Dプラグインの両方が必要になります。    

usd_dlls_1907.zipを解凍し、任意の場所に配置します。    
解凍すると、「usd_dlls」フォルダに「bin」と「lib」のフォルダが入っています。     
また、環境変数のPATHを通してShade3Dを起動するためのバッチファイル「run_shade3d_v17.bat」を同梱しています。    
これについては後述します。    

次に、Shade3Dプラグインをダウンロードします。    

### プラグインを配置し、Shade3Dを起動

Windowsの場合は、USDExporter64.dll をShade3Dのpluginsディレクトリに格納します。     

Shade3D実行時に「usd_dlls/bin」「usd_dlls/lib」フォルダへのPATH指定が必要です。    

以下のように、バッチファイル（*.bat）を作ってShade3Dを起動します。    
> set USD_DLLS_PATH=E:\Data\Shade3D\USDExporter\usd_dlls    
> set PATH=%USD_DLLS_PATH%\bin;%USD_DLLS_PATH%\lib;%PATH%    
> cd C:\Program Files\Shade3D\Shade3D ver.17\bin    
> c:    
> shade.exe    

USD_DLLS_PATHは「usd_dlls」フォルダを展開したパスを指定します。    
set PATHで環境設定のパスを指定しています。    
この場合は、Shade3D ver.17を起動しています。    

「usd_dlls_1907.zip」を解凍すると、「run_shade3d_v17.bat」のファイルが同梱されています。    
上記と同じバッチファイルになります。    
「USD_DLLS_PATH」の指定を環境に合わせて書き換え、「C:\Program Files\Shade3D\Shade3D ver.17\bin」をShade3Dのバージョンに合わせて書き換えるようにしてください。    

メインメニューの「ファイル」-「エクスポート」-「USD」が表示されるのを確認します。     

## 使い方

## サンプルファイル

## 対応している機能

* USDのバイナリ(拡張子 usdc)またはテキスト(拡張子 usda)とテクスチャの出力
* zip圧縮したusdz形式の出力
* iOS12/iOS13のAR表示への対応 (iOS13を推奨)
* ポリゴンメッシュとして形状出力
* マテリアルはPBR (diffuse/roughness/metallic/normal/emissive/occlusion/opacity) として出力
* ボーン＋スキンを使用したアニメーション出力対応
* ボールジョイント内に形状を入れた場合、位置と回転をアニメーションのキーフレームとして出力

## doubleSided対応について

doubleSidedとは、リアルタイムでMeshを表示する場合に裏面を表示するかしないか、の指定です。    
USDでは、形状(Mesh)に対してdoubleSidedの指定が存在します。    
USD Exporter for Shade3Dでは、    
マスターサーフェス名として「xxxx_doublesided」のように、「doublesided」が含まれる場合（大文字の場合でも小文字として判断）にdoubleSidedが指定されたと判断します。    
なお、iOS12/iOS13ではdoubleSidedの指定は無効化され、常に「裏面は表示しない (doubleSided=false)」となるようです。    

## 制限事項

USDエクスポート時の制限事項です。    
USDの仕様によるものもあるため、それらは後述します。     

* 形状名やマテリアル名は、英数字と「_」に変換されます。    
全角文字を使用している場合は、名前がmeshなどに置き換わります。    
* 複数の形状名やマテリアル名が同一の場合、「_1」のような連番が割り振られます。
* 自由曲面や回転体/掃引体などは、すべてポリゴンメッシュに変換されます。    
* ポリゴンメッシュの多角形面は、三角形または四角形に分解されて出力されます。    
* ポリゴンメッシュのサブディビジョン出力には対応していません。    
アニメーション情報を持たない場合は、サブディビジョンを使用したポリゴンメッシュは再分割されます。    
アニメーション情報を持つ場合は、サブディビジョンを使用したポリゴンメッシュは正しく動作しません。    
* ポリゴンメッシュのUV層は最大2層まで出力されます。    
ただし、iOS12/iOS13ではUV層は1層しか認識されません。    
* 出力USD名が全角の場合、「output_日付_時間」の名前に変換されます。

## 制限事項 (USDの仕様)

以下は、USDの仕様になります。    
ここでの「Mesh」はShade3Dでの「ポリゴンメッシュ」と同等のものを指します。    

* 形状名やマテリアル名は、英数字とアンダーバーのみ使用できます。    
全角含むUTF-8には対応していません。    
また、名前の先頭は数字を指定できません。'a'-'z'、'A'-'Z'、アンダーバーを使用します。    
* USDの構造上、形状名やマテリアル名はパス形式で保持されます。    
区切り文字は「/」を使います。    
このとき、同一パス名を持つことはできません。    
* 出力するUSDファイル名は、全角文字を使用できません。
* MeshのUV層は最大2層まで持つことができます。    
* Meshの面は多角形を保持できますが、USDを表示するビュワー環境によっては凹面は正しく表現できない場合があるようです。    
複雑な面の場合は、三角形または四角形に分割したほうがいいかもしれません。    
* ボーン＋スキンを使用した場合、SkelRootというノードに情報を押し込みます。    
このときシーンのツリー構造からボーン構造を配列に変換して格納する必要があり、オリジナルのツリー構造は保持されなくなります。    
* テクスチャイメージは、pngまたはjpgを指定できます。アニメーションするイメージには対応していません。    

### 未調査

まだUSDの仕様として未調査の箇所です。

* Meshの頂点ごとに頂点カラーを持てるか。
* MorphTarget(BlendShapes)を表現できるか。

## ライセンス  

This software is released under the MIT License, see [LICENSE](./LICENSE).  

## 更新履歴

[2019/10/12] ver.0.0.1.0   

* 初回版 (ベータ)

