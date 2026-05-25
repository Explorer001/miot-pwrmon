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

def process_file(file):
    with open(file, 'r') as f:
        lines = f.readlines()
    with open(file, 'w') as f:
        start_found = False
        for line in lines:
            if start_found:
                f.write(line)
                continue

            if '# Channels' in line:
                start_found = True
                f.write(line)
        
        if start_found == False:
            print(f'Could not convert file: {file}')
        

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        prog='pwrconv',
        description='Convert power files by removing nodes startup prints')
    
    parser.add_argument('file', help='Input file', nargs='+', type=str)

    args = parser.parse_args()
    for file in args.file:
        process_file(file)