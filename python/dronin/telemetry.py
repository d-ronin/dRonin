"""
Interface to telemetry streams -- log, network, or serial.

Copyright (C) 2014-2015 Tau Labs, http://taulabs.org
Copyright (C) 2015-2016 dRonin, http://dronin.org
Licensed under the GNU LGPL version 2.1 or any later version (see COPYING.LESSER)
"""

import socket
import time
import errno
from threading import Condition

from . import uavtalk, uavo_collection, uavo

import os

from abc import ABCMeta, abstractmethod

from six import with_metaclass

class TelemetryBase(with_metaclass(ABCMeta)):
    """
    Basic (abstract) implementation of telemetry used by all stream types.
    """

    def __init__(self, githash=None, service_in_iter=True,
            iter_blocks=True, use_walltime=True, do_handshaking=False,
            gcs_timestamps=False, name=None, progress_callback=None):

        """Instantiates a telemetry instance.  Called only by derived classes.
         - githash: revision control id of the UAVO's used to communicate.
             if unspecified, we use the version in this source tree.
         - service_in_iter: whether the actual iterator should service the
             connection/read the file.  If you don't specify this, you probably
             want to call start_thread()
         - iter_blocks: whether the iterator should block for new data.  Only
             really meaningful for network/serial
         - use_walltime: if true, automatically place current time into packets
             received.
         - do_handshaking: if true, indicates this is an interactive stream
             where we should speak the UAVO_GCSTelemetryStats connection status
             protocol.
         - gcs_timestamps: if true, this means we are reading from a file with
             the GCS timestamp protocol.  if None, request autodetection
         - name: a filename to store into .filename for legacy purposes
         - progress_callback: a function to call periodically with progress
             information
        """

        uavo_defs = uavo_collection.UAVOCollection()

        if githash:
            uavo_defs.from_git_hash(githash)
        else:
            xml_path = os.path.join(os.path.dirname(__file__), "..", "..",
                                    "shared", "uavobjectdefinition")
            uavo_defs.from_uavo_xml_path(xml_path)

        self.githash = githash

        self.uavo_defs = uavo_defs
        self.uavtalk_generator = uavtalk.process_stream(uavo_defs,
            use_walltime=use_walltime, gcs_timestamps=gcs_timestamps,
            progress_callback=progress_callback,
            ack_callback=self.gotack_callback,
            nack_callback=self.gotnack_callback,
            reqack_callback=self.reqack_callback)

        self.uavtalk_generator.send(None)

        self.uavo_list = []

        self.last_values = {}

        self.cond = Condition()
        self.ack_cond = Condition()

        if (service_in_iter) and (not iter_blocks):
            raise ValueError("Invalid combination of flags")

        self.service_in_iter = service_in_iter
        self.iter_blocks = iter_blocks

        self.do_handshaking = do_handshaking
        self.filename = name

        self.acks = set()
        self.nacks = set()

        self.eof = False

        self.first_handshake_needed = self.do_handshaking

        if do_handshaking:
            self.GCSTelemetryStats = uavo_defs.find_by_name('UAVO_GCSTelemetryStats')
            self.FlightTelemetryStats = uavo_defs.find_by_name('UAVO_FlightTelemetryStats')

    def reqack_callback(self, obj):
        if self.do_handshaking:
            self._send(uavtalk.acknowledge_object(obj))

    def gotack_callback(self, obj):
        with self.ack_cond:
            self.acks.add(obj)
            self.ack_cond.notifyAll()

    def gotnack_callback(self, obj):
        with self.ack_cond:
            self.nacks.add(obj)
            self.ack_cond.notifyAll()

    def as_numpy_array(self, match_class, filter_cond=None):
        """ Transforms all received instances of a given object to a numpy array.

        match_class: the UAVO_* class you'd like to match.
        """

        import numpy as np

        # Find the subset of this list that is of the requested class
        filtered_list = [x for x in self if isinstance(x, match_class)]

        # Perform any additional requested filtering
        if filter_cond is not None:
            filtered_list = list(filter(filter_cond, filtered_list))

        # Check for an empty list
        if filtered_list == []:
            return np.array([])

        return np.array(filtered_list, dtype=match_class._dtype)

    def __iter__(self):
        """ Iterator service routine. """
        iterIdx = 0

        self.cond.acquire()

        while True:
            if iterIdx < len(self.uavo_list):
                obj = self.uavo_list[iterIdx]

                iterIdx += 1

                self.cond.release()

                yield obj

                self.cond.acquire()
            elif self.iter_blocks and not self._done():
                if self.service_in_iter:
                    self.cond.release()

                    self.service_connection()

                    self.cond.acquire()
                else:
                    # wait for another thread to fill it in
                    self.cond.wait()
            else:
                break

    def __make_handshake(self, handshake):
        return self.GCSTelemetryStats._make_to_send(
                Status=self.GCSTelemetryStats.ENUM_Status[handshake])

    def __remove_from_ack_set(self, obj):
        with self.ack_cond:
            self.acks.discard(obj)

    def send_object(self, send_obj, req_ack=False, *args, **kwargs):
        if not self.do_handshaking:
            raise ValueError("Can only send on handshaking/bidir sessions")

        if req_ack:
            self.__remove_from_ack_set(send_obj.__class__)

            for i in range(8):
                self._send(uavtalk.send_object(send_obj, req_ack=req_ack, *args, **kwargs))
                if self.__wait_ack(send_obj.__class__, 0.26):
                    return True

            return False

        else:
            self._send(uavtalk.send_object(send_obj, req_ack=req_ack, *args, **kwargs))
            return True

    def __handle_handshake(self, obj):
        if obj.name == "UAVO_FlightTelemetryStats":
            # Handle the telemetry handshaking

            if obj.Status == obj.ENUM_Status['Disconnected']:
                # Request handshake
                print("Disconnected")
                send_obj = self.__make_handshake('HandshakeReq')
            elif obj.Status == obj.ENUM_Status['HandshakeAck']:
                # Say connected
                print("Handshake ackd")
                send_obj = self.__make_handshake('Connected')
            elif obj.Status == obj.ENUM_Status['Connected']:
                print("Connected")
                send_obj = self.__make_handshake('Connected')

            self.send_object(send_obj)

    def request_object(self, obj):
        if not self.do_handshaking:
            raise ValueError("Can only request on handshaking/bidir sessions")

        self._send(uavtalk.request_object(obj))

    def __wait_ack(self, obj, timeout):
        expiry = time.time() + timeout

        # properly sleep, etc.
        with self.ack_cond:
            while True:
                if obj in self.acks:
                    return True

                if self.eof:
                    return False

                diff = expiry - time.time();

                if (diff <= 0):
                    return False

                self.ack_cond.wait(diff + 0.001)

    def get_nacks(self):
        with self.ack_cond:
            ret = self.nacks
            self.nacks = set()

            return ret

    def __handle_frames(self, frames):
        objs = []

        if frames == b'':
            self.eof = True
            self._close()
        else:
            obj = self.uavtalk_generator.send(frames)

            while obj:
                if self.do_handshaking:
                    self.__handle_handshake(obj)

                objs.append(obj)

                obj = self.uavtalk_generator.send(b'')

        # Only traverse the lock when we've processed everything in this
        # batch.
        with self.cond:
            # keep everything in ram forever
            # for now-- in case we wanna see
            self.uavo_list.extend(objs)

            for obj in objs:
                self.last_values[obj.__class__]=obj

            self.cond.notifyAll()

    def get_last_values(self):
        """ Returns the last instance of each kind of object received. """
        with self.cond:
            return self.last_values.copy()

    def wait_connection(self):
        """ Waits for the connection handshaking to complete. """

        with self.cond:
            while True:
                fts = self.last_values.get(self.FlightTelemetryStats)

                if fts is not None:
                    if fts.Status == fts.ENUM_Status['Connected']:
                        return

                self.cond.wait()

    def start_thread(self):
        """ Starts a separate thread to service this telemetry connection. """
        if self.service_in_iter:
            # TODO sane exceptions here.
            raise

        if self._done():
            raise

        from threading import Thread

        def run():
            try:
                while not self._done():
                    self.service_connection()
            finally:
                with self.cond:
                    with self.ack_cond:
                        self.cond.notifyAll()
                        self.ack_cond.notifyAll()
                        self.eof = True
                        self._close()

        t = Thread(target=run, name="telemetry svc thread")

        t.daemon=True

        t.start()

    def service_connection(self, timeout=None):
        """
        Receive and parse data from a connection and handle the basic
        handshaking with the flight controller
        """

        if timeout is not None:
            finish_time = time.time() + timeout
        else:
            finish_time = None

        if self.first_handshake_needed:
            send_obj = self.__make_handshake('Disconnected')
            self.send_object(send_obj)

            self.first_handshake_needed = False
        data = self._receive(finish_time)
        self.__handle_frames(data)

    @abstractmethod
    def _receive(self, finish_time):
        return

    # No implementation required, so not abstract
    def _send(self, msg):
        return

    def _done(self):
        with self.cond:
            return self.eof

    def _close(self):
        return

