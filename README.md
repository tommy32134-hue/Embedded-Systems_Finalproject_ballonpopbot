# 嵌入式系統_Finalproject_ballonpopbot
一台能自動辨識並戳破氣球的自走車。採用 Raspberry Pi + Arduino 上下層架構：上層在 Linux 上以 ROS 2 執行影像辨識與決策，下層以 Arduino 韌體驅動馬達與攻擊機構。
## 功能
以自行訓練的 YOLOv8 模型即時偵測氣球
依目標框的中心位置與面積判斷方位與距離，自動轉向、接近並攻擊
ROS 2 三節點架構，影像、計算、通訊獨立執行
Arduino 端以狀態機處理指令，並內建通訊逾時停車、脫困、等安全機制
