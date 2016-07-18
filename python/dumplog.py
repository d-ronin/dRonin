#!/usr/bin/env python

if __name__ == "__main__":
    from dronin import telemetry
    uavo_list = telemetry.get_telemetry_by_args()

    for o in uavo_list: print(o)