class BidirTelemetry(TelemetryBase):
    """
    Stuff involved in buffering for bidirection telemetry.
    """
    def __init__(self, *args, **kwargs):
        """ Abstract instantiation of bidir telemetry

        Should only be called by derived classes.

        Meaningful parameters passed up to TelemetryBase include: githash,
        service_in_iter, iter_blocks, use_walltime
        """
        TelemetryBase.__init__(self, do_handshaking=True,
                gcs_timestamps=False,  *args, **kwargs)

        self.recv_buf = b''
        self.send_buf = b''

        self.send_lock = Condition()

    def _receive(self, finish_time):
        """ Fetch available data from file descriptor. """

        # Always do some minimal IO if possible
        self._do_io(0)

        while (len(self.recv_buf) < 1) and self._do_io(finish_time):
            pass

        if len(self.recv_buf) < 1:
            return None

        ret = self.recv_buf
        self.recv_buf = b''

        return ret

    def _send(self, msg):
        """ Send a string to the controller """

        with self.send_lock:
            self.send_buf += msg

        if self.service_in_iter:
            self._do_io(0)

    @abstractmethod
    def _do_io(self, finish_time):
        return


class FDTelemetry(BidirTelemetry):
    """
    Implementation of bidirectional telemetry from a file descriptor.

    Intended for network streams.
    """

    def __init__(self, fd, *args, **kwargs):
        """ Instantiates a telemetry instance on a given fd.

        Probably should only be called by derived classes.

         - fd: the file descriptor to perform telemetry operations upon

        Meaningful parameters passed up to TelemetryBase include: githash,
        service_in_iter, iter_blocks, use_walltime
        """

        BidirTelemetry.__init__(self, *args, **kwargs)

        self.fd = fd

    # Call select and do one set of IO operations.
    def _do_io(self, finish_time):
        import select

        rdSet = []
        wrSet = []

        did_stuff = False

        if len(self.recv_buf) < 1024:
            rdSet.append(self.fd)

        if len(self.send_buf) > 0:
            wrSet.append(self.fd)

        now = time.time()
        if finish_time is None:
            r,w,e = select.select(rdSet, wrSet, [])
        else:
            tm = finish_time-now
            if tm < 0: tm=0

            r,w,e = select.select(rdSet, wrSet, [], tm)

        if r:
            # Shouldn't throw an exception-- they just told us
            # it was ready for read.
            # TODO: Figure out why read sometimes fails when using sockets
            try:
                chunk = os.read(self.fd, 1024)
                if chunk == '':
                    raise RuntimeError("stream closed")

                self.recv_buf = self.recv_buf + chunk

                did_stuff = True
            except OSError as err:
                # For some reason, we sometimes get a
                # "Resource temporarily unavailable" error
                if err.errno != errno.EAGAIN:
                    raise

        if w:
            with self.send_lock:
                written = os.write(self.fd, self.send_buf)

                if written > 0:
                    self.send_buf = self.send_buf[written:]

                did_stuff = True

        return did_stuff
    
