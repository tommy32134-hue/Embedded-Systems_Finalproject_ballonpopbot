#include <Servo.h>  // 載入 Arduino 內建的伺服馬達函式庫

Servo myServo1;      // 建立一個伺服馬達物件
int servoPin1 = 10;   // 設定伺服馬達的訊號線接在腳位 10

// --- 馬達與感測器腳位設定 ---
const byte LEFT1 = 7; 
const byte LEFT2 = 6;
const byte LEFT_PWM = 5;

const byte RIGHT1 = 8;
const byte RIGHT2 = 9; 
const byte RIGHT_PWM = 11;

// --- 速度設定 ---
byte leftSpeed = 145;
byte rightSpeed = 110;
byte turnSpeed = 110;

// --- 系統狀態與計時變數 ---
char currentState = 'S'; 
unsigned long lastCmdTime = 0; 

// 🌟 新增：連續攻擊次數計數器
byte attackCount = 0; 

// --- 提前宣告馬達控制函式 (確保 setup 中可以呼叫) ---
void forward();
void stopMotor();

void setup() {
  Serial.begin(9600);
  
  // 馬達腳位設定
  pinMode(LEFT1, OUTPUT);
  pinMode(LEFT2, OUTPUT);
  pinMode(LEFT_PWM, OUTPUT);
  pinMode(RIGHT1, OUTPUT);
  pinMode(RIGHT2, OUTPUT);
  pinMode(RIGHT_PWM, OUTPUT);

  myServo1.attach(servoPin1);
  myServo1.write(10); 

  // 🌟 新增：開場起跑與盲衝邏輯
  delay(10000);   // 1. 燒錄/通電後，強制發呆 10 秒 (給你時間拔線、放車子)
  forward();      // 2. 啟動馬達直行
  delay(2800);    // 3. 盲衝 5 秒鐘 (若覺得 5 秒衝太遠，可把這數字改小)
  stopMotor();    // 4. 煞車停止，確保交接給 YOLO 時車子是穩定的
  
  // 記錄交接當下的時間，避免一進入 loop 就觸發超時斷線機制
  lastCmdTime = millis(); 
}

// --- 馬達控制基礎函式 ---
void forward() {
  digitalWrite(LEFT1, HIGH);
  digitalWrite(LEFT2, LOW);
  analogWrite(LEFT_PWM, leftSpeed);
  digitalWrite(RIGHT1, HIGH);
  digitalWrite(RIGHT2, LOW);
  analogWrite(RIGHT_PWM, rightSpeed);
}

void backward() {
  digitalWrite(LEFT1, LOW);
  digitalWrite(LEFT2, HIGH);
  analogWrite(LEFT_PWM, leftSpeed);
  digitalWrite(RIGHT1, LOW);
  digitalWrite(RIGHT2, HIGH);
  analogWrite(RIGHT_PWM, rightSpeed);
}

void turnLeft() {
  digitalWrite(LEFT1, LOW);
  digitalWrite(LEFT2, HIGH);
  analogWrite(LEFT_PWM, turnSpeed);
  digitalWrite(RIGHT1, HIGH);
  digitalWrite(RIGHT2, LOW);
  analogWrite(RIGHT_PWM, turnSpeed);
}

void turnRight() {
  digitalWrite(LEFT1, HIGH);
  digitalWrite(LEFT2, LOW);
  analogWrite(LEFT_PWM, turnSpeed);
  digitalWrite(RIGHT1, LOW);
  digitalWrite(RIGHT2, HIGH);
  analogWrite(RIGHT_PWM, turnSpeed);
}

void stopMotor() {
  // 1. 先切斷滑行比較嚴重的右輪動力
  analogWrite(RIGHT_PWM, 0);
  
  // 2. 稍微等待一下 (這個數值是關鍵，需要你實測微調！)
  // 通常設在 10 ~ 80 毫秒之間，視你車子的重量與馬達慣性而定
  delay(30); 
  
  // 3. 最後再切斷左輪動力，讓左輪剛好追上右輪的位置
  analogWrite(LEFT_PWM, 0);
}

// --- 主程式迴圈 ---
void loop() {
  if (Serial.available()) {
    char tempCmd = 0;
    while (Serial.available()) {
      tempCmd = Serial.read(); // 這裡會把剛才盲衝期間 Python 狂發的舊指令清空，只留最新的一筆
    }
    
    // 如果收到的新指令不是攻擊(A)也不是停止(S)，代表車子正在重新追蹤，將攻擊次數歸零
    if (tempCmd != 'A' && tempCmd != 'S') {
        attackCount = 0;
    }
    
    currentState = tempCmd; 
    lastCmdTime = millis(); 
  }

  // 失去訊號超過 3.5 秒就停車
  if (millis() - lastCmdTime > 3500) {
    currentState = 'S'; 
  }

  switch (currentState) {
    case 'A':
      attackCount++; // 每次進入 A 狀態，攻擊次數 +1
      
      if (attackCount >= 5) {
        // 脫困邏輯：連續攻擊 5 次失敗
        backward();           // 執行倒退
        delay(300);           // 倒退 0.3 秒 
        stopMotor();          // 倒退完煞車
        attackCount = 0;      // 重置計數器
        currentState = 'S';   // 強制回到停止狀態，等待 YOLO 重新判定
      } else {
        // 正常攻擊邏輯
        stopMotor();          // 攻擊前先停下車子
        delay(200);    
        myServo1.write(150);  // 瞬間全速揮到 150 度 (戳氣球)
        delay(100);           // 給馬達 0.1 秒的時間轉到定位
        myServo1.write(10);   // 瞬間全速收回到 10 度 (收回武器)
        delay(100);           // 給馬達 0.1 秒的時間轉回定位
        
        currentState = 'S';   // 確保戳完一次就停
      }
      break;
      
    case 'L': 
      turnLeft();
      delay(150);
      break;
    case 'C': 
      forward();
      delay(230);
      break;
    case 'R': 
      turnRight();
      delay(150);
      break;
    case 'N':
      turnRight(); 
      delay(250);
      break;
    case 'S':
      break;
  }

  stopMotor();
  if (currentState != 'S') {
     delay(2000); 
  }
}
