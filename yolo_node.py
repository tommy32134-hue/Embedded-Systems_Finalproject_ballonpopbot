import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
from std_msgs.msg import String
import cv2
from cv_bridge import CvBridge
from ultralytics import YOLO
import time

class YoloNode(Node):
    def __init__(self):
        super().__init__('yolo_node')
        
        # Subscribe to the camera node's image topic
        self.subscription = self.create_subscription(
            Image, 'camera/image_raw', self.image_callback, 10)
        self.bridge = CvBridge()
        
        # Publisher to broadcast the command ('L', 'R', 'C', 'N', 'A')
        self.publisher_ = self.create_publisher(String, 'cmd_dir', 10)
        
        # Load YOLO model
        self.model = YOLO("/home/tommy/best.pt")
        self.frame_width = 640
        self.left_bound = self.frame_width // 3
        self.right_bound = (self.frame_width // 3) * 2

        self.attack_area_threshold = 80000 

        # Inference Frequency Control Variables
        self.last_inference_time = 0
        self.inference_interval = 1

        self.get_logger().info('YOLO vision node started. Waiting for frames...')

    def image_callback(self, msg):
        frame = self.bridge.imgmsg_to_cv2(msg, "bgr8")
        current_time = time.time()

        # Display frame smoothly
        cv2.imshow("YOLO Vision", frame)
        cv2.waitKey(1)

        # Inference Throttle: Skip if less than interval has passed
        if current_time - self.last_inference_time < self.inference_interval:
            return 

        self.last_inference_time = current_time

        # Run YOLO inference
        results = self.model(frame, verbose=False, conf=0.6)
        cmd = "N"

        for r in results:
            boxes = r.boxes
            if len(boxes) > 0:
                largest_box = None
                max_area = 0
                
                for box in boxes:
                    x1, y1, x2, y2 = box.xyxy[0].tolist()
                    area = (x2 - x1) * (y2 - y1)
                    if area > max_area:
                        max_area = area
                        largest_box = box

                if largest_box:
                    x1, y1, x2, y2 = largest_box.xyxy[0].tolist()
                    center_x = int((x1 + x2) / 2)

                    if max_area > self.attack_area_threshold:
                        cmd = "A"  
                    elif center_x < self.left_bound:
                        cmd = "L"
                    elif center_x > self.right_bound:
                        cmd = "R"
                    else:
                        cmd = "C"

                    box_color = (0, 0, 255) if cmd == "A" else (0, 255, 0)
                    cv2.rectangle(frame, (int(x1), int(y1)), (int(x2), int(y2)), box_color, 2)
                    cv2.imshow("YOLO Vision", frame)
                
                break

        # Publish the command to ROS 2 topic
        msg_cmd = String()
        msg_cmd.data = cmd
        self.publisher_.publish(msg_cmd)
        area_log = max_area if 'max_area' in locals() else 0
        self.get_logger().info(f'Max Area: {int(area_log)} | Published cmd: {cmd}')

def main(args=None):
    rclpy.init(args=args)
    node = YoloNode()
    rclpy.spin(node)
    cv2.destroyAllWindows()
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()