class NetworkTelemetry(FDTelemetry):
    """ TCP telemetry interface. """
    def __init__(self, host="127.0.0.1", port=9000, *args, **kwargs):
        """ Creates a telemetry instance talking over TCP.

         - host: hostname to connect to (default localhost)
         - port: port number to communicate on (default 9000)

        Meaningful parameters passed up to TelemetryBase include: githash,
        service_in_iter, iter_blocks, use_walltime
        """

        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((host, port))

        s.setblocking(0)

        self.sock = s

        FDTelemetry.__init__(self, fd=s.fileno(), *args, **kwargs)

    def _close(self):
        self.sock.close()

# TODO XXX : Plumb appropriate cleanup / file close for these classes

class SerialTelemetry(BidirTelemetry):
    """ Serial telemetry interface """
    def __init__(self, port, speed=115200, *args, **kwargs):
        """ Creates telemetry instance talking over (real or virtual) serial port.

         - port: Serial port path
         - speed: Baud rate (doesn't really matter for VCP, defaults 115200)

        Meaningful parameters passed up to TelemetryBase include: githash,
        service_in_iter, iter_blocks, use_walltime

        Requires the module pyserial to provide OS independance.
        """

        import serial

        self.ser = serial.Serial(port, speed, timeout=0, writeTimeout=0.001)

        BidirTelemetry.__init__(self, *args, **kwargs)

    # Call select and do one set of IO operations.
    def _do_io(self, finish_time):
        import serial

        did_stuff = False

        while not did_stuff:
            try:
                chunk = self.ser.read(1024)

                if chunk != '':
                    did_stuff = True
                    self.recv_buf = self.recv_buf + chunk
            except serial.serialutil.SerialException:
                # Ignore this; looks like a pyserial bug
                pass

            with self.send_lock:
                if self.send_buf != '':
                    try:
                        written = self.ser.write(self.send_buf)

                        if written > 0:
                            self.send_buf = self.send_buf[written:]
                            did_stuff = True
                    except serial.SerialTimeoutException:
                        pass

            now = time.time()
            if finish_time is not None:
                if now > finish_time:
                    break

            time.sleep(0.0025)    # 2.5ms, ~28 bytes of time at 115200

        return did_stuff

