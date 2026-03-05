#!/usr/bin/env python

# Copyright (C) 2026  Lukas Gehreke
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

import argparse
import pandas as pd
import matplotlib.pyplot as plt

PWRMON_HEADER_ROW=6

def process_file(file_name):
    df = pd.read_csv(file_name, sep=';', index_col='timestamp' ,header=PWRMON_HEADER_ROW)
    df['current_ua'].plot()
    plt.show()
        

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        prog='pwrplot',
        description='Plot bus voltage and current from file')
    
    parser.add_argument('file', help='Input file', type=str)

    args = parser.parse_args()
    process_file(args.file)
