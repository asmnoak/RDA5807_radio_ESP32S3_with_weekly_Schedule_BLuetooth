<p><H3>週間スケジュールが可能なRDA5807 FM DSPラジオ(Bluetooth LE版)</H3></p>
<p>
予めBluetooth通信により設定した週間スケジュールに基づいて番組を聴くことができるFMラジオを製作したので紹介する。<br>
利用したRDA5807FPは、RDAマイクロ製のFM対応のDSPラジオICである。<a href="https://www.aitendo.com/product/4797">安価（データシートの参照リンクあり）</a>に購入でき<br>
SOP16ピンのパッケージで、かつ使用部品が少なくて済むので使い易い。<br>
I2Cインターフェースでコントロールするが、ここでは、目標機能を実現するために、Bluetooth LE（BLE）機能（およびWiFi機能）を搭載した<br>
<a href="https://www.switch-science.com/products/8968">Seeed Studio XIAO ESP32S3</a>と組み合わせた。<br>
RDA5807FPは電圧3.3Vで動作し、必要な電流は20mA程度なので、XIAO ESP32S3（3V3端子）から供給できる（5Vは不可）。<br>
XIAO ESP32S3の制御にはBLE機能のプロファイルである「GATT」を利用している。その際、通信の手段として、ChromeなどのWebブラウザに実装された「Web bluetooth API」を使用する。<br>
開発はArduino IDE 2.1で行った。<br>

使用したRDA5807用のライブラリは、<a href="https://github.com/pu2clr/RDA5807">こちら（pu2clr at GitHub）</a>にある。<a href="https://pu2clr.github.io/RDA5807/#schematic">回路図等</a>も掲載されているので参考にすると良い。<br>
なお、Arduino IDEのライブラリ管理からもインストール可能である。<br>
</p>
<p>
BLEでは、セントラル（ここではPCのブラウザ）とペリフェラル（ESP32S3）間で通信を行う。ペリフェラル側では、GATTで定義された「データ構造」のサービスを構築し<br>アドバタイズ（Advertise）状態で受信待ちする。
セントラル側からは、アドバタイズ状態のペリフェラルをスキャンして接続要求を行う。<br>
ここでは、便宜上、「データ構造」として、「GATT」の"Current time service"（UUIDで識別する）の定義を流用した。
サービスのキャラクタリスティックとディスクリプタはセントラル（指示）側からREAD/WRITEできるので、各々下記に示した目的で利用している。
</p>
<p>
&nbsp;&nbsp;&nbsp;&nbsp;サービス:&nbsp;Current time service&nbsp;―<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;キャラクタリスティック1&nbsp;―&nbsp;XIAO ESP32S3の操作に利用<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;ディスクリプタ1&nbsp;―&nbsp;書き込み時：時刻設定およびDSPラジオの操作、ブラウザに表示されているラジオボタンの曜日選択とラジオ局選択に利用<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;キャラクタリスティック2&nbsp;―&nbsp;読み込み時：選択された曜日のスケジュールの情報を提供、書き込み時：選択されたラジオ局の情報および曜日のスケジュールの設定<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;キャラクタリスティック3&nbsp;―&nbsp;読み込み時：ラジオ局の情報を提供<br>
</p>
<p><strong>機能</strong><br>
 ・週間スケジュールを設定できる。曜日ごとに、番組の開始時間、番組の長さ、ラジオ局、音量、番組終了後ON/OFFを設定する。<br>
 ・時刻はWebブラウザ側からの操作で、XIAO ESP32S3に伝え、内部クロックに設定する。<br>
 ・週間スケジュールの設定は、PC、スマホ等のブラウザからXIAO ESP32S3にアクセスして行う。<br>
 ・同様に、ラジオ局の選局、音量の変更、ラジオのON/OFFは、ブラウザから行うことができる。<br>
 ・週間スケジュールの設定により、目覚まし機能、スリープ機能が可能である。<br>
 ・OLED表示装置に、日付、曜日、時刻、音量、ラジオのON/OFF、受信周波数を表示する。<br>
 ・XIAO ESP32S3の特定のピンにタクトスイッチを接続すれば、選局、音量調節、ラジオのON/OFFが可能である。<br>
 ・受信周波数の範囲は、76－108MHzで、ワイドFM対応である。<br>
 ・出力はオーディオジャック経由で小口径のスピーカー（ステレオ）を接続する。<br>
</p>
<p><strong>H/W構成</strong><br>
 ・Seeed Studio XIAO ESP32S3 - コントローラ<br>
 ・I2C接続&nbsp; RDA5807FP<br>
 ・I2C接続&nbsp; SSD1306 64x32 OLED表示装置<br>
 ・Xtal発振器（32768Hz）、コンデンサ、抵抗類、オーディオジャック、配線類<br>
