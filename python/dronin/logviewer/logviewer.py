#!/usr/bin/env python

import pyqtgraph as pg
from pyqtgraph.Qt import QtCore, QtGui
import numpy as np

from dronin.logviewer.plotdockarea import PlotDockArea

from pyqtgraph.dockarea import *

def get_data_series(obj_name, fields):
    data = get_series(obj_name)

    base_fields = [ f.split(':')[0] for f in fields ]
    peeled = data[base_fields]

    if ':' not in ''.join(fields):
        try:
            return peeled.view(dtype='float').reshape(-1, 2)
        except Exception:
            pass

    rows = peeled.shape[0]
    cols = len(fields)
    outp = np.empty((peeled.shape[0], len(fields)))

    for j in range(cols):
        field_info = fields[j].split(':')

        if len(field_info) > 1:
            subs = int(field_info[1])
            for i in range(rows):
                val = peeled[i][j][subs]

                outp[i,j] = float(val)
        else:
            for i in range(rows):
                val = peeled[i][j]

                outp[i,j] = float(val)

    return outp

def add_plot_area(data_series, dock_name, axis_label, legend=False, **kwargs):
    dock = Dock(dock_name, size=(800, 300))

    pw = pg.PlotWidget()

    if legend:
        pw.addLegend()

    colors = [ 'w', 'm', 'y', 'c' ]
    idx = 0

    for plot_name, data in data_series.items():
        pw.plot(data, antialias=True, name='&nbsp;'+plot_name, pen=pg.mkPen(colors[idx]), **kwargs)
        idx += 1

    # pen=None, symbol='o', symbolSize=2.5

    pw.setLabel('left', axis_label)

    pw.setMenuEnabled(menus_enabled)

    dock.addWidget(pw)

    global last_plot

    if (last_plot is not None) and (last_plot.isVisible()):
        area.addDock(dock, 'bottom', last_plot)
    else:
        area.addDock(dock, 'left', dui)

    last_plot = dock

    return pw

def plot_vs_time(obj_name, fields):
    if not isinstance(fields, list):
        fields = [fields]

    if len(fields) > 1:
        left_axis_label = '%s<br>&nbsp;<br>&nbsp;' % (obj_name)
        legend = True
    else:
        left_axis_label = '%s<br>%s<br>%s' % (obj_name, fields[0], objtyps[obj_name]._units[fields[0].split(':')[0]])
        legend = False

    data_series = {}
    for f in fields:
        data_series[obj_name + '.' + f] = get_data_series(obj_name, ['time', f])

    global win_num

    win_num += 1
    dock_name = "TimeSeries%d" % (win_num)

    return add_plot_area(data_series, dock_name, left_axis_label, legend=legend)

def clear_plots(skip=None):
    containers, docks = area.findAll()

    for d in list(docks.values()):
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

def scan_for_events(uv):
    flight_mode = -1
    armed = -1

    events = []

    for u in uv:
        if u._name == 'UAVO_FlightStatus':
            ev = []
            # u.Armed DISARMED/ARMING/ARMED
            # u.FlightMode

            if u.Armed != armed:
                armed = u.Armed

                ev.append(u.ENUMR_Armed[armed])

            if u.FlightMode != flight_mode:
                flight_mode = u.FlightMode

                ev.append('MODE:' + u.ENUMR_FlightMode[flight_mode])

            if len(ev):
                tup = (u.time, '/'.join(ev))
                events.append(tup)

    return events

def handle_open(ignored=False, fname=None):
    from dronin import telemetry, uavo

    if fname is None:
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
            print("Couldn't stat file")
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

        for typ in t.uavo_defs.values():
            short_name = typ._name[5:]
            objtyps[short_name] = typ

        event_series = scan_for_events(t)

        global last_plot
        last_plot = None

        thrust_plot = plot_vs_time('StabilizationDesired', 'Thrust')

        clear_plots(skip=[last_plot])

        for tm,text in event_series:
            thrust_plot.addLine(x=tm)
            # label=text, labelOpts={'rotateAxis' : (1,0)} )

        dlg.setValue(925)
        plot_vs_time('AttitudeActual', ['Yaw', 'Roll', 'Pitch'])
        dlg.setValue(975)
        plot_vs_time('Gyros', ['x', 'y', 'z'])
        plot_vs_time('ActuatorCommand', ['Channel:0', 'Channel:1', 'Channel:2', 'Channel:3'])

        objtyps = { k:v for k,v in objtyps.items() if v in t.last_values }

        #add all non-settings objects, and autotune, to the keys.
        objSel.clear()

        for typnam in sorted(objtyps.keys()):
            if typnam == "SystemIdent" or not objtyps[typnam]._is_settings:
                objSel.addItem(typnam)

        objSel.setEnabled(True)

def updateItems(i):
    uavo = objtyps[str(objSel.currentText())]

    itemSel.clear()

    for i in range(0, len(uavo._num_subelems)):
        field_name = uavo._fields[3+i]
        num_subs = uavo._num_subelems[i]

        if num_subs == 1:
            itemSel.addItem(field_name)
        else:
            for j in range(0, num_subs):
                itemSel.addItem('%s:%d'%(field_name, j))

    itemSel.setEnabled(True)
    addPlotBtn.setEnabled(True)

def addTimeSeries():
    plot_vs_time(str(objSel.currentText()), str(itemSel.currentText()))

def enableContextuals(enabled):
    global menus_enabled

    menus_enabled = enabled

    containers, docks = area.findAll()

    for dock in docks.values():
        for w in dock.widgets:
            if isinstance(w, pg.widgets.PlotWidget.PlotWidget):
                w.setMenuEnabled(enabled)

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
menus_enabled = False

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

dui.addWidget(QtGui.QLabel("Obj:"), 0, 0)

objSel = QtGui.QComboBox()
objSel.setEnabled(False)
dui.addWidget(objSel, 0, 1)

dui.addWidget(QtGui.QLabel("Item:"), 1, 0)

itemSel = QtGui.QComboBox()
itemSel.setEnabled(False)
addPlotBtn = QtGui.QPushButton("&Add time-series plot")
addPlotBtn.setEnabled(False)

contextualCb = QtGui.QCheckBox("Enable contextual menus")
contextualCb.setChecked(False)

dui.addWidget(itemSel, 1, 1)
dui.addWidget(addPlotBtn, 2, 0, 1, 2)
dui.layout.addItem(QtGui.QSpacerItem(20, 20, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding), 3, 0)
dui.layout.addWidget(contextualCb, 4, 0, 1, 2)
dui.layout.addItem(QtGui.QSpacerItem(20, 20, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding), 5, 0)

objSel.currentIndexChanged.connect(updateItems)
addPlotBtn.clicked.connect(addTimeSeries)
contextualCb.toggled.connect(enableContextuals)

win.show()

def main():
    import sys

    if len(sys.argv) == 2:
        handle_open(fname=sys.argv[1])

    if (sys.flags.interactive != 1) or not hasattr(QtCore, 'PYQT_VERSION'):
        QtGui.QApplication.instance().exec_()
