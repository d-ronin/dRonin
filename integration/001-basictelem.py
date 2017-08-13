#!/usr/bin/env python

from __future__ import print_function

# Insert the parent directory into the module import search path.
import os
import sys

sys.path.insert(1, os.path.dirname(sys.path[0]) + "/python")

from dronin import uavo, telemetry, uavo_collection
from threading import Condition
import time
import unittest

#-------------------------------------------------------------------------------
USAGE = "%(prog)s"
DESC  = """
  Retrieve the configuration of a flight controller.\
"""

cond = Condition()
pending = []

def completion_cb(obj):
    with cond:
        pending.remove(obj._id)
        cond.notifyAll()

def wait_till_less_than(num):
    with cond:
        while len(pending) > num:
            cond.wait()


class TestBasicTelemetry(unittest.TestCase):
    def test_010_start_telemetry(self):
        global tStream

        tStream = telemetry.SubprocessTelemetry("./build/sim/sim.elf -S telemetry:stdio",
                service_in_iter=False, iter_blocks=True)

        tStream.start_thread()
        
        global startTm
        
        startTm = time.time()

        tStream.wait_connection()

    def test_020_get_all_settings(self):
        settings_objects = tStream.uavo_defs.get_settings_objects()

        for s in settings_objects:
            pending.append(s._id)

            tStream.request_object(s, cb=completion_cb)

            wait_till_less_than(5)

        wait_till_less_than(0)

    def test_030_make_sure_all_settings(self):
        settings_objects = tStream.uavo_defs.get_settings_objects()

        missing = []
        for s in settings_objects:
            val = tStream.last_values.get(s)
            if val is None:
                missing.append(s._name)

        self.assertEqual(len(missing), 0)

    def test_040_wait_for_6s(self):
        remain = 6 - (time.time() - startTm)

        while remain > 0:
            time.sleep(remain + 0.001)
            remain = 4 - (time.time() - startTm)

    def test_045_check_system_state(self):
        typ = tStream.uavo_defs.find_by_name("SystemStats")
        systemStats = tStream.last_values[typ]

        self.assertGreater(systemStats.FlightTime, 4000)
        self.assertLess(systemStats.FlightTime, 13000)

        # Telemetered only on change
        typ = tStream.uavo_defs.find_by_name("FlightStatus")
        flightStatus = tStream.request_object(typ)

        self.assertEqual(flightStatus.Armed, flightStatus.ENUM_Armed['Disarmed'])
        self.assertEqual(flightStatus.ControlSource, flightStatus.ENUM_ControlSource['Failsafe'])

        typ = tStream.uavo_defs.find_by_name("StabilizationDesired")
        stabDesired = tStream.last_values[typ]

        self.assertEqual(stabDesired.Thrust, -1)
        self.assertEqual(stabDesired.StabilizationMode[0], stabDesired.ENUM_StabilizationMode['Disabled'])
        self.assertEqual(stabDesired.StabilizationMode[1], stabDesired.ENUM_StabilizationMode['Disabled'])
        self.assertEqual(stabDesired.StabilizationMode[2], stabDesired.ENUM_StabilizationMode['Disabled'])

        typ = tStream.uavo_defs.find_by_name("ActuatorDesired")
        actDesired = tStream.last_values[typ]

        self.assertEqual(actDesired.Roll, 0)
        self.assertEqual(actDesired.Pitch, 0)
        self.assertEqual(actDesired.Yaw, 0)
        self.assertEqual(actDesired.Thrust, -1)

    def test_050_check_expected_objs(self):
        # These depend upon the configured telemetry rates in the UAVOs
        typ = tStream.uavo_defs.find_by_name("Gyros")
        gyros = tStream.as_numpy_array(typ, blocks = False)

        # Expect 6, but leave wiggle room
        self.assertGreaterEqual(len(gyros), 4)

        typ = tStream.uavo_defs.find_by_name("Accels")
        accels = tStream.as_numpy_array(typ, blocks = False)

        # Expect 6, but leave wiggle room
        self.assertGreaterEqual(len(accels), 4)

        typ = tStream.uavo_defs.find_by_name("AttitudeActual")
        atti = tStream.as_numpy_array(typ, blocks = False)

        # Expect 60, but leave wiggle room
        self.assertGreaterEqual(len(atti), 54)

#-------------------------------------------------------------------------------

if __name__ == "__main__":
    unittest.main()
