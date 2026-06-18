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

PWRMON_HEADER_ROW_MIOT=6
PWRMON_HEADER_ROW_PPK=0

def _load_file_miot(file_name: str) -> pd.DataFrame:
    """
    Load the power measurements from the miot power monitor.

    file_name: Name of the file.

    Returns a Dataframe containing the measurements.
    """
    df = pd.read_csv(file_name, sep=';' ,header=PWRMON_HEADER_ROW_MIOT)
    df = df.rename(columns={'timestamp': 'timestamp_ms'})
    
    return df[['timestamp_ms', 'current_ua']]

def _load_file_ppk(file_name: str) -> pd.DataFrame:
    """
    Load the power measurements from the power profiler kit.

    file_name: Name of the file.

    Returns a Dataframe containing the measurements.
    """
    df = pd.read_csv(file_name, sep=',' ,header=PWRMON_HEADER_ROW_PPK)
    df = df.rename(columns={'Timestamp(ms)': 'timestamp_ms', 'Current(uA)': 'current_ua'})

    return df[['timestamp_ms', 'current_ua']]

def _load_file(file_name: str) -> pd.DataFrame:
    """
    Automatically detect file type and load power measurements.

    file_name: Name of the file.

    Returns a Dataframe containing the measurements.
    """
    with open(file_name, 'r') as f:
        l = f.readline()
        if '# Channels' in l:
            is_miot = True
        else:
            is_miot = False

    if is_miot:
        return _load_file_miot(file_name)

    return _load_file_ppk(file_name)

def _get_first_spike_ts(meas: pd.DataFrame):
    """
    Get the timestamp of the first spike.

    meas: The measurement dataframe.

    Return the timestamp of the first spike.
    """
    max_current = meas['current_ua'].max()
    avg_current = meas['current_ua'].mean()

    threshold_current = avg_current + (max_current - avg_current) / 2

    return meas[meas.current_ua > threshold_current].iloc[0]['timestamp_ms']

def _dump_meas_info(data: list, out: str):
    """
    Dump information regarding the measurement.

    data: List of data to dump info from.
    out: Name of the output file.
    """
    dump_tokens = []

    for d in data:
        df = d['df']

        dump_tokens.append(f'{d['name']}:')
        dump_tokens.append(f'\tmin   ={df.current_ua.min()}')
        dump_tokens.append(f'\tmax   ={df.current_ua.max()}')
        dump_tokens.append(f'\tmean  ={df.current_ua.mean()}')
        dump_tokens.append(f'\tmedian={df.current_ua.median()}')

    dump_str = '\n'.join(dump_tokens)

    if out:
        with open(f'{out}_info.txt', 'w') as f:
            f.write(dump_str)
    else:
        print(dump_str)
        

def process_files(measurement_name: str, reference_name: str, sync: bool, legend: str, out: str):
    """
    Plot the power measurements and reference.

    measurement_name: The file name of the measurement file.
    reference_name: The file name of the reference file.
    sync: Synchronize measurements and reference on first spike.
    legend: Plot legend names.
    out: Output file name.
    """

    meas = _load_file(measurement_name)

    if reference_name != None:
        plot_ref = True
        ref = _load_file(reference_name)
        num_labels = 2
    else:
        plot_ref = False
        num_labels = 1

    legend_lst = []

    if legend != None:
        legend_lst = legend.split(',')
        # Reverse because reference is plotted first
        legend_lst.reverse()

        if len(legend_lst) != num_labels:
            print('Wrong number of labels provided')
            legend_lst = []

    if sync and plot_ref:
        meas_spike = _get_first_spike_ts(meas)
        ref_spike = _get_first_spike_ts(ref)
        diff = ref_spike - meas_spike
        print(f'Adjusting reference by {diff} ms')

        ref.timestamp_ms = ref.timestamp_ms - diff

    dump_data = []

    dump_data.append({'df': meas, 'name': legend_lst[-1] if legend_lst else 'Measurement'})
    if plot_ref:
        dump_data.append({'df': ref, 'name': legend_lst[0] if legend_lst else 'Reference'})
    
    _dump_meas_info(dump_data, out)

    fig, ax = plt.subplots()

    if plot_ref:
        ref.plot(ax=ax,x='timestamp_ms', y='current_ua', xlabel='Zeit (ms)', ylabel='Strom (uA)', color='tab:orange')

    meas.plot(ax=ax, x='timestamp_ms', y='current_ua', xlabel='Zeit (ms)', ylabel='Strom (uA)', color='tab:blue')
    
    plt.legend(legend_lst)

    if out != None:
        plt.savefig(f'{out}.pdf', format='pdf', bbox_inches='tight')
    else:
        plt.show()
        

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        prog='pwrplot',
        description='Plot bus voltage and current from file')
    
    parser.add_argument('file', help='Input file', type=str)
    parser.add_argument('reference', help='Reference power measurement', type=str, nargs='?')
    parser.add_argument('-s', '--sync', help='Synchronize file and reference on first power spike', action='store_true')
    parser.add_argument('-l', '--legend', help='Comma separated list of labels for file and reference', type=str)
    parser.add_argument('-o', '--out', help='Output file name (excluding file ending)', type=str)

    args = parser.parse_args()
    process_files(args.file, args.reference, args.sync, args.legend, args.out)
