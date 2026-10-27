# 嵌入式系統_Finalproject_ballonpopbot
一台能自動辨識並戳破氣球的自走車。採用 Raspberry Pi + Arduino 上下層架構：上層在 Linux 上以 ROS 2 執行影像辨識與決策，下層以 Arduino 韌體驅動馬達與攻擊機構。
## 功能
以自行訓練的 YOLOv8 模型即時偵測氣球
依目標框的中心位置與面積判斷方位與距離，自動轉向、接近並攻擊
ROS 2 三節點架構，影像、計算、通訊獨立執行
Arduino 端以狀態機處理指令，並內建通訊逾時停車、脫困、等安全機制
## ROS2節點說明（節錄重點部分）
#### camera_node.py
  以 ROS 2 Timer 固定週期擷取影像，透過 CvBridge 將 OpenCV 的 numpy 陣列轉為 sensor_msgs/Image，發布到 camera/image_raw。
  ```python
self.publisher_ = self.create_publisher(Image, 'camera/image_raw', 10)
self.timer = self.create_timer(0.1, self.timer_callback)   # 每 0.1 秒觸發一次（10 FPS）
self.cap = cv2.VideoCapture(0)
self.bridge = CvBridge()

def timer_callback(self):
    ret, frame = self.cap.read()
    if ret:
        msg = self.bridge.cv2_to_imgmsg(frame, "bgr8")      # OpenCV 影像 → ROS 2 Image
        self.publisher_.publish(msg)
```
#### yolo_node.py
  推論影像並做出決策。
  攝影機以 10 FPS 送入影像，但 Raspberry Pi 無法每幀都跑 YOLO。這裡讓畫面持續更新，推論則每 1 秒才執行一次，在即時顯示與運算負載之間取得平衡。
  ```python
cv2.imshow("YOLO Vision", frame)      # 每張影像都即時顯示
cv2.waitKey(1)

if current_time - self.last_inference_time < self.inference_interval:
    return                            # 未達間隔就跳過推論
self.last_inference_time = current_time
```
  以信心門檻 0.6 過濾誤判，畫面中有多顆氣球時，選擇氣球面積最大者作為目標。面積大代表距離近，優先處理最近的氣球。
  ```python
results = self.model(frame, verbose=False, conf=0.6)

for box in boxes:
    x1, y1, x2, y2 = box.xyxy[0].tolist()
    area = (x2 - x1) * (y2 - y1)
    if area > max_area:
        max_area = area
        largest_box = box
```
  決策樹。以氣球面積當作距離的估計，只靠單一鏡頭就能判斷夠不夠近。
  以中心 x 座標 判斷方位：畫面切成左、中、右三等分，決定轉向或直行。
  預設指令為 `N`，沒偵測到氣球時讓車子原地旋轉搜尋。
  ```python
center_x = int((x1 + x2) / 2)

if max_area > self.attack_area_threshold:   # 80000 px²
    cmd = "A"                               # 夠近 → 攻擊
elif center_x < self.left_bound:            # 畫面左 1/3
    cmd = "L"
elif center_x > self.right_bound:           # 畫面右 1/3
    cmd = "R"
else:
    cmd = "C"                               # 在中間 → 前進接近
```
#### uart_node.py
	將指令以 UART 傳送給 Arduino，自動偵測序列埠。
  ```python
try:
    self.ser = serial.Serial("/dev/ttyUSB0", 9600, timeout=1)
except:
    try:
        self.ser = serial.Serial("/dev/ttyACM0", 9600, timeout=1)
    except:
        self.get_logger().error("Arduino not found.")
```
##Arudino底層控制說明
#### balloon_final.ino
  依狀態機執行動作
  ```cpp
switch (currentState) {
  case 'L': turnLeft();  delay(150); break;   // 左轉一下
  case 'R': turnRight(); delay(150); break;   // 右轉一下
  case 'C': forward();   delay(230); break;   // 往前走一點
  case 'N': turnRight(); delay(250); break;   // 沒看到氣球，轉圈找
  case 'A': stopMotor();           // 攻擊
            delay(200);
            myServo1.write(150);  // 伺服馬達揮到 150 度，針頭往下戳
            delay(100);
            myServo1.write(10);   // 收回到 10 度
            delay(100);
                 
}
stopMotor();
delay(2000);   // 每個動作做完都停 2 秒
```
  安全機制，連續攻擊 5 次都沒成功 → 倒退脫困
  ```cpp
if (attackCount >= 5) {
  backward();
  delay(300);
  stopMotor();
  attackCount = 0;
}
```





