int analogPin = A0;
int sensorValue = 0;
int th[10] = {550, 525, 489, 415, 350, 290, 225, 201, 175, 175}; //15cm,20cm,25cm,30cm,35cm,40cm,45cm,50cm,60cm,70cm
const int datamax = 100;
int data[datamax];

//以下，自作関数
//0から7の数字へ変換
int thchange(int sensorValue){
  if(th[1] < sensorValue){ //590以下,500未満
    return 0;
  } else if(th[1] >= sensorValue && th[2] < sensorValue){ //500以下,470以上
    return 1;
  } else if(th[2] >= sensorValue && th[3] < sensorValue){ //470以下,400以上
    return 2;
  } else if(th[3] >= sensorValue && th[4] < sensorValue){ //400以下,318以上
    return 3;
  } else if(th[4] >= sensorValue && th[5] < sensorValue){ //318以下,280以上
    return 4;
  } else if(th[5] >= sensorValue && th[6] < sensorValue){ //280以下,260以上
    return 5;
  } else if(th[6] >= sensorValue && th[7] < sensorValue){ //260以下,205以上
    return 6;
  } else if(th[7] >= sensorValue && th[8] < sensorValue){ //205以下,182以上
    return 7;
  }else if((th[0] >= sensorValue && th[1]< sensorValue) || th[9] >= sensorValue){
    return -1; //開始，終了のとき-1
  }else{
    return 999; //それ以外の表示は999,エラー
  }
};

void setup() {
  Serial.begin(9600);
}

void loop() {
  sensorValue = analogRead(analogPin);

  // 読み取り開始(570以上)
  if (th[0] <= sensorValue ) {
    Serial.print("読み取り開始");
    Serial.println(sensorValue);
    int re = 0;
    delay(1300);

    // 読み取り終了(180以下)になるまで while で回す
    while (sensorValue > th[9]) {
      sensorValue = analogRead(analogPin);
      int number = thchange(sensorValue);

      if (number >= 0 && number <= 7 && re < datamax) {
        data[re] = number;
        re++;
      }
      Serial.print(number);
      Serial.print(",");
      Serial.println(sensorValue);

      delay(1000);
    }

    Serial.print("読み取り終了");
    Serial.println(sensorValue);


    for (int i = 0; i < re; i++) {
      Serial.print(data[i]);
      Serial.print(",");
    }
    Serial.println();
  }else{
    Serial.print("まだ読み取っていません");
    Serial.print("現在の値:");
    Serial.println(sensorValue);
  }
  
  delay(200);
}