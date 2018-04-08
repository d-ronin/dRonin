"""
Interface to telemetry streams -- log, network, or serial.

Copyright (C) 2014-2015 Tau Labs, http://taulabs.org
Copyright (C) 2015-2016 dRonin, http://dronin.org
Licensed under the GNU LGPL version 2.1 or any later version (see COPYING.LESSER)
"""

import socket
import time
import errno
import sys
from threading import Condition

from . import uavtalk, uavo_collection, uavo

import os

from abc import ABCMeta, abstractmethod

class TelemetryBase(metaclass=ABCMeta):
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

        self.uavo_defs = uavo_defs

        self.uavtalk_generator = uavtalk.process_stream(uavo_defs,
            use_walltime=use_walltime, gcs_timestamps=gcs_timestamps,
            progress_callback=progress_callback,
            ack_callback=self.gotack_callback,
            nack_callback=self.gotnack_callback,
            reqack_callback=self.reqack_callback,
            filedata_callback=self.filedata_callback)

        # Kick the generator off to a sane start.
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
        self.req_obj = {}

        self.eof = False

        self.file_id = None

        self.first_handshake_needed = self.do_handshaking

        self.iter_idx = 0

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
        with self.cond:
            # TODO: Need to handle instance id better
            key = (obj._id, 0)

            request = self.req_obj.pop(key, None)

            if request is not None:
                request.completed(None, obj._id)

    def as_filtered_list(self, match_class, filter_cond=None, blocks=True):
        if isinstance(match_class, str):
            match_class = self.uavo_defs.find_by_name(match_class)

        if blocks:
            to_iter = self
        else:
            to_iter = self.uavo_list

        # Find the subset of this list that is of the requested class
        filtered_list = [x for x in to_iter if isinstance(x, match_class)]

        # Perform any additional requested filtering
        if filter_cond is not None:
            filtered_list = list(filter(filter_cond, filtered_list))

        return filtered_list

    def as_numpy_array(self, match_class, filter_cond=None, blocks=True):
        """ Transforms all received instances of a given object to a numpy array.

        match_class: the UAVO_* class you'd like to match.
        """

        import numpy as np

        # Find the subset of this list that is of the requested class
        filtered_list = self.as_filtered_list(match_class, filter_cond, blocks)

        # Perform any additional requested filtering
        if filter_cond is not None:
            filtered_list = list(filter(filter_cond, filtered_list))

        # Check for an empty list
        if filtered_list == []:
            return np.array([])

        return np.array(filtered_list, dtype=match_class._dtype)

    def __iter__(self):
        """ Iterator service routine. """
        iter_idx = self.iter_idx

        self.cond.acquire()

        while True:
            if iter_idx < len(self.uavo_list):
                obj = self.uavo_list[iter_idx]

                iter_idx += 1

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
                self.iter_idx = iter_idx
                self.cond.release()
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

        # It seems OK / desirable to do these synchronously.  Can always support
        # a future async API.
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
                #print("Disconnected")
                send_obj = self.__make_handshake('HandshakeReq')
            elif obj.Status == obj.ENUM_Status['HandshakeAck']:
                # Say connected
                #print("Handshake ackd")
                send_obj = self.__make_handshake('Connected')
            elif obj.Status == obj.ENUM_Status['Connected']:
                #print("Connected")
                send_obj = self.__make_handshake('Connected')

            self.send_object(send_obj)

    class PendingReq:
        def __init__(self, obj, inst_id, cb, retries=4, retry_time=0.5):
            self.obj = obj
            self.inst_id = inst_id
            self.retries = retries
            self.retry_time = retry_time
            self.expiration = time.time() + self.retry_time
            self.cbs = [ cb ]

        def inherit_old_state(self, replaces):
            if replaces is not None:
                self.cbs.extend(replaces)

        def key(self):
            return (self.obj._id, self.inst_id)

        def time_to_resend(self):
            if (not self.retries):
                return False

            if time.time() >= self.expiration:
                self.expiration = time.time() + self.retry_time
                self.retries -= 1
                return True

            return False

        def completed(self, value, id_val):
            for cb in self.cbs:
                cb(value, id_val)

        def expired(self):
            if (not self.retries) and (time.time() >= self.expiration):
                return True

            return False

        def make_request(self):
            return uavtalk.request_object(self.obj, self.inst_id)

    def _check_resends(self):
            # Check for anything that needs retrying--- requests,
            # acked operations.  Do at most one thing per cycle.

            for f in list(self.req_obj.values()):
                if f.expired():
                    self.req_obj.pop(f.key())
                    f.completed(None, f.obj._id)
                elif f.time_to_resend():
                    key = f.key()
                    self._send(f.make_request())

                    return

    def request_object(self, obj, inst_id=0, cb=None):
        if not self.do_handshaking:
            raise ValueError("Can only request on handshaking/bidir sessions")

        response = []

        def default_cb(val):
            with self.cond:
                response.append(val)
                # cond is awfully overloaded, but... the number of waiters
                # and event rates are relatively moderate.
                self.cond.notifyAll()

        if cb is None:
            cb = default_cb

        req = self.PendingReq(obj, inst_id, cb)
        key = req.key()

        with self.cond:
            old_req = self.req_obj.pop(key, None)

            if old_req is not None:
                req.inherit_old_state(old_req)

            self.req_obj[key] = req

            self._send(req.make_request())

            if cb == default_cb:
                while len(response) < 1:
                    self.cond.wait()

                return response[0]

    def filedata_callback(self, file_id, offset, eof, last_chunk, data):
        #print("Offs %d fd=[%s]" % (offset, data.hex()))
        with self.ack_cond:
            if self.file_id != file_id:
                return

            if offset != self.file_offset:
                self.file_chunkdone = True
                self.ack_cond.notifyAll()

            self.file_eof = eof
            self.file_chunkdone = last_chunk
            self.file_data += data
            self.file_offset += len(data)

            self.ack_cond.notifyAll()

    def request_filedata(self, file_id, offset):
        if not self.do_handshaking:
            raise ValueError("Can only request on handshaking/bidir sessions")

        self._send(uavtalk.request_filedata(file_id, offset))

    def save_object(self, obj, send_first=False):
        if send_first:
            self.send_object(obj, req_ack=True)

        save_obj = self.uavo_defs.find_by_name('UAVO_ObjectPersistence')

        save_req = save_obj._make_to_send(
                Operation = save_obj.ENUM_Operation['Save'],
                ObjectID = obj._id,
                InstanceID = 0
        )

        self.send_object(save_req, req_ack=True)

        for i in range(30):
            try:
                lv = self.last_values[save_obj]
                if lv.ObjectID == obj._id:
                    if lv.Operation == save_obj.ENUM_Operation['Completed']:
                        return

                    if lv.Operation != save_obj.ENUM_Operation['Save']:
                        raise Exception("Did not save successfully - bad status")
            except KeyError:
                pass

            time.sleep(0.05)

        raise Exception("Did not save successfully - timeout")

    def save_objects(self, objs, *arg, **kwargs):
        for obj in objs:
            self.save_object(obj, *arg, **kwargs)

    def transfer_file(self, file_id):
        with self.ack_cond:
            self.file_data = b''
            self.file_eof = False

            self.file_offset = 0
            self.file_id = file_id

        while not self.file_eof:
            with self.ack_cond:
                self.file_id = file_id
                self.file_chunkdone = False

            self.request_filedata(file_id, self.file_offset)

            with self.ack_cond:
                while not self.file_chunkdone:
                    self.ack_cond.wait(1.0)
                    # XXX timeout

                self.file_id = None

        return self.file_data

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

    def __handle_frames(self, frames):
        objs = []

        if frames is None:
            self.eof = True
            self._close()
        elif frames == b'':
            return
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

                key = (obj._id, obj.get_inst_id())

                request = self.req_obj.pop(key, None)

                if request is not None:
                    request.completed(obj, obj._id)

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

        while True:
            # 150ms timesteps
            expire = time.time() + 0.15

            self._check_resends()

            if (finish_time is not None) and (expire > finish_time):
                expire = finish_time

            data = self._receive(expire)
            self.__handle_frames(data)

            if self.eof:
                break

            if len(data):
                break

            if (finish_time is not None) and (time.time() >= finish_time):
                break

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

        if self.recv_buf is None:
            return None

        # Always do some minimal IO if possible
        self._do_io(0)

        if self.recv_buf is None:
            return None

        if len(self.recv_buf) < 1:
            self._do_io(finish_time)

        if self.recv_buf is None:
            return None

        if len(self.recv_buf) < 1:
            return b''

        ret = self.recv_buf
        self.recv_buf = b''

        return ret

    def _send(self, msg):
        """ Send a string to the controller """

        with self.send_lock:
            self.send_buf += msg

        self._do_io(0)

    @abstractmethod
    def _do_io(self, finish_time):
        return


