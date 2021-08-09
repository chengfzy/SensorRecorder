"""
Prepare Images and IMU Data Obtained from MYNT-EYE Camera for Calibration using kalibr
"""

import sys
from pathlib import Path

sys.path.append(str(Path(__file__).parent.parent.resolve()))
import argparse
import logging
from typing import Tuple, List
from pathlib import Path
import numpy as np
import matplotlib.pyplot as plt
import util


class ImuConvertor:

    def __init__(self, folder: Path) -> None:

        self.folder = folder
        # get IMU file
        if (self.folder / 'raw_imu.csv').exists():
            self.imu_path = self.folder / 'raw_imu.csv'
        elif (self.folder / 'imu.csv').exists():
            self.imu_path = self.folder / 'imu.csv'
            self.imu_path.rename(self.folder / 'raw_imu.csv')
            self.imu_path = self.folder / 'raw_imu.csv'

        # gravity constant
        self.__kg = 9.81

        # raw IMU data
        self.acc_timestamp = None  # 0.01 ms
        self.gyro_timestamp = None  # 0.01 ms
        self.split_acc = None  # g
        self.split_gyro = None  # deg/s

        # converted IMU data
        self.timestamp = None  # ns
        self.acc = None  # m/s^2
        self.gyro = None  # rad/s

        # plot configuration
        self.__figsize = (8, 6)
        self.__colors = ['b', 'r', 'k']
        self.__labels = ['X', 'Y', 'Z']
        self.__markersize = 3

    def convert(self) -> None:
        # read IMU, timestamp: 0.01 ms, acc: g, gyro: deg/s
        self.__read_imu()

        # interpolate gyro at acc timestamp to obtain packed acc and gyro
        self.__interpolate_imu()

        # save packed IMU data to file
        save_path = self.folder / 'imu.csv'
        logging.info(f'save converted IMU data to file "{save_path}"')
        fs = open(str(save_path), 'w')
        fs.write(f'#Timestamp(ns),GyroX(rad/s),GyroY(rad/s),GyroZ(rad/s),AccX(m/s^2),AccZ(m/s^2),AccZ(m/s^2)\n')
        for n in range(len(self.timestamp)):
            fs.write(f'{self.timestamp[n]},{self.gyro[n, 0]},{self.gyro[n, 1]},{self.gyro[n, 2]}'
                     f',{self.gyro[n, 0]},{self.gyro[n, 1]},{self.gyro[n, 2]}\n')
        fs.close()

        # plot IMU data
        self.__plot_imu()

        # check frequency(lost record)
        self.__frequency_check()

    def __read_imu(self) -> None:
        """
        Read IMU data
        """
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
        self.acc_timestamp = np.array(acc_timestamp)  # 0.01 ms
        self.gyro_timestamp = np.array(gyro_timestamp)  # 0.01 ms
        self.split_acc = np.array(split_acc)  # g
        self.split_gyro = np.array(split_gyro)  # deg/s

    def __interpolate_imu(self) -> None:
        """
        Interpolation gyro at acc timestamp to obtained the packed IMU data
        """
        self.timestamp = self.acc_timestamp.copy()  # set timestamp to acc timestamp
        self.acc = self.split_acc.copy()

        # make sure the first gyro timestamp is less than the first (acc) timestamp, and the last gyro timestamp is
        # greater than (acc) timestamp
        idx0 = np.argmax(self.timestamp > self.gyro_timestamp[0])
        idx1 = np.argmax(self.timestamp > self.gyro_timestamp[-1])
        if idx1 == 0:
            idx1 = len(self.timestamp)
        logging.info(f'crop acc range ({idx0}:{idx1}) to meet the gyro timestamp')
        self.timestamp = self.timestamp[idx0:idx1]
        self.acc = self.acc[idx0:idx1, :]

        # init gyro data
        self.gyro = np.zeros_like(self.acc)

        # find first gyro timestamp
        for n in range(len(self.timestamp)):
            # calculate current gyro value
            current_idx = np.argmax(self.gyro_timestamp > self.timestamp[n])
            last_idx = current_idx - 1
            # print(f'last index = {last_idx}, current index = {current_idx}')

            t = self.timestamp[n]
            t0, t1 = self.gyro_timestamp[last_idx], self.gyro_timestamp[current_idx]
            g0, g1 = self.split_gyro[last_idx], self.split_gyro[current_idx]
            g = (g1 - g0) / (t1 - t0) * (t - t0) + g0
            self.gyro[n, :] = g

        # convert unit
        # self.timestamp = self.timestamp * 1.E4  # 0.01 ms => ns
        # self.acc = self.acc * self.__kg  # g => m/s^2
        # self.gyro = self.gyro * np.pi / 180.  # deg/s => rad/s

        self.timestamp = self.timestamp  #  ns
        self.acc = self.acc  # g => m/s^2
        self.gyro = self.gyro  # deg/s => rad/s

    def __plot_imu(self) -> None:
        """
        Plot IMU data
        """
        # plot acceleration
        fig = plt.figure('IMU Acceleration', figsize=self.__figsize)
        for m in range(0, 3):
            ax = fig.add_subplot(3, 1, m + 1)
            ax.plot(self.acc_timestamp * 1.E-9, self.split_acc[:, m], 'b.', linewidth=1.0)
            ax.plot(self.timestamp * 1.E-9, self.acc[:, m], 'r-', linewidth=1.0)
            ax.set_title('Acceleration - ' + self.__labels[m])
            ax.set_xlabel('Time (s)')
            ax.set_ylabel('Acceleration (m/s^2)')
            ax.grid(True)
        fig.tight_layout()

        # plot gyroscope
        fig = plt.figure('IMU Gyroscope', figsize=self.__figsize)
        for m in range(0, 3):
            ax = fig.add_subplot(3, 1, m + 1)
            ax.plot(self.gyro_timestamp * 1.E-9, self.split_gyro[:, m], 'b.', linewidth=1.0)
            ax.plot(self.timestamp * 1.E-9, self.gyro[:, m], 'r-', linewidth=1.0)
            ax.set_title('Gyroscope - ' + self.__labels[m])
            ax.set_xlabel('Time (s)')
            ax.set_ylabel('Acceleration (rad/s)')
            ax.grid(True)
        fig.tight_layout()

    def __frequency_check(self) -> None:
        """
        Check IMU frequency, or whether some recording is lost
        """
        expect_delta_time = 1. / 200  # 200 Hz
        plot_with_index = True

        # check accelerator
        print(util.Paragraph('Lost Recording for IMU Accelerator'))
        lost_count = 0
        for n in range(len(self.acc_timestamp) - 1):
            delta = (self.acc_timestamp[n + 1] - self.acc_timestamp[n]) * 1.E-9
            if delta > 2. * expect_delta_time or delta < 0:
                print(f'[{n}/{len(self.acc_timestamp)}] t = {self.acc_timestamp[n]:.5f} s'
                      f', deltaTime = {delta * 100:.5f} ms')
                lost_count += 1
        print(f'Lost Count = {lost_count}, Lost Ratio = {lost_count * 100. / (len(self.acc_timestamp) - 1):.3f}%')
        # check gyroscope
        print(util.Paragraph('Lost Recording for IMU Gyroscope'))
        lost_count = 0
        for n in range(len(self.gyro_timestamp) - 1):
            delta = (self.gyro_timestamp[n + 1] - self.gyro_timestamp[n]) * 1.E-9
            if delta > 2. * expect_delta_time or delta < 0:
                print(f'[{n}/{len(self.gyro_timestamp)}] t = {self.gyro_timestamp[n]:.5f} s'
                      f', deltaTime = {delta * 1000:.5f} ms')
                lost_count += 1
        print(f'Lost Count = {lost_count}, Lost Ratio = {lost_count * 100. / (len(self.gyro_timestamp) - 1):.3f}%')

        # plot delta time
        fig = plt.figure('IMU Frequency', figsize=self.__figsize)
        # accelerator
        delta_time = (self.acc_timestamp[1:] - self.acc_timestamp[:-1]) * 1.E-6  # ms
        ax = fig.add_subplot(211)
        ax.set_title('Accelerator')
        if plot_with_index:
            ax.plot(range(len(self.acc_timestamp) - 1), delta_time, 'b.', markersize=self.__markersize)
            ax.set_xlabel('Index')
        else:
            ax.plot(self.acc_timestamp[:-1] - self.acc_timestamp[0], delta_time, 'b.', markersize=self.__markersize)
            ax.set_xlabel('Timestamp (s)')
        ax.set_ylabel('Delta Time (ms)')
        ax.grid(True)
        # gyroscope
        delta_time = (self.gyro_timestamp[1:] - self.gyro_timestamp[:-1]) * 1.E-6
        ax = fig.add_subplot(212)
        ax.set_title('Gyroscope')
        if plot_with_index:
            ax.plot(range(len(self.gyro_timestamp) - 1), delta_time, 'b.', markersize=self.__markersize)
            ax.set_xlabel('Index')
        else:
            ax.plot((self.gyro_timestamp[:-1] - self.gyro_timestamp[0]) * 1.E-5,
                    delta_time,
                    'b.',
                    markersize=self.__markersize)
            ax.set_xlabel('Timestamp (s)')
        ax.set_ylabel('Delta Time (ms)')
        ax.grid(True)
        fig.tight_layout()


if __name__ == '__main__':
    print(util.Title("Prepare MYNT-EYE Data"))

    # config logging
    logging.basicConfig(level=logging.INFO, format="[%(asctime)s %(levelname)s %(filename)s:%(lineno)d] %(message)s")

    # argument parser
    parser = argparse.ArgumentParser(
        description='Prepare Images and IMU Data Obtained from MYNT-EYE Camera for Calibration using kalibr')
    parser.add_argument('folder', type=str, help='input and save folder')
    args = parser.parse_args()

    # convert
    converter = ImuConvertor(Path(args.folder))
    converter.convert()
    plt.show(block=True)
