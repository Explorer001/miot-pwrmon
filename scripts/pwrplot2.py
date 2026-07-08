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
import seaborn as sb
import matplotlib.pyplot as plt

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        prog='pwrplot',
        description='Plot mean current of multiple measurements')
    
    parser.add_argument('file', help='Input file', type=str)
    parser.add_argument('-o', '--out', help='Output file name (excluding file ending)', type=str)

    args = parser.parse_args()

    df = pd.read_csv(args.file ,header=0)
    print(df)

    fig, ax = plt.subplots()
    sb.boxplot(x='id', y='mean_current_ua', data=df, ax=ax).set(xlabel='UID', ylabel='Mittlerer Stromverbrauch (uA)')
    plt.show()