class FDTelemetry(BidirTelemetry):
    """
    Implementation of bidirectional telemetry from a file descriptor.

    Intended for network streams.
    """

    def __init__(self, fd, write_fd=None, *args, **kwargs):
        """ Instantiates a telemetry instance on a given fd.

        Probably should only be called by derived classes.

         - fd: the file descriptor to perform telemetry operations upon

        Meaningful parameters passed up to TelemetryBase include: githash,
        service_in_iter, iter_blocks, use_walltime
        """

        BidirTelemetry.__init__(self, *args, **kwargs)

        self.fd = fd

        if write_fd is None:
            self.write_fd = fd
        else:
            self.write_fd = write_fd

    # Call select and do one set of IO operations.
    def _do_io(self, finish_time):
        import select

        rd_set = []
        wr_set = []

        did_stuff = False

        if self.recv_buf is None:
            return False

        if len(self.recv_buf) < 1024:
            rd_set.append(self.fd)
        elif len(self.send_buf) == 0:
            # If we don't want I/O, return quick!
            return True

        if len(self.send_buf) > 0:
            wr_set.append(self.write_fd)

        now = time.time()

        tm = finish_time-now
        if tm < 0: tm=0

        r,w,e = select.select(rd_set, wr_set, [], tm)

        with self.cond:
            if r:
                # Shouldn't throw an exception-- they just told us
                # it was ready for read.
                try:
                    chunk = os.read(self.fd, 1024)
                    if chunk == b'':
                        if self.recv_buf == b'':
                            self.recv_buf = None
                    else:
                        self.recv_buf = self.recv_buf + chunk

                    did_stuff = True
                except OSError as err:
                    # For some reason, we sometimes get a
                    # "Resource temporarily unavailable" error
                    if err.errno != errno.EAGAIN:
                        raise

            if w:
                with self.send_lock:
                    written = os.write(self.write_fd, self.send_buf)

                    if written > 0:
                        self.send_buf = self.send_buf[written:]

                    did_stuff = True

        return did_stuff

