#!/usr/bin/env python

import pyqtgraph as pg
from pyqtgraph.Qt import QtCore, QtGui
import numpy as np

from logviewer.plotdockarea import PlotDockArea

from pyqtgraph.dockarea import *

# Create our application, consisting of dockarea stuff.
# For now fix the size at 1024x600, so that we can make it work well in this
# size
app = QtGui.QApplication([])
win = QtGui.QMainWindow()
area = PlotDockArea()
win.setCentralWidget(area)
win.resize(1024, 600)

win.statusBar().showMessage('Load a file to begin analysis...')
menubar = win.menuBar()

quitAction = QtGui.QAction('&Quit', win)
quitAction.triggered.connect(app.quit)

def add_plot_area(obj_name, fields):
    dock = Dock("TimeSeries", size=(800,300))

    pw = pg.PlotWidget()

    data = get_series(obj_name)

    view = data[fields].view(dtype='float').reshape(-1, 2)

#    pw.addLegend(offset=(-20, -20))

    plot_name = obj_name + '.' + fields[1]

    pw.plot(view, antialias=True, name=plot_name)
    #pw.plot(view, antialias=False, pen=None, symbol='o', name=plot_name,
    #        symbolSize=2.5)

    left_axis_label = '%s<br>%s<br>%s' % (obj_name, fields[1], objtyps[obj_name]._units[fields[1]])
    pw.setLabel('left', left_axis_label)

    pw.setMenuEnabled(False)

    dock.addWidget(pw)

    global dl
    area.addDock(dock, 'bottom', dl) 

def get_series(name):
    global series

    if name in series:
        return series[name]

    typ = t.uavo_defs.find_by_name(name)

    series[name] = t.as_numpy_array(typ)

    return series[name]

def handle_open():
    from dronin import telemetry, uavo

    with pg.ProgressDialog("Loading objects...", wait=1, cancelText=None) as dlg:
        global t

        f = open('test.drlog', 'rb')

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
            perc = (n_bytes * 90.0) / num_bytes
            
            if (perc > 90.0): perc = 90.0
            dlg.setValue(perc)

        t = telemetry.FileTelemetry(f, parse_header=True, service_in_iter=True,
                    gcs_timestamps=False, name='test.drlog', progress_callback=cb)

        global series, objtyps
        series = {}
        objtyps = {}

        for typ in t.uavo_defs.itervalues():
            short_name = typ._name[5:]
            objtyps[short_name] = typ

        add_plot_area('AttitudeActual', ['time', 'Yaw'])
        add_plot_area('AttitudeActual', ['time', 'Roll'])
        add_plot_area('AttitudeActual', ['time', 'Pitch'])
        add_plot_area('Gyros', ['time', 'z'])
        add_plot_area('Accels', ['time', 'z'])


openAction = QtGui.QAction("&Open", win)
openAction.triggered.connect(handle_open)
fileMenu = menubar.addMenu('&File')


fileMenu.addAction(openAction)
fileMenu.addAction(quitAction)

## Create docks, place them into the window.
dl = Dock("Waiting...", size=(800, 1))

#dg1 = Dock("TimeSeries", size=(800, 300))

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
saveBtn = QtGui.QPushButton('Save dock state')
restoreBtn = QtGui.QPushButton('Restore dock state')
restoreBtn.setEnabled(False)
w1.addWidget(label, row=0, col=0)
w1.addWidget(saveBtn, row=1, col=0)
w1.addWidget(restoreBtn, row=2, col=0)
dui.addWidget(w1)

state = None
def save():
    global state
    state = area.saveState()
    restoreBtn.setEnabled(True)
def load():
    global state
    area.restoreState(state)
saveBtn.clicked.connect(save)
restoreBtn.clicked.connect(load)
#w5 = pg.ImageView()
#w5.setImage(np.random.normal(size=(100,100)))
#d5.addWidget(w5)

#w6 = pg.PlotWidget(title="Dock 6 plot")
#w6.plot(np.random.normal(size=100))
#d6.addWidget(w6)

win.show()

## Start Qt event loop unless running in interactive mode or using pyside.
if __name__ == '__main__':
    import sys
    if (sys.flags.interactive != 1) or not hasattr(QtCore, 'PYQT_VERSION'):
        QtGui.QApplication.instance().exec_()
