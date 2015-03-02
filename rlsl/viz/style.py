import matplotlib
from itertools import cycle

matplotlib.rcParams['font.sans-serif'] = 'Gill Sans Mt'
matplotlib.rcParams['font.size'] = '10'

__lines = ["o-","*--","s-.","^:"]
#__hatch = ["", "/", "\\", "//", "\\\\", "-", "+"]
__hatch = ['/', '\\', '|', '-', '+', 'x', 'o', 'O', '.', '*']
__color = ['r', 'g', 'b', 'y']

LINES = cycle(__lines)
HATCH = cycle(__hatch)
COLOR = cycle(__color)


def tuftestyle(ax):
    """
    Styles an axes object to have minimalist graph features.
    """

#    ax.grid(True, 'major', color='w', linestyle='-', linewidth=1.5,alpha=0.6)
#    ax.patch.set_facecolor('white')

    ax.spines["right"].set_visible(False)
    ax.spines["top"].set_visible(False)
    ax.spines["bottom"].set_visible(True)
    ax.spines["left"].set_visible(True)

#    #restyle the tick lines
#    for line in ax.get_xticklines() + ax.get_yticklines():
#        line.set_markersize(4)
#        line.set_color("black")
#        line.set_markeredgewidth(1)
#
#    for line in ax.xaxis.get_ticklines(minor=True) + ax.yaxis.get_ticklines(minor=True):
#        line.set_markersize(0)
#
#    matplotlib.rcParams['xtick.direction'] = 'out'
#    matplotlib.rcParams['ytick.direction'] = 'out'
#    ax.xaxis.set_ticks_position('bottom')
#    ax.yaxis.set_ticks_position('left')
