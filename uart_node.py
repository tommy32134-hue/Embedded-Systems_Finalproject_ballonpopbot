import rclpy
from rclpy.node import Node
from std_msgs.msg import String
import serial
import time


class UartNode(Node):
    def __init__(self):
        super().__init__("uart_node")
        self.ser = None

        try:
            self.ser = serial.Serial("/dev/ttyUSB0", 9600, timeout=1)
        except:
            try:
                self.ser = serial.Serial("/dev/ttyACM0", 9600, timeout=1)
            except:
                self.get_logger().error("Arduino not found.")

        if self.ser:
            time.sleep(2)

        self.subscription = self.create_subscription(
            String, "cmd_dir", self.listener_callback, 10
        )

    def listener_callback(self, msg):
        cmd = msg.data
        if self.ser and self.ser.is_open:
            self.ser.write(cmd.encode("utf-8"))


def main(args=None):
    rclpy.init(args=args)
    node = UartNode()
    rclpy.spin(node)
    if node.ser:
        node.ser.close()
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
