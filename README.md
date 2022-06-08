# myftp : 疑似 ftp サーバ、クライアント
## 概要
ソケット通信を利用した C 言語による CLI 擬似 ftp プログラムです．クライアント、サーバ双方のプログラムからなります．
**実用目的でない擬似プログラムのため暗号化せず平文で通信しています。また動作の保証はしていません。実際の用途での利用はしないでください。**

## 利用方法
ディレクトリ内で make コマンドを実行するとコンパイルが実行され、myftps (サーバ側プログラム) と myftpc (クライアント側プログラム) が生成されます。それぞれをサーバ側、クライアント側ターミナルで実行 (./myftps、./myftpc) することで ftp システムの再現ができます。

## 機能
### サーバ
50021 番ポートを利用し、クライアントからの接続を待ちます。接続後、クライアントからの要求 (ファイル要求やディレクトリ内表示) に返答します。出力は表示しません。
### クライアント
コマンドライン引数で入力された IP アドレスの 50021 番ポートのサーバへ接続します。IP アドレスが入力されていない場合やサーバが起動していない場合、エラーを出してプログラムを終了します。
クライアント側のターミナル上で help コマンドを打つと詳細な機能について参照できます。
