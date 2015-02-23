"""
 1. What's the performance spread between the best and the worst set of
parameters? 

 2. In each of the above cases, when you're wrong, which one do you pick?  Do
 you pick one that performs close to the best? B A good property of thee
 predictor would be that it tends to pick the next best one or one closeose to
 the best performing set of parameters when it gets things wrong. B I don't
 know exactly how you'd engineer that with a plain old Classifier but it's
 something to think about.
"""
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

from sciload import *

import sys

def autolabel(bars,std):
  for ii,bar in enumerate(bars):
    rect = bars[ii]
    height = rect.get_height()
    plt.text(rect.get_x()+(rect.get_width()/2), maxtimes[ii]+0.5, '%.2f'% (std[ii]), ha='center', va='bottom', size='xx-small')

data_file = sys.argv[1]


dtype={'names': ('problem size', 'outer', 'inner', 'loop type', 'set type', 'loop', 'range size', 'segments', 'function size', 'time'),
    'formats': ('i4', 'S64', 'S64', 'S64', 'S64', 'S1024', 'i4', 'i4', 'i4', np.float64)}

#data = loadcsvdata(data_file, dtype)
data = loadcsvdata(data_file)
#data = loadcsvdata(data_file, include_size=False)

data['loop'] = [x.split('/')[-1] for x in data['loop']]

loops = np.unique(data['loop'])
meantimes = np.empty(len(loops))
mintimes = np.empty(len(loops))
maxtimes = np.empty(len(loops))
stddev = np.empty(len(loops))
times = []


print data['loop']

for i,l in enumerate(loops):
  loop = loops[i]
  ldata = data[data['loop'] == loop]
  sdata = ldata[ldata['range size'] == ldata['range size'].max()]
  meantimes[i] = np.mean(sdata['time'])
  stddev[i] = np.std(sdata['time'])
  mintimes[i] = sdata['time'].min()
  maxtimes[i] = sdata['time'].max()
  times.append(sdata['time'])

maxerror = maxtimes - meantimes
minerror = meantimes - mintimes

sorted_by_stddev = [time for var,time in sorted(zip(stddev,times), reverse=True)]
loops_by_stddev = [loop for var,loop in sorted(zip(stddev,loops), reverse=True)]
print loops_by_stddev

loopinds = np.array(range(len(loops)))
#loopinds = np.arange(0,50*len(times),50)


#size = [5, 10, 15, 20, 25, 30, 50, 75, 100]
size = [1,2,3]
#
best_data = add_labels(data)

dtype={'names': ('problem size', 'outer', 'inner', 'loop type', 'set type', 'loop', 'range size', 'segments', 'function size', 'time', 'best'),
        'formats': ('i4', 'S64', 'S64', 'S64', 'S64', 'S1024', 'i4', 'i4', 'i4', 'f8', 'bool')}

best_data = get_x(data, dtype=None)

#
#for s in size:
#    default_time = 0.0
#    best_time = 0.0
#
#    #stimes = best_data[best_data['problem size'] == s]
#    loopss = np.unique(data['loop'])
#
#    for l in loopss:
#        for r in np.unique(stimes[stimes['loop'] == l]['range size']):
#            default = stimes[np.logical_and(stimes['loop'] == l, np.logical_and(stimes['outer'] == 'SEGIT_SEQ', np.logical_and(stimes['inner'] == 'SEG_SEQ', stimes['range size'] == r)))]
#            best = stimes[np.logical_and(stimes['loop'] == l, np.logical_and(stimes['best'] == True, stimes['range size'] == r))]
#
#
#            if len(default) != len(best):
#                continue
#
#            default_time = default_time + sum(default['time'])
##            print '=============================================================================='
##            print default['time']
##            print best['time']
#            if len(default['time']) != len(best['time']):
#                print len(default['time'])
#            #print '=============================================================================='
#
#            #print stimes[np.logical_and(stimes['loop'] == l, np.logical_and(stimes['best'] == True, stimes['range size'] == r))]['inner']
#            #print stimes[np.logical_and(stimes['loop'] == l, stimes['best'] == 1)]
#            best_time = best_time + sum(best['time'])
#            #sum(stimes[np.logical_and(stimes['loop'] == l,
#               # np.logical_and(stimes['best'] == True, stimes['range size'] == r))]['time'][0])
#            #print stimes[np.logical_and(stimes['loop'] == l, stimes['best'] == 1)]['outer'], stimes[np.logical_and(stimes['loop'] == l, stimes['best'] == 1)]['inner']
#
#    print s, default_time, " -> ", best_time, " (", 100.0-100.0*(best_time/default_time), ")"

w = 50
#bars = plt.bar(loopinds, mintimes, w, color='b', alpha=0.4)
#bars = plt.bar(loopinds, meantimes-mintimes, w, bottom=mintimes, color='r', alpha=0.4)
#bars = plt.bar(loopinds, maxtimes-(meantimes+mintimes), w, bottom=(meantimes), color='g', alpha=0.4)
#bars = plt.bar(loopinds, meantimes, color='b', alpha=0.4, yerr=[minerror, maxerror])
#autolabel(bars,stddev)
#plt.xticks(loopinds+50+(w/2.0), short_loops, rotation='vertical')
#plt.xticks(loopinds+1,short_loops,rotation='vertical')
#plt.xlabel('Loop')
#plt.ylabel('Runtime ')
#plt.title('Minimum, mean and maximum runtimes for each loop')
#fig = plt.gcf()
#fig.set_size_inches(48, 12)

print "LOOP DATA"
for p in sorted_by_stddev[:10]:
    print "===="
    for i in p:
        print i
print "===="
#plt.boxplot(sorted_by_stddev[:10])
#plt.xticks([],[])
#plt.axes().spines['top'].set_visible(False)
#plt.axes().spines['right'].set_visible(False)
#plt.axes().spines['bottom'].set_visible(False)
#plt.axes().xaxis.set_ticks_position('none')
#plt.axes().yaxis.set_ticks_position('left')
#
#plt.savefig('box-plot.pdf',bbox_inches='tight',transparent=True)
