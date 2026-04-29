# ALiS 各段の詳細図

`architecture.md` の補足。各段のロジックを掘り下げた図を集めています。

---

## 1. 段1: 文字 → 距離 への変換フロー

```mermaid
flowchart TB
    IN[Serial で文字列受信]
    LOOP{"buf に蓄積<br/>'\n' or '\r' を待つ"}
    EACH[各文字を取り出し]
    FIND[findPos with c, row, col<br/>8×8 glyph を線形探索]
    OK{見つかった?}
    ERR[エラー出力<br/>「対応していない文字」]
    PRINT[Serial に row,col を出力]
    MOVE_R[moveTo with row<br/>L6470_goto]
    MOVE_C[moveTo with col<br/>L6470_goto]
    NEXT{次の文字?}
    HOME[原点付近に戻す<br/>2500*8 step]

    IN --> LOOP
    LOOP -->|改行検出| EACH
    EACH --> FIND --> OK
    OK -- No  --> ERR --> LOOP
    OK -- Yes --> PRINT --> MOVE_R --> MOVE_C --> NEXT
    NEXT -- Yes --> EACH
    NEXT -- No  --> HOME --> LOOP
```

**ステップ換算式 (`indexToStep`)**

```
mm[]      = {200, 250, 300, 350, 400, 450, 500, 600, 700}
steps     = mm[label] * 200 * 8 / 60
              = mm * 26.667 (step)
```

ラベル 0-8 が 8×8 テーブルの「行 or 列」に対応し、9 段階の物理距離に変換される。

---

## 2. 段2: 距離 → モールス への変換フロー

```mermaid
flowchart TB
    READ[A0 ADC読み取り]
    INTV{経過時間 ≥ 1000ms?}
    TH[thchange ADCを 0..7 / -1 / -2 / 999 に分類]
    START{detected = -1<br/>かつ未読込?}
    SETF[isReadingSensor = true<br/>dataCount = 0]
    DATA{detected = 0..7?}
    BUF["dataBuffer[dataCount++] = detected"]
    END{detected = -2?}
    SEND[processAndSendMorseCode]
    HEADER[Header送信<br/> ..---..]
    LOOPN[各 dataBuffer 要素を<br/>3要素モールスで送信]
    FOOTER[Footer送信<br/> .-.-.]
    DONE[isReadingSensor=false]

    READ --> INTV
    INTV -- No  --> READ
    INTV -- Yes --> TH --> START
    START -- Yes --> SETF --> READ
    START -- No  --> DATA
    DATA -- Yes --> BUF --> READ
    DATA -- No  --> END
    END  -- Yes --> SEND
    END  -- No  --> READ
    SEND --> HEADER --> LOOPN --> FOOTER --> DONE --> READ
```

**閾値テーブル (ADC 値、上→下に距離が遠くなる前提)**

| 用途      | 値    |
|-----------|-------|
| 開始      | ≥ 550 |
| 数字 0    | > 525 |
| 数字 1    | 480〜525 |
| 数字 2    | 415〜480 |
| 数字 3    | 350〜415 |
| 数字 4    | 290〜350 |
| 数字 5    | 225〜290 |
| 数字 6    | 201〜225 |
| 数字 7    | 175〜201 |
| 終了      | ≤ 170 |

---

## 3. 段3: 受信側エッジ検出（モールス → 数字）

```mermaid
flowchart TB
    POLL[A0 = analogRead]
    NEW{lightValue > 500?}
    SAME{状態が変化?}
    DBNC{経過 > DEBOUNCE 10ms?}
    UPD[currentLightState 更新]
    EDGE{立ち上がり?}
    GAP[前回 OFF 期間を計測]
    GAP3{≥ 2.5×UT?}
    PROCESS["バッファを照合<br/>(数字 / Header / Footer)"]
    GAP1{≥ 0.5×UT?}
    OFF[lightOnTime 更新]
    DUR[ON 期間を計測]
    DOT{0.7×UT 〜 2×UT?}
    DASH{2.5×UT 〜 4×UT?}
    ADDD[buffer に '.' 追加]
    ADDDH[buffer に '-' 追加]
    SAVE[lightOffTime 更新]
    TO{TIMEOUT 7×UT 経過?}
    FLUSH[残りバッファ処理 + リセット]

    POLL --> NEW --> SAME
    SAME -- No  --> TO
    SAME -- Yes --> DBNC
    DBNC -- No  --> POLL
    DBNC -- Yes --> UPD --> EDGE
    EDGE -- 立ち上がり --> GAP --> GAP3
    GAP3 -- Yes --> PROCESS --> OFF
    GAP3 -- No  --> GAP1
    GAP1 -- Yes --> OFF
    GAP1 -- No  --> OFF
    EDGE -- 立ち下がり --> DUR --> DOT
    DOT  -- Yes --> ADDD --> SAVE
    DOT  -- No  --> DASH
    DASH -- Yes --> ADDDH --> SAVE
    DASH -- No  --> SAVE
    SAVE --> TO
    TO   -- Yes --> FLUSH --> POLL
    TO   -- No  --> POLL
```