class SubprocessTelemetry(FDTelemetry):
    """ TCP telemetry interface. """
    def __init__(self, cmdline, shell=True, *args, **kwargs):
        """ Creates a telemetry instance talking over TCP.

         - host: hostname to connect to (default localhost)
         - port: port number to communicate on (default 9000)

        Meaningful parameters passed up to TelemetryBase include: githash,
        service_in_iter, iter_blocks, use_walltime
        """

        import subprocess

        sp = subprocess.Popen(cmdline, stdout=subprocess.PIPE,
                stdin=subprocess.PIPE, shell=shell)

        wfd = sp.stdin.fileno()
        rfd = sp.stdout.fileno()

        import fcntl

        flag = fcntl.fcntl(wfd, fcntl.F_GETFD)
        fcntl.fcntl(wfd, fcntl.F_SETFL, flag | os.O_NONBLOCK)

        flag = fcntl.fcntl(rfd, fcntl.F_GETFD)
        fcntl.fcntl(rfd, fcntl.F_SETFL, flag | os.O_NONBLOCK)

        self.sp = sp

        FDTelemetry.__init__(self, fd=rfd, write_fd=wfd, *args, **kwargs)

    def _close(self):
        self.sp.kill()

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

                if chunk != b'':
                    did_stuff = True
                    self.recv_buf = self.recv_buf + chunk
            except serial.serialutil.SerialException:
                # Ignore this; looks like a pyserial bug
                pass

            with self.send_lock:
                if self.send_buf != b'':
                    try:
                        written = self.ser.write(self.send_buf)

                        if written > 0:
                            self.send_buf = self.send_buf[written:]
                            did_stuff = True
                    except serial.SerialTimeoutException:
                        pass

            now = time.time()

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

    # Do USB I/O operations.
    def _do_io(self, finish_time):
        import errno

        did_stuff = False

        to_block = 10   # Wait no more than 10ms first time around

        while not did_stuff:
            chunk = b''

            try:
                raw = self.ep_in.read(64, timeout=to_block)

                if (raw[0] == 1):
                    length = raw[1]

                    if length <= len(raw) - 2:
                        chunk = raw.tostring()[2:2+length]
            except IOError as e:
                if e.errno != errno.ETIMEDOUT:
                    raise

            if chunk != b'':
                did_stuff = True
                self.recv_buf = self.recv_buf + chunk

            with self.send_lock:
                if self.send_buf != b'':
                    to_write = len(self.send_buf)

                    if to_write > 60:
                        to_write = 60

                    try:
                        buf = bytes((1, to_write)) + self.send_buf[0:to_write]

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
                if self.send_buf == b'':
                    remaining = finish_time - now

                    to_block = int((remaining * 0.75) * 1000) + 25
            else:
                if self.send_buf == b'':
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

        if buf == b'':
            return None

        return buf

