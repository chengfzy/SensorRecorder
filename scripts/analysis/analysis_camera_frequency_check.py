"""
Check if Some Images are Lost for MYNT-EYE Camera
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


class Checker:

    def __init__(self, folder: Path) -> None:
        # list image files
        logging.info(f'list image files from folder "{folder}"')
        self.image_files = sorted([f for f in folder.iterdir() if f.is_file()])

        # plot configuration
        self.__figsize = (8, 6)
        self.__colors = ['b', 'r', 'k']
        self.__labels = ['X', 'Y', 'Z']
        self.__markersize = 3

    def check(self) -> None:
        # check frequency(lost record)
        self.__frequency_check()

    def __frequency_check(self) -> None:
        """
        Check image frequency, or whether some recording is lost
        """
        expect_delta_time = 1. / 30  # 30 Hz
        plot_with_index = True

        timestamp = np.array(sorted([int(f.stem) for f in self.image_files]))  # ns

        # check accelerator
        print(util.Paragraph('Lost Recording for Image'))
        lost_count = 0
        for n in range(len(timestamp) - 1):
            delta = (timestamp[n + 1] - timestamp[n]) * 1.E-9
            if delta > 2. * expect_delta_time or delta < 0:
                print(f'[{n}/{len(timestamp)}] t = {timestamp[n] * 1.E-9:.5f} s, deltaTime = {delta * 1000:.5f} ms')
                lost_count += 1
        print(f'Lost Count = {lost_count}, Lost Ratio = {lost_count * 100. / (len(timestamp) - 1):.3f}%')

        # plot delta time
        fig = plt.figure('Image Frequency', figsize=self.__figsize)
        delta_time = (timestamp[1:] - timestamp[:-1]) * 1.E-6  # ms
        ax = fig.add_subplot()
        ax.set_title('Image')
        if plot_with_index:
            ax.plot(range(len(timestamp) - 1), delta_time, 'b.', markersize=self.__markersize)
            ax.set_xlabel('Index')
        else:
            ax.plot(timestamp[:-1] - timestamp[0], delta_time, 'b.', markersize=self.__markersize)
            ax.set_xlabel('Timestamp (s)')
        ax.set_ylabel('Delta Time (ms)')
        ax.grid(True)


if __name__ == '__main__':
    print(util.Title("Check MYNT-EYE Frequency"))

    # config logging
    logging.basicConfig(level=logging.INFO, format="[%(asctime)s %(levelname)s %(filename)s:%(lineno)d] %(message)s")

    # argument parser
    parser = argparse.ArgumentParser(description='Check MYNT-EYE Frequency')
    parser.add_argument('folder', type=str, help='image folder')
    args = parser.parse_args()

    # check
    checker = Checker(Path(args.folder))
    checker.check()
    plt.show(block=True)
