#!/usr/bin/env python

import pyqtgraph as pg
from pyqtgraph.Qt import QtCore, QtGui
import numpy as np

from logviewer.plotdockarea import PlotDockArea

from pyqtgraph.dockarea import *

# Create our application, consisting of dockarea stuff.
# For now fix the size at 1024x680, so that we can make it work well in this
# size
app = QtGui.QApplication([])
win = QtGui.QMainWindow()
area = PlotDockArea()
win.setCentralWidget(area)
win.resize(1024, 680)

menubar = win.menuBar()

win_num = 0

def get_data_series(obj_name, fields):
    data = get_series(obj_name)

    return data[fields].view(dtype='float').reshape(-1, 2)

def add_plot_area(data_series, dock_name, axis_label, legend=False, **kwargs):
    dock = Dock(dock_name, size=(800, 300))

    pw = pg.PlotWidget()

    if legend:
        pw.addLegend()

    colors = [ 'w', 'm', 'y', 'c' ]
    idx = 0

    for plot_name, data in data_series.iteritems():
        pw.plot(data, antialias=True, name='&nbsp;'+plot_name, pen=pg.mkPen(colors[idx]), **kwargs)
        idx += 1

    # pen=None, symbol='o', symbolSize=2.5

    pw.setLabel('left', axis_label)

    pw.setMenuEnabled(False)

    dock.addWidget(pw)

    global last_plot

    if last_plot != None:
        area.addDock(dock, 'bottom', last_plot)
    else:
        area.addDock(dock, 'left', dui)

    last_plot = dock

def plot_vs_time(obj_name, fields):
    if not isinstance(fields, list):
        fields = [fields]

    if len(fields) > 1:
        left_axis_label = '%s<br>&nbsp;<br>&nbsp;' % (obj_name)
        legend = True
    else:
        left_axis_label = '%s<br>%s<br>%s' % (obj_name, fields[0], objtyps[obj_name]._units[fields[0]])
        legend = False

    data_series = {}
    for f in fields:
        data_series[obj_name + '.' + f] = get_data_series(obj_name, ['time', f])

    global win_num

    win_num += 1
    dock_name = "TimeSeries%d" % (win_num)

    add_plot_area(data_series, dock_name, left_axis_label, legend=legend)

def clear_plots(skip=None):
    containers, docks = area.findAll()

    for d in docks.values():
        if skip is not None and d in skip:
            continue

        if (not d.name().startswith("TimeSeries")) and (d.name() != "Waiting..."):
            continue

        d.close()

def get_series(name):
    global series

    if name in series:
        return series[name]

    typ = t.uavo_defs.find_by_name(name)

    series[name] = t.as_numpy_array(typ)

    return series[name]

def handle_open():
    from dronin import telemetry, uavo

    fname = QtGui.QFileDialog.getOpenFileName(win, 'Open file', filter="Log files (*.drlog *.txt)")
    with pg.ProgressDialog("0 objects read...", wait=500, maximum=1000, cancelText=None) as dlg:
        global t

        f = open(fname, 'rb')
        num_bytes = 1000000000

        try:
            import os

            stat_info = os.fstat(f.fileno())

            num_bytes = stat_info.st_size
        except Exception:
            print "Couldn't stat file"
            pass

        def cb(n_objs, n_bytes):
            # Top out at 90%, so the dialog doesn't hang at 100%
            # during the non-reading operations...
            permille = (n_bytes * 900.0) / num_bytes

            # should not happen, but cover the case anyways
            if (permille > 900.0): permille = 900.0
            dlg.setValue(permille)
            dlg.setLabelText("%d objects read..." % n_objs)

        t = telemetry.FileTelemetry(f, parse_header=True, service_in_iter=True,
                    gcs_timestamps=None, name=fname, progress_callback=cb)

        global series, objtyps
        series = {}
        objtyps = {}

        for typ in t.uavo_defs.itervalues():
            short_name = typ._name[5:]
            objtyps[short_name] = typ

        global last_plot
        last_plot = None

        plot_vs_time('ManualControlCommand', 'Throttle')

        clear_plots(skip=[last_plot])

        dlg.setValue(925)
        plot_vs_time('AttitudeActual', ['Yaw', 'Roll', 'Pitch'])
        dlg.setValue(950)
        plot_vs_time('Gyros', 'z')
        dlg.setValue(975)
        plot_vs_time('Accels', 'z')

openAction = QtGui.QAction("&Open", win)
openAction.setShortcut(QtGui.QKeySequence.Open)
openAction.triggered.connect(handle_open)

quitAction = QtGui.QAction('&Quit', win)
quitAction.setShortcut(QtGui.QKeySequence.Quit)
quitAction.triggered.connect(app.quit)

fileMenu = menubar.addMenu('&File')

fileMenu.addAction(openAction)
fileMenu.addAction(quitAction)

## Create docks, place them into the window.
dl = Dock("Waiting...", size=(800, 1))

dui = Dock("UI", size=(224, 300))

area.addDock(dui, 'right')     ## place d2 at right edge of dock area
area.addDock(dl, 'left') 

w1 = pg.LayoutWidget()
label = QtGui.QLabel("""The
Real
UI
Will
Go
Here
Soon.
""")
w1.addWidget(label, row=0, col=0)
dui.addWidget(w1)

win.show()


## Start Qt event loop unless running in interactive mode or using pyside.
if __name__ == '__main__':
    import sys
    if (sys.flags.interactive != 1) or not hasattr(QtCore, 'PYQT_VERSION'):
        QtGui.QApplication.instance().exec_()
