import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
import cv2
from cv_bridge import CvBridge

class CameraNode(Node):
    def __init__(self):
        super().__init__('camera_node')
        
        # Create image publisher
        self.publisher_ = self.create_publisher(Image, 'camera/image_raw', 10)
        
        # Capture frame every 0.1 seconds (10 FPS)
        self.timer = self.create_timer(0.1, self.timer_callback)
        
        self.cap = cv2.VideoCapture(0)
        self.bridge = CvBridge()
        self.get_logger().info('Camera node started. Transmitting frames...')

    def timer_callback(self):
        ret, frame = self.cap.read()
        if ret:
            # Convert OpenCV frame to ROS 2 Image message
            msg = self.bridge.cv2_to_imgmsg(frame, "bgr8")
            self.publisher_.publish(msg)

def main(args=None):
    rclpy.init(args=args)
    node = CameraNode()
    rclpy.spin(node)
    node.cap.release()
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()