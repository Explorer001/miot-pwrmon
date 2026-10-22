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
import matplotlib.colors as mcolors
from matplotlib.collections import LineCollection
import numpy as np

PWRMON_HEADER_ROW_MIOT=6
PWRMON_HEADER_ROW_PPK=0

EXPORT_FIG_WIDTH=12
EXPORT_FIG_HEIGHT=8

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
        dump_tokens.append(f'\tmin        ={df.current_ua.min()}')
        dump_tokens.append(f'\tmax        ={df.current_ua.max()}')
        dump_tokens.append(f'\tmean       ={df.current_ua.mean()}')
        dump_tokens.append(f'\tmedian     ={df.current_ua.median()}')

        if 'thres' in df.columns:
            df_lo = df[df.thres < 0]
            df_mid = df[df.thres == 0]
            df_hi = df[df.thres > 0]

            dump_tokens.append(f'\tz_thres    ={d['z_thres']}')
            dump_tokens.append(f'\tlo_mean    ={df_lo.current_ua.mean()}')
            dump_tokens.append(f'\tlo_count   ={df_lo.current_ua.count()}')
            dump_tokens.append(f'\tmid_mean   ={df_mid.current_ua.mean()}')
            dump_tokens.append(f'\tmid_count  ={df_mid.current_ua.count()}')
            dump_tokens.append(f'\thi_mean    ={df_hi.current_ua.mean()}')
            dump_tokens.append(f'\thi_count   ={df_hi.current_ua.count()}')
            dump_tokens.append(f'\ttotal_count={df.current_ua.count()}')


    dump_str = '\n'.join(dump_tokens)

    if out:
        with open(f'{out}_info.txt', 'w') as f:
            f.write(dump_str)
    else:
        print(dump_str)

def _z_map(x, thres):
    if x <= -thres:
        return -1
    elif x >= thres:
        return 1
    else:
        return 0

def process_files(measurement_name: str, reference_name: str, sync: bool, legend: str, out: str, fontsize: int, time_range: (int, int), z_thres: float):
    """
    Plot the power measurements and reference.

    measurement_name: The file name of the measurement file.
    reference_name: The file name of the reference file.
    sync: Synchronize measurements and reference on first spike.
    legend: Plot legend names.
    out: Output file name.
    fontsize: Plot fontsize
    time_range: Time range to plot. If None the whole range is plotted
    z_thres: Z threshold for spike detection
    """

    meas = _load_file(measurement_name)

    if reference_name != None:
        plot_ref = True
        ref = _load_file(reference_name)
        num_labels = 2
    else:
        plot_ref = False
        num_labels = 1

    if plot_ref and z_thres is not None:
        print('Plotting reference is not supported when using z-threshold')
        return

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

    if time_range is not None:
        start_ms, end_ms = time_range

        meas = meas[meas['timestamp_ms'].between(start_ms, end_ms, inclusive='both')]
        if plot_ref:
            ref = ref[ref['timestamp_ms'].between(start_ms, end_ms, inclusive='both')]

    plt.rcParams.update({'font.size': fontsize})

    fig, ax = plt.subplots()

    if z_thres is None:
        if plot_ref:
            ref.plot(ax=ax,x='timestamp_ms', y='current_ua', xlabel='Zeit (ms)', ylabel='Strom (uA)', color='tab:orange')

        meas.plot(ax=ax, x='timestamp_ms', y='current_ua', xlabel='Zeit (ms)', ylabel='Strom (uA)', color='tab:blue')
    else:
        meas['current_ua_mod_z'] = (meas.current_ua - meas.current_ua.median()) / meas.current_ua.std()
        meas['thres'] = meas.current_ua_mod_z.apply(lambda x: _z_map(x, z_thres))

        xy = np.column_stack([meas.timestamp_ms, meas.current_ua]).reshape(-1, 1, 2)
        line_segments = np.concatenate([xy[:-1], xy[1:]], axis=1)

        cmap = mcolors.ListedColormap(['tab:green', 'tab:blue', 'tab:red'])

        line_coll = LineCollection(line_segments, cmap=cmap)
        line_coll.set_array(meas.thres)

        ax.add_collection(line_coll)
        ax.autoscale()

    plt.legend(legend_lst, framealpha=0.5)

    dump_data = []

    dump_data.append({'df': meas, 'name': legend_lst[-1] if legend_lst else 'Measurement', 'z_thres': z_thres})
    if plot_ref:
        dump_data.append({'df': ref, 'name': legend_lst[0] if legend_lst else 'Reference', 'z_thres': z_thres})
    
    _dump_meas_info(dump_data, out)

    if out != None:
        fig.set_figheight(EXPORT_FIG_HEIGHT)
        fig.set_figwidth(EXPORT_FIG_WIDTH)
        plt.tight_layout()
        plt.savefig(f'{out}.pdf', format='pdf', bbox_inches='tight')
    else:
        plt.show()
        

def parse_range(val):
    try:
        start, end = val.split('-')
        return (int(start), int(end))
    except Exception as e:
        raise argparse.ArgumentTypeError(f'Illegal range settings: {val}')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        prog='pwrplot',
        description='Plot bus voltage and current from file')
    
    parser.add_argument('file', help='Input file', type=str)
    parser.add_argument('reference', help='Reference power measurement', type=str, nargs='?')
    parser.add_argument('-s', '--sync', help='Synchronize file and reference on first power spike', action='store_true')
    parser.add_argument('-l', '--legend', help='Comma separated list of labels for file and reference', type=str)
    parser.add_argument('-o', '--out', help='Output file name (excluding file ending)', type=str)
    parser.add_argument('-fs', '--font-size', type=int, default=14, help='Font size for the plot text')
    parser.add_argument('-r', '--range', type=parse_range, help='Range to plot in milliseconds. Format: \'<start_ms>-<end_ms>\'')
    parser.add_argument('-z', '--z-threshold', type=float, help='Z Threshold for spike detection. If not given no spike detection is performed')

    args = parser.parse_args()
    process_files(args.file, args.reference, args.sync, args.legend, args.out, args.font_size, args.range, args.z_threshold)
