# USD Exporter for Shade3D : 開発情報

USD Exporter for Shade3DのShade3Dプラグインをビルドする際の情報を記載します。    
ここでは、Windows 10環境でビルドしました。    
なお、Shade3Dプラグインとしてのビルド時に、PixarのUSDのライブラリやヘッダファイルが必要となります。    

## USDのライブラリビルドで必要なモジュール

* Visual Studio 2017 (Community版でOK)
* Python27 (ビルド時に必要。Python35では失敗しました)
* CMake
* NASM

## USDのビルド (Pythonを内包しない)

Visual Studio 2017はあらかじめインストール済みとする。    

### Python27のインストール

https://www.python.org/downloads/windows/    
より「Python 2.7.16」の「Windows x86-64 MSI installer」を探してインストール。    
「C:\Python27」にインストールしたとする。    
Pythonは、USDのビルド時に必要になります。    

### CMakeをインストール

CMake ( https://cmake.org/ )をダウンロードしてインストールしておく。    
「C:\Program Files\CMake」にインストールしたとする。     

### NASMをインストール

NASM ( http://www.nasm.us/ ) のWindows 64bit版をダウンロードしてインストール。    
https://www.nasm.us/pub/nasm/releasebuilds/2.14.02/win64/     
でダウンロード。    

NASMはアセンブラのコンパイラ。    
「C:\Program Files\NASM」にインストールしたとする。    

### USDのビルド (C++ライブラリとして使用する場合)

https://github.com/PixarAnimationStudios/USD

より、v19.07のソースをダウンロード。    
「C:\WinApp\USD」に展開したとする。    

以下の手順でビルドする。    

スタートメニューより「Visual Studio 2017」-「VS2017用 x64 Native Tools コマンドプロンプト」を起動。    
これで、VS2017のビルドのパスが通ったコマンドプロンプトが起動する。   

以下のパスを通す。    
> set PATH=C:\Python27;C:\Python27\Scripts;%PATH%    
> set PATH=C:\Program Files\CMake\bin;%PATH%    
> set PATH=C:\Program Files\NASM;%PATH%    

Python/CMake/NASMのそれぞれのパスは、インストールしたときのものを指定すること。    

以下で、USDの関連ファイルをビルドする。    
これは時間がかかる。    

> python C:\WinApp\USD\build_scripts\build_usd.py --no-python "C:\WinApp\USD\builds_no_python"    


この場合は、「C:\WinApp\USD」にUSDのファイルが展開されているとする。    
「--no-python」を指定することで、ビルドするライブラリはPythonを参照しなくて済むようになる。    
Pythonと切り離したライブラリ利用ができる。これにより、純粋にC++を使ったUSDのインポータやエクスポータ、ツール類を作成できる。     
なお、このビルドでは動的ライブラリを参照する形になる。    
「C:\WinApp\USD\builds_no_python」はビルドしたライブラリや関連ヘッダが格納されるフォルダ。     

なお、「--no-python」を指定している場合は、USDのコマンドラインツール(usdcat/usdzip/usdviewなど)は作成されない点に注意。    

ビルド結果として、    
「C:\WinApp\USD\builds_no_python\bin」に関連のdll類、    
「C:\WinApp\USD\builds_no_python\lib」に関連のdllとビルドで必要なライブラリ(lib)、Shaderファイル類が格納されている。    
「C:\WinApp\USD\builds_no_python\include」にヘッダファイルが格納されている。    

## Shade3Dプラグインとしてのビルド

https://github.com/shadedev/pluginsdk/releases    
より、Shade 15.1.0 Plugin SDKをダウンロードする。    

USD Exporter for Shade3Dのソースをダウンロードし、    
プラグインSDKの「ShadePluginSDK1510\plugin_projects」の下に「USDExporter」ディレクトリごとコピーする。    
「USDExporter\win_vs2017\Template.sln」をVS2017で起動する。    


### 検索インクルードディレクトリの指定

ソリューションの「USDExporter」のプロパティを開き、    
「C/C++」-「全般」の「追加のインクルードディレクトリ」に    
> C:\WinApp\USD\builds_no_python\include\boost-1_65_1    
> C:\WinApp\USD\builds_no_python\include    

があるのを確認。このパスを環境に合わせて書き換える。    

### 検索ライブラリディレクトリの指定

「リンカー」-「全般」の「追加のライブラリディレクトリ」に    
> C:\WinApp\USD\builds_no_python\lib    

があるのを確認。このパスを環境に合わせて書き換える。    

### 参照ライブラリの確認

「リンカー」-「入力」で、必要なライブラリが指定されているのを確認（これはすでに指定済み）。    

> pcp.lib    
> sdf.lib    
> vt.lib    
> gf.lib    
> tf.lib    
> usd.lib    
> usdGeom.lib    
> usdShade.lib    
> usdSkel.lib    

### ヘッダを書き換え

pxr/usd/pcp/types.h の    

> constexpr size_t PCP_INVALID_INDEX = std::numeric_limits<size_t>::max();    

の前に以下を入れる。   

> #undef max    

以上で、プロジェクトのUSDのビルド指定が整ったのでプラグインとしてビルドする。    
なお、Debug時は正しく実行できない点に注意。    
x64のReleaseでビルドする。    

ビルドが正常終了すると「win_vs2017」フォルダに「USDExporter64.dll」が配置される。    