def _finish_telemetry_args(parser, args, service_in_iter, iter_blocks):
    parse_header = False
    githash = None

    if args.githash is not None:
        # If we specify the log header no need to attempt to parse it
        githash = args.githash
    else:
        parse_header = True # only for files

    from dronin import telemetry

    if args.serial:
        return telemetry.SerialTelemetry(args.source, speed=args.baud,
                service_in_iter=service_in_iter, iter_blocks=iter_blocks,
                githash=githash)

    if args.baud != "115200":
        parser.print_help()
        raise ValueError("Baud rates only apply to serial ports")

    if args.hid:
        return telemetry.HIDTelemetry(args.source,
                service_in_iter=service_in_iter, iter_blocks=iter_blocks,
                githash=githash)

    if args.command:
        return telemetry.SubprocessTelemetry(args.source,
                service_in_iter=service_in_iter, iter_blocks=iter_blocks,
                githash=githash)

    import os.path

    if os.path.isfile(args.source):
        file_obj = open(args.source, 'rb')

        if parse_header:
            return telemetry.FileTelemetry(file_obj, parse_header=True,
                gcs_timestamps=args.timestamped, name=args.source)
        else:
            return telemetry.FileTelemetry(file_obj, parse_header=False,
                gcs_timestamps=args.timestamped, name=args.source,
                githash=githash)

    # OK, running out of options, time to try the network!
    host,sep,port = args.source.partition(':')

    if sep != ':':
        parser.print_help()
        raise ValueError("Target doesn't exist and isn't a network address")

    return telemetry.NetworkTelemetry(host=host, port=int(port), name=args.source,
            service_in_iter=service_in_iter, iter_blocks=iter_blocks,
            githash=githash)

def get_telemetry_by_args(desc="Process telemetry", service_in_iter=True,
        iter_blocks=True, arg_parser=None):
    """ Parses command line to decide how to get a telemetry object. """
    # Setup the command line arguments.

    if not arg_parser:
        import argparse
        parser = argparse.ArgumentParser(description=desc)
    else:
        parser = arg_parser

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

    parser.add_argument("-c", "--command",
                        action  = "store_true",
                        default = False,
                        dest    = "command",
                        help    = "indicates that source is a command")

    parser.add_argument("-u", "--hid",
                        action  = "store_true",
                        default = False,
                        dest    = "hid",
                        help    = "use usb hid to communicate with FC")

    parser.add_argument("source",
            help  = "file, host:port, vid:pid, command, or serial port")

    # Parse the command-line.
    args = parser.parse_args()

    ret = _finish_telemetry_args(parser, args, service_in_iter, iter_blocks)

    if arg_parser is None:
        return ret
    else:
        return (ret, args)
