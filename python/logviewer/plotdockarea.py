from pyqtgraph.dockarea import DockArea, Dock
import pyqtgraph as pg
from pyqtgraph.dockarea.Container import VContainer, TContainer

# This code is so we can link plots automatically when time series 
# are above each other...

class PlotDockArea(DockArea):
    def addDock(self, *args, **kwds):
        DockArea.addDock(self, *args, **kwds)
        self.linkPlots()

    def floatDock(self, *args, **kwds):
        ret = DockArea.floatDock(self, *args, **kwds)
        self.linkPlots()

    def addTempArea(self):
        # Hook addDock to make it all work
        ret = DockArea.addTempArea(self)

        def addDockShim(*args, **kwds):
            DockArea.addDock(ret, *args, **kwds)

            self.linkPlots()

        ret.addDock = addDockShim

        return ret

    ## PySide bug: We need to explicitly redefine these methods
    ## or else drag/drop events will not be delivered.
    def dragEnterEvent(self, *args):
        DockArea.dragEnterEvent(self, *args)

    def dragMoveEvent(self, *args):
        DockArea.dragMoveEvent(self, *args)

    def dragLeaveEvent(self, *args):
        DockArea.dragLeaveEvent(self, *args)

    def dropEvent(self, *args):
        DockArea.dropEvent(self, *args) 

    def linkPlotsRecurse(self, c, x_link=None):
        for i in range(c.count()):
            dock = c.widget(i)

            if isinstance(dock, TContainer):
                x_link = self.linkPlotsRecurse(dock, x_link)

            if not isinstance(dock, Dock):
                continue

            if not dock.name().startswith('TimeSeries'):
                continue

            for w in dock.widgets:
                if isinstance(w, pg.widgets.PlotWidget.PlotWidget):
                    w.setXLink(x_link)

                    x_link = w

        return x_link

    def linkPlots(self):
        # Walk the containers for all vertical containers, find graphs, link
        # ones that are on top of each other in a single container.

        containers, docks = self.findAll()

        for c in containers:
            if isinstance(c, VContainer):
                self.linkPlotsRecurse(c)
            
