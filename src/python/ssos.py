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
        prog_name = ffi.new("char[]", sys.argv[0].encode('ascii'))
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
            print ("ERROR: Unable to connect to the SOS daemon.")
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

    def get_guid(self):
        new_guid = ffi.new("uint64_t*")
        new_guid[0] = 0
        lib.SSOS_get_guid(new_guid)
        return new_guid[0]

    def pack(self, pyentry_name, entry_type, pyentry_value):
        entry_name = ffi.new("char[]", pyentry_name.encode('ascii'))
        if (entry_type == self.INT):
            entry_addr = ffi.new("int*", pyentry_value)
        elif (entry_type == self.LONG):
            entry_addr = ffi.new("long*", pyentry_value)
        elif (entry_type == self.DOUBLE):
            entry_addr = ffi.new("double*", pyentry_value)
        elif (entry_type == self.STRING):
            entry_addr = ffi.new("char[]", pyentry_value.encode('ascii'))
        else:
            print ("invalid type provided to SOS.pack(...): " \
                + entry_type + "  (doing nothing)")
            return
        lib.SSOS_pack(entry_name, entry_type, entry_addr)

    def request_pub_manifest(self, pub_title_filter, target_host, target_port):
        res_manifest          = ffi.new("SSOS_query_results*");
        res_max_frame_overall = ffi.new("int*");
        res_pub_title_filter  = ffi.new("char[]", pub_title_filter.encode('ascii'));
        res_target_host       = ffi.new("char[]", target_host.encode('ascii'));
        res_target_port       = ffi.new("int*", int(target_port));

        lib.SSOS_request_pub_manifest(res_manifest, res_max_frame_overall, \
                res_pub_title_filter, res_target_host, res_target_port[0])

        results = []
        for row in range(res_manifest.row_count):
            thisrow = []
            for col in range(res_manifest.col_count):
                thisrow.append(ffi.string(res_manifest.data[row][col]).decode('ascii'))
            #print "results[{}] = {}".format(row, thisrow)
            results.append(thisrow)

        # Generate the column name list:
        col_names = []
        for col in range(0, res_manifest.col_count):
            col_names.append(ffi.string(res_manifest.col_names[col]).decode('ascii'))

        lib.SSOS_result_destroy(res_manifest);

        max_frame = int(res_max_frame_overall[0])

        return (max_frame, results, col_names)



    def cache_grab(self, pub_filter, val_filter,    \
            frame_start, frame_depth,               \
            sos_host, sos_port):
        res_pub_filter = ffi.new("char[]", pub_filter.encode('ascii'))
        res_val_filter = ffi.new("char[]", val_filter.encode('ascii'))
        res_frame_start = ffi.new("int*", int(frame_start))
        res_frame_depth = ffi.new("int*", int(frame_depth))
        res_host = ffi.new("char[]", sos_host.encode('ascii'))
        res_port = ffi.new("int*", int(sos_port))
        # Send out the cache grab...
        lib.SSOS_cache_grab(res_pub_filter, res_val_filter,            \
                            res_frame_start[0], res_frame_depth[0],    \
                            res_host, res_port[0])

        res_obj = ffi.new("SSOS_query_results*")
        lib.SSOS_result_claim(res_obj);
        results = []
        for row in range(res_obj.row_count):
            thisrow = []
            for col in range(res_obj.col_count):
                thisrow.append(ffi.string(res_obj.data[row][col]).decode('ascii'))
            #print "results[{}] = {}".format(row, thisrow)
            results.append(thisrow)

        # Generate the column name list:
        col_names = []
        for col in range(0, res_obj.col_count):
           col_names.append(ffi.string(res_obj.col_names[col]).decode('ascii'))

        lib.SSOS_result_destroy(res_obj)
        return (results, col_names)


    def query(self, sql, host, port):
        res_sql = ffi.new("char[]", sql.encode('ascii'))
        res_obj = ffi.new("SSOS_query_results*")
        res_host = ffi.new("char[]", host.encode('ascii'))
        res_port = ffi.new("int*", int(port))

        # Send out the query...
        print ("== SSOS.PY: Sending the query...")
        lib.SSOS_query_exec(res_sql, res_host, res_port[0])
        # Grab the next available result.
        # NOTE: For queries submitted in a thread pool, this may not
        #       be the results for the query that was submitted above!
        #       Use of a thread pool requires that the results returned
        #       can be processed independently, for now.
        #print "Claiming the results..."
        print ("== SSOS.PY: Claiming the results...")
        lib.SSOS_result_claim(res_obj);

        #print "Results received!"
        #print "   row_count = " + str(res_obj.row_count)
        #print "   col_count = " + str(res_obj.col_count)

        print ("== SSOS.PY: Converting results to Python objects...")
        results = []
        for row in range(res_obj.row_count):
            thisrow = []
            for col in range(res_obj.col_count):
                thisrow.append(ffi.string(res_obj.data[row][col]).decode('ascii'))
            #print "results[{}] = {}".format(row, thisrow)
            results.append(thisrow)

        # Generate the column name list:
        col_names = []
        for col in range(0, res_obj.col_count):
            col_names.append(ffi.string(res_obj.col_names[col]).decode('ascii'))
        print ("== SSOS.PY:     len(results)   == " + str(len(results)))
        print ("== SSOS.PY:     len(col_names) == " + str(len(col_names)))

        print ("== SSOS.PY: Destroying the SOS results object...")
        lib.SSOS_result_destroy(res_obj)

        print ("== SSOS.PY: Returning Python-formatted results to calling function.")
        return (results, col_names)



    def trigger(self, handle, payload_size, payload_data):
        c_handle = ffi.new("char[]", handle.encode('ascii'))
        c_payload_size = ffi.new("int*", payload_size)
        c_payload_data = ffi.new("char[]", payload_data)
        c_payload_data = ffi.new("unsigned char[]", payload_data.encode('ascii'))
        lib.SSOS_sense_trigger(c_handle, c_payload_size[0], c_payload_data)

    def announce(self):
        lib.SSOS_announce()

    def publish(self):
        lib.SSOS_publish()

    def finalize(self):
        lib.SSOS_finalize()

if (__name__ == "__main__"):
    print ("This a library wrapper intended for use in other Python scripts.")
