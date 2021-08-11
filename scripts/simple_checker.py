"""
Prepare Images and IMU Data Obtained from MYNT-EYE Camera for Calibration using kalibr
"""

from pathlib import Path
import argparse
import logging
from typing import Tuple, List
import numpy as np
import util


class Checker:

    def __init__(self, folder: Path) -> None:
        self.folder = folder.resolve()
        # get image folder
        self.left_folder = self.folder / 'left'
        self.right_folder = self.folder / 'right'
        self.imu_path = self.folder / 'imu.csv'

        # raw IMU data
        self.acc_timestamp = None  # s
        self.gyro_timestamp = None  # s
        self.split_acc = None  # m/s^2
        self.split_gyro = None  # rad/s

        # image files
        self.left_images = None
        self.right_images = None

    def check(self) -> None:
        # read sensor data
        self.__read_imu()
        self.__list_images()

        # check frequency(lost record)
        self.__frequency_check()

    def __read_imu(self) -> None:
        """
        Read IMU data
        """
        if not self.imu_path.exists():
            return

        # read imu
        logging.info(f'loading IMU data from file "{self.imu_path}"')
        data = np.loadtxt(str(self.imu_path), delimiter=',', skiprows=1)

        # split data to acc and gyro
        acc_timestamp = []  # 0.01 ms
        gyro_timestamp = []  # 0.01 ms
        split_acc = []  # g
        split_gyro = []  # rad/s
        for n in range(data.shape[0]):
            t = data[n, 0]
            a0 = data[n, 1:4]
            g0 = data[n, 4:7]
            if np.linalg.norm(a0) == 0:
                # only gyro
                gyro_timestamp.append(t)
                split_gyro.append(g0)
            elif np.linalg.norm(g0) == 0:
                # only acc
                acc_timestamp.append(t)
                split_acc.append(a0)

        # logging basic info
        logging.info(f' acc time range = ({acc_timestamp[0]}, {acc_timestamp[-1]}), data length = {len(acc_timestamp)}')
        logging.info(f'gyro time range = ({gyro_timestamp[0]}, {gyro_timestamp[-1]})'
                     f', data length = {len(gyro_timestamp)}')

        # assign data
        self.acc_timestamp = np.array(acc_timestamp) * 1.E-9  # s
        self.gyro_timestamp = np.array(gyro_timestamp) * 1.E-9  # s
        self.split_acc = np.array(split_acc)  # m/s^2
        self.split_gyro = np.array(split_gyro)  # rad/s

    def __list_images(self) -> None:
        """
        List image files
        """
        if self.left_folder.exists():
            logging.info(f'loading left image files from folder "{self.left_folder}"')
            self.left_images = sorted([f for f in self.left_folder.iterdir() if f.is_file()])
            logging.info(f'left image count = {len(self.left_images)}')

        if self.right_folder.exists():
            logging.info(f'loading right image files from folder "{self.right_folder}"')
            self.right_images = sorted([f for f in self.right_folder.iterdir() if f.is_file()])
            logging.info(f'right image count = {len(self.right_images)}')

    def __frequency_check(self) -> None:
        """
        Check frequency, or whether some recording is lost
        """
        expect_imu_delta_time = 1. / 200.  # IMU 200 Hz
        expect_image_delta_time = 1. / 30  # Image 30 Hz

        # check accelerator
        if self.acc_timestamp is not None:
            print(util.Paragraph('Lost Recording for IMU Accelerator'))
            lost_count = 0
            for n in range(len(self.acc_timestamp) - 1):
                delta = (self.acc_timestamp[n + 1] - self.acc_timestamp[n])
                if delta > 2. * expect_imu_delta_time or delta < 0:
                    print(f'[{n}/{len(self.acc_timestamp)}] t = {self.acc_timestamp[n]:.5f} s'
                          f', deltaTime = {delta * 1000:.5f} ms')
                    lost_count += 1
            print(f'IMU Accelerator, Lost Count = {lost_count}'
                  f', Lost Ratio = {lost_count * 100. / (len(self.acc_timestamp) - 1):.3f}%')

        # check gyroscope
        if self.gyro_timestamp is not None:
            print(util.Paragraph('Lost Recording for IMU Gyroscope'))
            lost_count = 0
            for n in range(len(self.gyro_timestamp) - 1):
                delta = (self.gyro_timestamp[n + 1] - self.gyro_timestamp[n])
                if delta > 2. * expect_imu_delta_time or delta < 0:
                    print(f'[{n}/{len(self.gyro_timestamp)}] t = {self.gyro_timestamp[n]:.5f} s'
                          f', deltaTime = {delta * 1000:.5f} ms')
                    lost_count += 1
            print(f'IMU Gyroscope, Lost Count = {lost_count}'
                  f', Lost Ratio = {lost_count * 100. / (len(self.gyro_timestamp) - 1):.3f}%')

        # check left images
        if self.left_images is not None:
            print(util.Paragraph('Lost Recording for Left Image'))
            lost_count = 0
            timestamp = np.array(sorted([float(f.stem) * 1E-9 for f in self.left_images]))  # s
            for n in range(len(timestamp) - 1):
                delta = (timestamp[n + 1] - timestamp[n])
                if delta > 1.5 * expect_image_delta_time or delta < 0:
                    print(f'[{n}/{len(timestamp)}] t = {timestamp[n]:.5f} s, deltaTime = {delta * 1000:.5f} ms')
                    lost_count += 1
            print(f'Left Image, Lost Count = {lost_count}'
                  f', Lost Ratio = {lost_count * 100. / (len(timestamp) - 1):.3f}%')

        # check right images
        if self.right_images is not None:
            print(util.Paragraph('Lost Recording for Right Image'))
            lost_count = 0
            timestamp = np.array(sorted([float(f.stem) * 1E-9 for f in self.right_images]))  # s
            for n in range(len(timestamp) - 1):
                delta = (timestamp[n + 1] - timestamp[n])
                if delta > 1.5 * expect_image_delta_time or delta < 0:
                    print(f'[{n}/{len(timestamp)}] t = {timestamp[n]:.5f} s, deltaTime = {delta * 1000:.5f} ms')
                    lost_count += 1
            print(f'Right Image, Lost Count = {lost_count}'
                  f', Lost Ratio = {lost_count * 100. / (len(timestamp) - 1):.3f}%')


if __name__ == '__main__':
    print(util.Title("Frequency Checking for MYNT-EYE Data"))

    # config logging
    logging.basicConfig(level=logging.INFO, format="[%(asctime)s %(levelname)s %(filename)s:%(lineno)d] %(message)s")

    # argument parser
    parser = argparse.ArgumentParser(description='Frequency Checking for MYNT-EYE Data')
    parser.add_argument('folder', type=str, help='input and save folder')
    args = parser.parse_args()

    # convert
    checker = Checker(Path(args.folder))
    checker.check()