class HIDTelemetry(BidirTelemetry):
    """ HID telemetry interface """
    def __init__(self, vidpid=None, vid=None, pid=None, speed=115200, *args, **kwargs):
        """ Creates telemetry instance talking over USB HID.  Likely to only
        work on linux for now.

         - vidpid: a string, of two hex numbers separated by colon,
         like 39ee:4a3e, that in turn specifies the vendor id and product id
         to talk to.

        Meaningful parameters passed up to TelemetryBase include: githash,
        service_in_iter, iter_blocks, use_walltime

        Requires the module pyusb
        """

        if vid is None:
            try:
                raw = str.split(vidpid, ':')

                if len(raw) > 2:
                    raise

                vid = int(raw[0], base=16)

                if len(raw) == 2:
                    if raw[1] != '':
                        pid = int(raw[1], base=16)

            except Exception:
                raise ValueError("Invalid value of vidpid")

        import usb.core
        import usb.util

        if pid is None:
            dev = usb.core.find(idVendor = vid)
        else:
            dev = usb.core.find(idVendor = vid, idProduct = pid)

        if dev is None:
            raise RuntimeError("No fc found")

        # XXX do better things here
        if dev.is_kernel_driver_active(0):
            dev.detach_kernel_driver(0)
        if dev.is_kernel_driver_active(1):
            dev.detach_kernel_driver(1)
        if dev.is_kernel_driver_active(2):
            dev.detach_kernel_driver(2)

        #dev.set_configuration()

        cfg = dev.get_active_configuration()

        self.ep_out = None

        self.ep_in = None

        # Class 3 = HID
        for f in usb.util.find_descriptor(cfg, bInterfaceClass = 3):
            if f.bEndpointAddress < 0x80:
                self.ep_out = f
            else:
                self.ep_in = f

        if self.ep_out is None:
            raise RuntimeError("no output endpoint found")

        if self.ep_in is None:
            raise RuntimeError("no input endpoint found")

        BidirTelemetry.__init__(self, *args, **kwargs)

    # Call select and do one set of IO operations.
    def _do_io(self, finish_time):
        import errno
        from six import int2byte, indexbytes, byte2int, iterbytes

        did_stuff = False

        to_block = 10   # Wait no more than 10ms first time around

        while not did_stuff:
            chunk = ''

            try:
                raw = self.ep_in.read(64, timeout=to_block)

                if (raw[0] == 1):
                    length = raw[1]

                    if length <= len(raw) - 2:
                        chunk = raw.tostring()[2:2+length]
            except IOError as e:
                if e.errno != errno.ETIMEDOUT:
                    raise

            if chunk != '':
                did_stuff = True
                self.recv_buf = self.recv_buf + chunk

            with self.send_lock:
                if self.send_buf != '':
                    to_write = len(self.send_buf)

                    if to_write > 60:
                        to_write = 60

                    try:
                        buf = int2byte(1) + int2byte(to_write) + self.send_buf[0:to_write]

                        written = self.ep_out.write(buf) - 2

                        if written > 0:
                            self.send_buf = self.send_buf[written:]
                            did_stuff = True
                    except IOError as e:
                        if e.errno != errno.ETIMEDOUT:
                            raise

            now = time.time()
            if finish_time is not None:
                if now > finish_time:
                    break

                # if we have nothing left to send
                if self.send_buf == '':
                    remaining = finish_time - now

                    to_block = int((remaining * 0.75) * 1000) + 25
            else:
                if self.send_buf == '':
                    to_block = 2000
                else:
                    to_block = 10

            if not did_stuff:
                time.sleep(0.0025)    # 2.5ms, ~28 bytes of time at 115200

        return did_stuff

