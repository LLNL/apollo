# file "ssos.py"
#
#   SSOS API for Python
#

import time
import sys
import subprocess
import os
import glob
import io
import csv
from ssos_python import ffi, lib

class SSOS:
    INT    = lib.SSOS_TYPE_INT
    LONG   = lib.SSOS_TYPE_LONG
    DOUBLE = lib.SSOS_TYPE_DOUBLE
    STRING = lib.SSOS_TYPE_STRING

    def init(self):
        prog_name = ffi.new("char[]", sys.argv[0])
        lib.SSOS_init(prog_name)
        is_online_flag = ffi.new("int*")
        is_online_flag[0] = 0
        lib.SSOS_is_online(is_online_flag)
        connect_delay = 0
        while((connect_delay < 4) and (is_online_flag[0] < 1)):
            lib.SSOS_is_online(is_online_flag)
            #print "   ... waiting to connect to SOS"
            time.sleep(0.5)
            connect_delay += 1

        if (is_online_flag[0] < 1):
            print "ERROR: Unable to connect to the SOS daemon."
            exit()

    def is_online(self):
        is_online_flag = ffi.new("int*")
        is_online_flag[0] = 0
        lib.SSOS_is_online(is_online_flag)
        return bool(is_online_flag[0])

    def is_query_done(self):
        is_query_done_flag = ffi.new("int*")
        is_query_done_flag[0] = 0
        lib.SSOS_is_query_done(is_query_done_flag)
        return bool(is_query_done_flag[0])

    def pack(self, pyentry_name, entry_type, pyentry_value):
        entry_name = ffi.new("char[]", pyentry_name)
        if (entry_type == self.INT):
            entry_addr = ffi.new("int*", pyentry_value)
        elif (entry_type == self.LONG):
            entry_addr = ffi.new("long*", pyentry_value)
        elif (entry_type == self.DOUBLE):
            entry_addr = ffi.new("double*", pyentry_value)
        elif (entry_type == self.STRING):
            entry_addr = ffi.new("char[]", pyentry_value)
        else:
            print "invalid type provided to SOS.pack(...): " \
                + entry_type + "  (doing nothing)"
            return
        lib.SSOS_pack(entry_name, entry_type, entry_addr)

    def query(self, sql, host, port):
        res_sql = ffi.new("char[]", sql)
        res_obj = ffi.new("SSOS_query_results*")
        res_host = ffi.new("char[]", host)
        res_port = ffi.new("int*", int(port))

        # Send out the query...
        #print "Sending the query..."
        lib.SSOS_query_exec(res_sql, res_host, res_port[0])
        # Grab the next available result.
        # NOTE: For queries submitted in a thread pool, this may not
        #       be the results for the query that was submitted above!
        #       Use of a thread pool requires that the results returned
        #       can be processed independently, for now.
        #print "Claiming the results..."
        lib.SSOS_result_claim(res_obj);

        results = []
        for row in range(res_obj.row_count):
            thisrow = []
            for col in range(res_obj.col_count):
                thisrow.append(ffi.string(res_obj.data[row][col]))
            #print "results[{}] = {}".format(row, thisrow)
            results.append(thisrow)

        # Generate the column name list:
        col_names = []
        for col in range(0, res_obj.col_count):
           col_names.append(ffi.string(res_obj.col_names[col]))

        lib.SSOS_result_destroy(res_obj)
        return (results, col_names)



    def trigger(self, handle, payload_size, payload_data):
        c_handle = ffi.new("char[]", handle)
        c_payload_size = ffi.new("int*", payload_size)
        c_payload_data = ffi.new("unsigned char[]", payload_data)
        lib.SSOS_sense_trigger(c_handle, c_payload_size[0], c_payload_data)

    def announce(self):
        lib.SSOS_announce()

    def publish(self):
        lib.SSOS_publish()

    def finalize(self):
        lib.SSOS_finalize()

if (__name__ == "__main__"):
    print "This a library wrapper intended for use in other Python scripts."
