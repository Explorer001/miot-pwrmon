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

PWRMON_HEADER_ROW=6

def get_id_from_file(file: str) -> int:
    parts = file.split('/')
    _, name = parts[-1].split('-')
    name, _ = name.split('.')
    id = name.strip('Node')
    return int(id)

def process_file(file: str) -> pd.DataFrame:
    try:
        node_id = get_id_from_file(file)
    except:
        print(f'Could not get node id frome file: {file}')
        return pd.DataFrame()

    df = pd.read_csv(file, sep=';' ,header=PWRMON_HEADER_ROW)

    res_df = pd.DataFrame({
        'id': node_id,
        'duration_ms': df['timestamp'].max(),
        'min_current_ua': df['current_ua'].min(),
        'max_current_ua': df['current_ua'].max(),
        'mean_current_ua': df['current_ua'].mean()
        }, index=[0])

    return res_df

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        prog='pwrconv',
        description='Analyze power measurements')
    
    parser.add_argument('-o', '--out', help='Output file', type=str, default='pwr.csv')
    parser.add_argument('file', help='Input file', nargs='+', type=str)

    args = parser.parse_args()

    frames = []

    for file in args.file:
        try:
            frame = process_file(file)
        except Exception as e:
            print(f'Failed to analyze {file}: {e}')
        frames.append(frame)

    pwr_frame = pd.concat(frames, ignore_index=True)

    pwr_frame.to_csv(args.out, mode='w')