class FileTelemetry(TelemetryBase):
    """ Telemetry interface to data in a file """

    def __init__(self, file_obj, parse_header=False,
             *args, **kwargs):
        """ Instantiates a telemetry instance reading from a file.

         - file_obj: the file object to read from
         - parse_header: whether to read a header like the GCS writes from the
           file.

        Meaningful parameters passed up to TelemetryBase include: githash,
        service_in_iter, iter_blocks, gcs_timestamps
        """

        self.f = file_obj

        if parse_header:
            # Check the header signature
            #    First line is "dRonin git hash:" or "Tau Labs git hash:"
            #    Second line is the actual git hash
            #    Third line is the UAVO hash
            #    Fourth line is "##" (only from GCS)

            # Scan up to 100 "lines" looking for the signature, in case
            # there's garbage at the beginning of the log
            found = False

            for i in range(100):
                sig = self.f.readline()
                if sig.endswith(b'dRonin git hash:\n') or sig.endswith(b'Tau Labs git hash:\n'):
                    found = True
                    break;

            if not found:
                print("Source file does not have a recognized header signature")
                raise IOError("no header signature")

            # Determine the git hash that this log file is based on
            githash = self.f.readline()[:-1]
            if githash.find(b':') != -1:
                import re
                githash = re.search(b':(\w*)\W', githash).group(1)

            # For python3, convert from byte string.
            githash = githash.decode('latin-1')

            print("Log file is based on git hash: %s" % githash)

            uavohash = self.f.readline()
            # divider only occurs on GCS-type streams.  This causes us to
            # miss first objects in telemetry-type streams
            # divider = self.f.readline()

            TelemetryBase.__init__(self, iter_blocks=True,
                do_handshaking=False, githash=githash, use_walltime=False,
                *args, **kwargs)
        else:
            TelemetryBase.__init__(self, iter_blocks=True,
                do_handshaking=False, use_walltime=False, *args, **kwargs)

        self.done=False

    def _receive(self, finish_time):
        """ Fetch available data from file """

        buf = self.f.read(524288)   # 512k

        return buf

def get_telemetry_by_args(desc="Process telemetry", service_in_iter=True,
        iter_blocks=True):
    """ Parses command line to decide how to get a telemetry object. """
    # Setup the command line arguments.
    import argparse
    parser = argparse.ArgumentParser(description=desc)

    # Log format indicates this log is using the old file format which
    # embeds the timestamping information between the UAVTalk packet
    # instead of as part of the packet
    parser.add_argument("-t", "--timestamped",
                        action  = 'store_false',
                        default = None,
                        help    = "indicate that this is not timestamped in GCS format")

    parser.add_argument("-g", "--githash",
                        action  = "store",
                        dest    = "githash",
                        help    = "override githash for UAVO XML definitions")

    parser.add_argument("-s", "--serial",
                        action  = "store_true",
                        default = False,
                        dest    = "serial",
                        help    = "indicates that source is a serial port")

    parser.add_argument("-b", "--baudrate",
                        action  = "store",
                        dest    = "baud",
                        default = "115200",
                        help    = "baud rate for serial communications")

    parser.add_argument("-u", "--hid",
                        action  = "store_true",
                        default = False,
                        dest    = "hid",
                        help    = "use usb hid to communicate with FC")

    parser.add_argument("source",
            help  = "file, host:port, vid:pid, or serial port")

    # Parse the command-line.
    args = parser.parse_args()

    parse_header = False
    githash = None

    if args.githash is not None:
        # If we specify the log header no need to attempt to parse it
        githash = args.githash
    else:
        parse_header = True # only for files

    from dronin import telemetry

    if args.hid:
        return telemetry.HIDTelemetry(args.source,
                service_in_iter=service_in_iter, iter_blocks=iter_blocks,
                githash=githash)

    if args.serial:
        return telemetry.SerialTelemetry(args.source, speed=args.baud,
                service_in_iter=service_in_iter, iter_blocks=iter_blocks,
                githash=githash)

    if args.baud != "115200":
        parser.print_help()
        raise ValueError("Baud rates only apply to serial ports")

    import os.path

    if os.path.isfile(args.source):
        file_obj = open(args.source, 'rb')

        if parse_header:
            t = telemetry.FileTelemetry(file_obj, parse_header=True,
                gcs_timestamps=args.timestamped, name=args.source)
        else:
            t = telemetry.FileTelemetry(file_obj, parse_header=False,
                gcs_timestamps=args.timestamped, name=args.source,
                githash=githash)

        return t

    # OK, running out of options, time to try the network!
    host,sep,port = args.source.partition(':')

    if sep != ':':
        parser.print_help()
        raise ValueError("Target doesn't exist and isn't a network address")

    return telemetry.NetworkTelemetry(host=host, port=int(port), name=args.source,
            service_in_iter=service_in_iter, iter_blocks=iter_blocks,
            githash=githash)
