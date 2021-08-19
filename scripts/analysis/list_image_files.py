"""
List Image Files, and Save File Name to .csv File
"""

import sys
from pathlib import Path

sys.path.append(str(Path(__file__).parent.parent.resolve()))

import argparse
import logging
from typing import Tuple, List
import numpy as np
import util

if __name__ == '__main__':
    print(util.Title("List Images Files"))

    # config logging
    logging.basicConfig(level=logging.INFO, format="[%(asctime)s %(levelname)s %(filename)s:%(lineno)d] %(message)s")

    # argument parser
    parser = argparse.ArgumentParser(description='List Image Files and Save File Name to .csv File')
    parser.add_argument('folder', type=str, help='input and save folder')
    args = parser.parse_args()
    print(args)

    root_folder = Path(args.folder).resolve()
    names = ('left', 'right', 'images', 'cam')
    for n in names:
        image_path = root_folder / n
        if image_path.exists():
            files = sorted([f for f in image_path.iterdir() if f.is_file()])
            save_file = root_folder / (n + '.csv')
            logging.info(f'{image_path} => {save_file}')
            with open(save_file, 'w') as f:
                for v in files:
                    f.write(f'{v}\n')
