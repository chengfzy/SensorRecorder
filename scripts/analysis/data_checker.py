"""
Checker
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
        self.left_image_path = self.folder / 'left.csv'
        self.right_image_path = self.folder / 'right.csv'
        self.left_folder = self.folder / 'left'
        self.right_folder = self.folder / 'right'
        self.imu_path = self.folder / 'imu.csv'

        # raw IMU data
        self.timestamp = None  # s
        self.system_timestamp = None  # s
        self.acc = None  # m/s^2
        self.gyro = None  # rad/s

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

        # assign data
        if has_sys_time:
            self.timestamp = data[:, 0] * 1.E-9  # s
            self.system_timestamp = data[:, 1] * 1.E-9  #s
            self.acc = data[:, 2:5]  # m/s^2
            self.gyro = data[:, 5:8]  # rad/s
        else:
            self.timestamp = data[:, 0] * 1.E-9  # s
            self.acc = data[:, 1:4]  # m/s^2
            self.gyro = data[:, 4:7]  # rad/s

        # logging basic info
        logging.info(f'IMU time range = ({self.timestamp[0]}, {self.timestamp[-1]}), length = {len(self.timestamp)}')

    def __list_images(self) -> None:
        """
        List image files
        """
        if self.left_image_path.exists():
            logging.info(f'loading left image files from file "{self.left_image_path}"')
            f = open(self.left_image_path, 'r')
            files = f.readlines()
            f.close()
            self.left_images = [Path(v) for v in files]
        elif self.left_folder.exists():
            logging.info(f'loading left image files from folder "{self.left_folder}"')
            left_images = sorted([f for f in self.left_folder.iterdir() if f.is_file()])
            logging.info(f'left image count = {len(left_images)}')
            if len(left_images) > 0:
                self.left_images = left_images
        if self.right_image_path.exists():
            logging.info(f'loading right image files from file "{self.right_image_path}"')
            f = open(self.right_image_path, 'r')
            files = f.readlines()
            f.close()
            self.right_images = [Path(v) for v in files]
        elif self.right_folder.exists():
            logging.info(f'loading right image files from folder "{self.right_folder}"')
            right_images = sorted([f for f in self.right_folder.iterdir() if f.is_file()])
            logging.info(f'right image count = {len(right_images)}')
            if len(right_images) > 0:
                self.right_images = right_images

    def __frequency_check(self, plot_with_index=True) -> None:
        """
        Check frequency, whether some recording is lost
        """
        expect_imu_delta_time = 1. / 100.  # IMU 100 Hz
        expect_image_delta_time = 1. / 30  # Image 30 Hz

        # check IMU
        if self.timestamp is not None:
            print(util.Paragraph('Lost Recording for IMU'))
            lost_count = 0
            for n in range(len(self.timestamp) - 1):
                delta = (self.timestamp[n + 1] - self.timestamp[n])
                if delta > 2. * expect_imu_delta_time or delta < 0:
                    print(
                        f'[{n}/{len(self.timestamp)}] t = {self.timestamp[n]:.5f} s'
                        f', deltaTime = {delta * 1000:.5f} ms',
                        end='')
                    if self.system_timestamp is not None:
                        d = self.system_timestamp[n + 1] - self.system_timestamp[n]
                        print(f', system delta time = {d*1000:.5f} ms')
                    else:
                        print()
                    lost_count += 1
            print(f'IMU Lost Count = {lost_count}, Lost Ratio = {lost_count * 100. / (len(self.timestamp) - 1):.3f}%')

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
        if self.timestamp is None:
            return

        # plot
        fig = plt.figure('IMU Frequency', figsize=self.__figsize)
        # plot index - frequency
        ax = fig.add_subplot(211)
        delta_time = (self.timestamp[1:] - self.timestamp[:-1]) * 1000.  # ms
        ax.set_title('IMU Frequency')
        if plot_with_index or self.system_timestamp is None:
            ax.plot(range(len(self.timestamp) - 1), delta_time, 'b.', markersize=self.__markersize)
            ax.set_xlabel('Index')
        else:
            ax.plot(self.system_timestamp[:-1] - self.system_timestamp[0],
                    delta_time,
                    'b.',
                    markersize=self.__markersize)
            ax.set_xlabel('System Timestamp (s)')
        ax.set_ylabel('Delta Sensor Time (ms)')
        ax.grid(True)
        # plot index - timestamp
        ax = fig.add_subplot(212)
        ax.set_title('IMU Timestamp')
        ax.plot(range(len(self.timestamp)),
                self.timestamp * 1000,
                'b.',
                markersize=self.__markersize,
                label='Sensor Timestamp')
        ax.plot(range(len(self.system_timestamp)),
                self.system_timestamp * 1000,
                'r.',
                markersize=self.__markersize,
                label='System Timestamp')
        ax.set_xlabel('Index')
        ax.set_ylabel('Timestamp (ms)')
        ax.grid(True)
        ax.legend()
        fig.tight_layout()

    def __plot_image_frequency(self, plot_with_index=True) -> None:
        """
        Plot image frequency
        """
        # check sensor data
        if self.left_images is None and self.right_images is None:
            return

        # plot
        current_plot_index = 1
        fig = plt.figure('Image Frequency', figsize=self.__figsize)
        # plot left image
        if self.left_images is not None:
            ax = fig.add_subplot(211)
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
            # plot index - timestamp
            ax = fig.add_subplot(212)
            ax.set_title('Image Timestamp')
            ax.plot(range(len(timestamp)),
                    timestamp * 1000,
                    'b.',
                    markersize=self.__markersize,
                    label='Sensor Timestamp')
            ax.set_xlabel('Index')
            ax.set_ylabel('Timestamp (ms)')
            ax.grid(True)
            ax.legend()
            fig.tight_layout()

            # add current plot index
            current_plot_index += 1

        # plot right image
        if self.right_images is not None:
            ax = fig.add_subplot(111)
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
    parser = argparse.ArgumentParser(description='Checker for Recorded Data')
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
