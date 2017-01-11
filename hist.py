import sys
import pyqtgraph
from pyqtgraph import plot


def hist(file):
    print "parsing", file
    with open(file, 'r') as f:
        data = f.read()
    hist = {}
    t = 0
    maxt = 0
    frames = 0
    total = 0
    lanes = [0]*7
    o = 0
    for b in data:
        b, o = ord(b) & ~o, ord(b)
        if b & 0x80:
            t = 0
            frames += 1
        l = hist.setdefault(t, [0]*7)
        for idx, m in enumerate([1, 2, 4, 8, 16, 32, 64]):
            if b & m:
                l[idx] += 1
                total +=1
                lanes[idx] +=1
        t += 1
        if t>maxt: maxt=t
    print "%d frames (longest=%d) found"%(frames, maxt)
    print "total events:", total
    print "avg events/frame", total/float(frames)
    print "events/frame/lane", lanes 
    print "avg events/frame/lane", total/float(frames*7)

    # make 8 lists, 1 per lane and one summed up
    hists = [[],[],[],[],[],[],[],[]]
    for t in range(maxt):
        summed = 0
        entry = hist[t]
        for i in range(7):
            counts = entry[i]
            hists[i].append(counts)
            summed += counts
        hists[7].append(summed)
    return frames, maxt, total, hists


if len(sys.argv) == 1:
    print "help"
elif len(sys.argv) == 2:
    frames, maxt, total, hists = hist(sys.argv[1])
    x = range(len(hists[0]))
    p = plot(x, hists[0], symbol='o')
    p.plot(x, hists[1], symbol='x')
    p.plot(x, hists[2])
    p.plot(x, hists[3])
    p.plot(x, hists[4])
    p.plot(x, hists[5])
    p.plot(x, hists[6])
    pyqtgraph.QtGui.QApplication.exec_()

else:
    for fn in sys.argv[1:]:
        frames, maxt, total, hists = hist(fn)
        print
        x = range(maxt)
        p = plot(x, hists[7])
        p.setYRange(0, max(hists[7]))
        p.topLevelWidget().setWindowTitle(fn+': %d frames, %.1f evts/frame' %(frames, total/float(frames)))
    pyqtgraph.QtGui.QApplication.exec_()
