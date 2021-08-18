"""
Checker for MYNT-EYE Sensor Data
"""

import sys
from pathlib import Path

sys.path.append(str(Path(__file__).parent.parent.resolve()))

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

        # plot configuration
        self.__figsize = (8, 6)
        self.__colors = ['b', 'r', 'k']
        self.__labels = ['X', 'Y', 'Z']
        self.__markersize = 3

    def check(self, plot_frequency=False, plot_with_index=True) -> None:
        # read sensor data
        self.__read_imu()
        self.__list_images()

        # check frequency(lost record)
        self.__frequency_check()

        # plot frequency
        if plot_frequency:
            self.__plot_imu_frequency(plot_with_index)
            self.__plot_image_frequency(plot_with_index)

            plt.show(block=True)

    def __read_imu(self) -> None:
        """
        Read IMU data
        """
        if not self.imu_path.exists():
            return

        # read imu
        logging.info(f'loading IMU data from file "{self.imu_path}"')
        data = np.loadtxt(str(self.imu_path), delimiter=',', skiprows=1)

        # check if it's empty data
        if len(data.shape) == 1:
            logging.warning('empty IMU data')
            return

        # check this file has system timestamp
        if data.shape[1] == 7:
            has_sys_time = False
        elif data.shape[1] == 8:
            has_sys_time = True
        logging.info(f'has system timestamp = {has_sys_time}')

        # split data to acc and gyro
        acc_timestamp = []  # 0.01 ms
        gyro_timestamp = []  # 0.01 ms
        split_acc = []  # g
        split_gyro = []  # rad/s
        for n in range(data.shape[0]):
            t = data[n, 0]
            if has_sys_time:
                g0 = data[n, 2:5]
                a0 = data[n, 5:8]
            else:
                g0 = data[n, 1:4]
                a0 = data[n, 4:7]
            if np.linalg.norm(a0) == 0:
                # only gyro
                gyro_timestamp.append(t)
                split_gyro.append(g0)
            elif np.linalg.norm(g0) == 0:
                # only acc
                acc_timestamp.append(t)
                split_acc.append(a0)

        # check if it's empty data
        if len(acc_timestamp) == 0 or len(gyro_timestamp) == 0:
            logging.warning('empty IMU data')
            return

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
            left_images = sorted([f for f in self.left_folder.iterdir() if f.is_file()])
            logging.info(f'left image count = {len(left_images)}')
            if len(left_images) > 0:
                self.left_images = left_images

        if self.right_folder.exists():
            logging.info(f'loading right image files from folder "{self.right_folder}"')
            right_images = sorted([f for f in self.right_folder.iterdir() if f.is_file()])
            logging.info(f'right image count = {len(right_images)}')
            if len(right_images) > 0:
                self.right_images = right_images

    def __frequency_check(self, plot_with_index=True) -> None:
        """
        Check frequency, whether some recording is lost
        """
        expect_imu_delta_time = 1. / 200.  # IMU 200 Hz
        expect_image_delta_time = 1. / 30  # Image 30 Hz

        # check IMU
        if self.acc_timestamp is not None:
            # check accelerator
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

    def __plot_imu_frequency(self, plot_with_index=True) -> None:
        """
        Plot IMU frequency
        """
        # check sensor data
        if self.acc_timestamp is None or self.gyro_timestamp is None:
            return

        # plot
        fig = plt.figure('IMU Frequency', figsize=self.__figsize)
        # plot accelerator
        ax = fig.add_subplot(211)
        delta_time = (self.acc_timestamp[1:] - self.acc_timestamp[:-1]) * 1000.  # ms
        ax.set_title('IMU Accelerator')
        if plot_with_index:
            ax.plot(range(len(self.acc_timestamp) - 1), delta_time, 'b.', markersize=self.__markersize)
            ax.set_xlabel('Index')
        else:
            ax.plot(self.acc_timestamp[:-1] - self.acc_timestamp[0], delta_time, 'b.', markersize=self.__markersize)
            ax.set_xlabel('Timestamp (s)')
        ax.set_ylabel('Delta Time (ms)')
        ax.grid(True)
        # plot gyroscope
        ax = fig.add_subplot(212)
        delta_time = (self.gyro_timestamp[1:] - self.gyro_timestamp[:-1]) * 1000.  # ms
        ax.set_title('IMU Gyroscope')
        if plot_with_index:
            ax.plot(range(len(self.gyro_timestamp) - 1), delta_time, 'b.', markersize=self.__markersize)
            ax.set_xlabel('Index')
        else:
            ax.plot(self.gyro_timestamp[:-1] - self.gyro_timestamp[0], delta_time, 'b.', markersize=self.__markersize)
            ax.set_xlabel('Timestamp (s)')
        ax.set_ylabel('Delta Time (ms)')
        ax.grid(True)
        fig.tight_layout()

    def __plot_image_frequency(self, plot_with_index=True) -> None:
        """
        Plot image frequency
        """
        # check sensor data
        if self.left_images is None and self.right_images is None:
            return

        # calculate plot number
        plot_num = 0
        if self.left_images is not None:
            plot_num += 1
        if self.right_images is not None:
            plot_num += 1

        # plot
        current_plot_index = 1
        fig = plt.figure('Image Frequency', figsize=self.__figsize)
        # plot left image
        if self.left_images is not None:
            ax = fig.add_subplot(plot_num, 1, current_plot_index)
            timestamp = np.array(sorted([float(f.stem) * 1E-9 for f in self.left_images]))  # s
            delta_time = (timestamp[1:] - timestamp[:-1]) * 1000.  # ms
            ax.set_title('Left Images')
            if plot_with_index:
                ax.plot(range(len(timestamp) - 1), delta_time, 'b.', markersize=self.__markersize)
                ax.set_xlabel('Index')
            else:
                ax.plot(timestamp[:-1] - timestamp[0], delta_time, 'b.', markersize=self.__markersize)
                ax.set_xlabel('Timestamp (s)')
            ax.set_ylabel('Delta Time (ms)')
            ax.grid(True)

            # add current plot index
            current_plot_index += 1

        # plot right image
        if self.right_images is not None:
            ax = fig.add_subplot(plot_num, 2, current_plot_index)
            timestamp = np.array(sorted([float(f.stem) * 1E-9 for f in self.right_images]))  # s
            delta_time = (timestamp[1:] - timestamp[:-1]) * 1000.  # ms
            ax.set_title('Right Images')
            if plot_with_index:
                ax.plot(range(len(timestamp) - 1), delta_time, 'b.', markersize=self.__markersize)
                ax.set_xlabel('Index')
            else:
                ax.plot(timestamp[:-1] - timestamp[0], delta_time, 'b.', markersize=self.__markersize)
                ax.set_xlabel('Timestamp (s)')
            ax.set_ylabel('Delta Time (ms)')
            ax.grid(True)
        fig.tight_layout()


if __name__ == '__main__':
    print(util.Title("Checker for MYNT-EYE Data"))

    # config logging
    logging.basicConfig(level=logging.INFO, format="[%(asctime)s %(levelname)s %(filename)s:%(lineno)d] %(message)s")

    # argument parser
    parser = argparse.ArgumentParser(description='Checker for MYNT-EYE Data')
    parser.add_argument('folder', type=str, help='input and save folder')
    parser.add_argument('--plot-frequency', action='store_true', help='whether to plot frequency')
    parser.add_argument('--plot-with-index', action='store_true', help='whether to plot x label with sensor index')
    args = parser.parse_args()
    print(args)

    if args.plot_frequency:
        import matplotlib.pyplot as plt

    # convert
    checker = Checker(Path(args.folder))
    checker.check(plot_frequency=args.plot_frequency, plot_with_index=args.plot_with_index)