</p>
<p>
<img src="./xiao_esp32s3_rda5807_ble.jpg" width="480" height="480"><br>
専用の基板（XIAO_ESP32C3の物を流用。<a href="https://www.pcbway.com/project/shareproject/RDA5807_FM_DSP_radio_with_weekly_schedule_which_is_controlled_by_XIAO_ESP32C3_dbd09236.html">基板のデータ</a>）に実装。右側がXIAO ESP32S3、左側がRDA5807FP。
</p>
<p><strong>接続</strong><br>
各コンポーネントの接続は以下の通り。<br>
<p>
<table> 
<tr>
<td>I2C&nbsp;</td><td>XIAO(既定)</td>
</tr>
<tr>
<td>SCK</td><td>GPIO6</td>
</tr>
<tr>
<td>SDA</td><td>GPIO5</td>
</tr>
</table>
</p>
<p>
<table> 
<tr>
<td>タクトスイッチ</td><td>XIAO</td>
</tr>
<tr>
<td>音量</td><td>GPIO2</td>
</tr>
<tr>
<td>選局</td><td>GPIO3</td>
</tr>
<tr>
<td>PON/POFF</td><td>GPIO4</td>
</tr>
</table>
</p>
</p>
<p>
I2Cのアドレス
<table> 
<tr>
<td>RDA5807FP</td><td>0x10&nbsp;or&nbsp;0x11&nbsp;ライブラリで既定</td>
</tr>
<tr>
<td>OLED</td><td>0x3C&nbsp;既定</td>
</tr>
</table>
</p>
<p><strong>操作方法</strong><br>
ブラウザから、"Web_Bluetooth_Radio_Sched.html" ファイルにアクセス（ドラッグ＆ドロップ）すると以下の画面が表示される。<br>
操作時には毎回、"Connect to ESP32"ボタンを押して、ESP32に接続する必要がある。ESP32との接続は約30秒維持されるので、この間に操作を行う。<br>
"Connect to ESP32"ボタンを押すと、Bluetoothデバイスのスキャンウィンドウが表示されるので"ESP32S3_X"を選択する。<br>
接続されると"Set current time to ESP32 RTC"、音量、選局、電源ON/OFFなどの操作ボタンが押せるようになる。<br>
"Set current time to ESP32 RTC"ボタンは、ESP32に現在時刻を設定するため、電源ON時に一回だけ押す必要がある。<br>
［注意］PC側のデバイスリストに"ESP32S3_X"が登録されるアクセスできなくなるので、その場合はデバイスリストから"ESP32S3_X"を削除する。

<p>
<img src="./weekly_schedule_ble.png" width="900" height="560"><br>
</p>
</p>
<p><strong>FM局の初期設定</strong><br>
最初に、地域の受信可能なFM局の情報（周波数と局名）を設定する必要がある。<br>
まず、"Radio Stations"のラジオボタンの"0"をクリックすると入力域に初期値（ダミー値）が表示されるので<br>
"ST0"に続く、周波数(MHz)と局名(半角5文字まで)を修正し、"Save Staion Info"ボタンを押すと保存される。<br>
必要に応じて、ラジオボタンの"1"から"6"まで繰り返す(ラジオ局の番号になる。最大7局設定可能)。
</p>
<p><strong>週間スケジュールの設定方法</strong><br>
スケジュールは曜日ごとに設定する。各曜日ごとの聴取予定を時刻順に指定する。<br>
1エントリの項目は、番組開始時刻、ラジオ局（番号）、放送時間（分）、音量（0-8）、番組終了後に電源OFF（1の時）である。<br>
例えば、"22:00,1,119,2,1;"は、「番組開始時刻は22:00、ラジオ局の番号は1、放送時間は119分、音量は2、番組終了後に電源OFF」を意味する。最後の";"は区切り文字である。<br>
"Schedule of Week"の下にある各曜日のラジオボタンをクリックすると、現在の内容（最初はダミー値）が、入力領域に表示されるので、それを編集する。<br>
編集後、"Save schedule day of  Week"ボタンをクリックすると、設定内容が保存される。<br>
設定内容に形式上の間違いがある場合は、エラーが表示され、保存されない。なお、各番組の開始時刻と終了時刻は分単位で重ならないように指定すること。<br>
</p>
<p><strong>実行時のログメッセージについて</strong><br>
"Live Output"に実行時のログメッセージが表示される。例えば、PC側にBLEデバイスがない場合は以下のメッセージが表示される。
<p>
"Requesting any Bluetooth Device...<br>
Argh! NotFoundError: Bluetooth adapter not available."
</p>
</p>
<p><strong>インストール</strong><br>
<ol>
<li>コードを、ZIP形式でダウンロード、適当なフォルダに展開する。</li>
<li>ArduinoIDEにおいて、ライブラリマネージャから以下を検索してインストールする</li>
 <ul>
  <li>Adafruit_BusIO</li>
  <li>Adafruit_GFX</li>
  <li>Adafruit_SSD1306</li>
  <li>RDA5807</li>
 </ul>
<li>追加のライブラリを、ZIP形式でダウンロード、ライブラリマネージャからインストールする</li>
 <ul>
  <li>TimeLib&nbsp;:&nbsp; https://github.com/PaulStoffregen/Time</li>
 </ul>
<li>ArduinoIDEからxiao_esp32s3_BLE_clock_radio_RDA5807_master.inoを開く</li>
<li>「検証・コンパイル」に成功したら、一旦、「名前を付けて保存」を行う</li>
<li>上記のH/W構成、接続であればスケッチ修正の必要はない。</li>
</ol>
</p>
<p><strong>若干の解説</strong><br>
・回路図はRDA5807FPのデータシートを参照のこと。LOUT、ROUTのコンデンサは100－200μF程度を接続すると音質が良くなる。<br>
&nbsp;&nbsp;なお、回路図にあるインダクタンス系の部品は無くても動作する。<br>
&nbsp;&nbsp;参考までに実際に動作させたRDA5807FP回りの回路図を下に示した。<br>
・1日に指定できる番組の数は9までである（スケッチを修正して増やす場合は自己責任でお願いします）。<br>
&nbsp;&nbsp;同じラジオ局の番組を続けて聴く場合は、放送時間を合算して、1エントリで指定すればよい。<br>
&nbsp;&nbsp;なお、深夜0時を跨いだ部分は、次の曜日に指定する。<br>
<p>
<img src="./rda5807_connection.png" width="460" height="360"><br>
</p>
</p>
<p><strong>注意事項</strong><br>
・動作を保証するものではありませんので、利用の際は、自己責任でお楽しみください。<br>
</p>
