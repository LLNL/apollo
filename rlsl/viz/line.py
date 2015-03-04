import matplotlib.pyplot as plt
import numpy as np

from style import LINES, tuftestyle


def plot_lines(xlabel, ylabel, legend, output_filename, xlabels, *args):
    """
    This method plots a line graph using pyplot from the data given in from
    arrays of y-values given *args.

    
    """
    fig, ax = plt.subplots(figsize=(8,4))

    N = len(xlabels)
    x_locations = np.arange(N)

    for data in args:
        ax.plot(x_locations, data, next(LINES))

    plt.xlim([-0.5,x_locations.size-0.5])

    ax.legend(legend, loc='lower right')

    plt.xlim([-0.5,x_locations.size-0.5])
    
    ax.set_yscale('log')

    ax.set_ylabel(ylabel)
    ax.set_xlabel(xlabel)
    ax.set_xticks(x_locations)
    ax.set_xticklabels(xlabels)

    tuftestyle(ax)

    fig.tight_layout()
    plt.savefig(output_filename)


def plot_sub_axis(xlabel, ylabel, xlocations, xlabels,
        x, y, width, height, *args):
    """
    Add an inset axis to a plot, and plot a line graph using the data given as
    y-value arrays in args.

    x_locations is the array of x-values.
    xlabels are the ticks to use on the x-axis.
    x, y specify the position of the axis
    width, height specify the size of the axis.

    N.B. this method uses the rlsl.viz.style.LINES generator to cycle through
    line styles. To match the legend the main plot, make sure this generator is
    on the right value.
    """
    a = plt.axes([x, y, width, height])

    for data in args:
        a.plot(x_locations, data, next(LINES))

    a.set_xticks(x_locations)
    a.set_xticklabels(xlabels)
    a.set_ylabel(ylabel, fontsize=12)
    a.set_xlabel(xlabel, fontsize=12)