---

## 4. 段3: 数字 → 2色ペア送信フロー

```mermaid
flowchart LR
    DATA[getNum 配列] --> EACH[各要素 v]
    EACH --> PAIR["colorPairs v 0 / v 1"]
    PAIR --> C1[1色目 sendColor a]
    C1   --> C2[2色目 sendColor b]
    C2   --> LAST{最後?}
    LAST -- No  --> SEP[LOWCYAN 区切り] --> EACH
    LAST -- Yes --> FT[GREEN フッタ]
```

注: `outputColorsFromGetNum()` の前にヘッダー色 (BLUE) が送信される。

---

## 5. 段4: 色 → 文字 への復号フロー

```mermaid
flowchart TB
    SCAN[TCS34725 RGB+C取得]
    DET[detectColor with R, G, B, C]
    PREV{前回と同じ?<br/>OFF/-1 は除外}
    OFF{COLOR_OFF?}
    INV{値 = -1?}
    UPD[previousDetectedCodeInLoop 更新]
    SM[現在の状態に応じた処理]
    COMP{COMPLETE?}
    RECV[receivedData を 2個ずつ取り<br/>row, col → glyph で文字復元]
    HALT[Serial 出力 → while true]

    SCAN --> DET --> PREV
    PREV -- Yes --> SCAN
    PREV -- No  --> OFF
    OFF  -- Yes --> SCAN
    OFF  -- No  --> INV
    INV  -- Yes --> SCAN
    INV  -- No  --> UPD --> SM --> COMP
    COMP -- No  --> SCAN
    COMP -- Yes --> RECV --> HALT
```

**状態ごとの遷移条件（Proc04 の switch ケースより）**

| 現状態 | 入力色 | 次状態 / 動作 |
|---|---|---|
| WAIT_FOR_HEADER | BLUE | RECEIVING_DATA_COLOR1 |
| WAIT_FOR_HEADER | その他 | 警告（同状態） |
| RECEIVING_DATA_COLOR1 | RED/LOWBLUE/LOWGREEN/CYAN | A=色, → COLOR2 |
| RECEIVING_DATA_COLOR1 | GREEN | COMPLETE |
| RECEIVING_DATA_COLOR1 | LOWCYAN/異常 | リセット |
| RECEIVING_DATA_COLOR2 | RED/LOWBLUE/LOWGREEN/CYAN | B=色, ペア確定 → SEPARATOR待ち |
| RECEIVING_DATA_COLOR2 | GREEN | COMPLETE |
| WAIT_FOR_DATA_SEPARATOR | LOWCYAN | COLOR1 |
| WAIT_FOR_DATA_SEPARATOR | GREEN | COMPLETE |

---

## 6. タイミング・チャート（モールス例「2」=「-.-」）

```
時刻(ms): 0   100  200  300  400  500  600  700  800  900  1000
LASER  : ███████████░░░░███░░░░███████████░░░░░░░░░░░░░░░░ (idle)
状態   : <───── - ─────><.><.. ↔ -><───── - ─────><── 文字間 ──>
                  300ms 100ms 100ms 100ms 300ms     300ms
```

UNIT_TIME を変える場合は送信側 `Send_Morse_EX.ino` の `UNIT_TIME` と受信側 `Receive_Morse_EX.ino` の `UNIT_TIME` を同値にする必要がある（受信ウィンドウは UNIT_TIME を基準に算出されるため）。

---

## 7. ファイル ↔ 段 対応早見表

| ファイル | 段 | 入力 | 出力 |
|---|---|---|---|
| `JoinProgram/Proc01_Encode_IS1/Proc01_Encode_IS1.ino` | 1 | Serial 文字列 | モータ位置 (距離 mm) |
| `JoinProgram/Proc02_IC1_LM2/Proc02_IC1_LM2.ino` | 2 | A0 ADC (赤外線) | D9 レーザー (モールス) |
| `JoinProgram/Proc03_LM2_FS3/Proc03_LM2_FS3.ino` | 3 | A0 (フォトTr) | D9/D3/D11 (RGB LED) |
| `JoinProgram/Proc04_FS3_Decode/Proc04_FS03_Decode.ino` | 4 | I2C (TCS34725) | Serial 出力 (文字) |
| `LightMorse/Send_Morse_EX/Send_Morse_EX.ino` | 単体 | Serial 数字列 | レーザー |
| `LightMorse/Receive_Morse_EX/Receive_Morse_EX.ino` | 単体 | フォトTr | Serial |
| `FullColorSensor/FC_EX/FC_SendColor_EX.ino` | 単体 | 固定 data[] | RGB LED |
| `FullColorSensor/FC_EX/FC_ReceiveColor_EX.ino` | 単体 | TCS34725 | Serial |
| `DecryptionTable/decryptionTable_encord/...` | 単体 | Serial 文字列 | row,col CSV |
| `DecryptionTable/decryptionTable_decode/...` | 単体 | row,col CSV | 文字 |
