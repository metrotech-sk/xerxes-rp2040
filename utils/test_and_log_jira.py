
import os
import sys
import argparse
from serial import Serial
import time
import logging
import re
import dotenv
from jira import JIRA
import statistics

dotenv.load_dotenv()

from xerxes_protocol import (
    XerxesRoot,
    XerxesNetwork,
    Leaf,
    DebugSerial,
    MsgIdMixin
)

# parse arguments
parser = argparse.ArgumentParser(description='Read process values from Xerxes Cutter device and print them in tight loop. Use Ctrl+C to exit.')
parser.add_argument(
    '-a', 
    "--address", 
    metavar='ADDR', 
    required=False, 
    type=int, default=0, 
    help='address of Xerxes Cutter device, default is 0'
)
parser.add_argument(
    "-d",
    "--debug",
    action="store_true",
    help="enable debug output"
)
parser.add_argument(
    "-p",
    "--port",
    metavar="PORT",
    default="/dev/ttyUSB0",
    help="serial port to use"
)
parser.add_argument(
    "-t",
    "--timeout",
    metavar="TIMEOUT",
    type=float,
    default=0.05,
    help="serial port timeout in seconds"
)
parser.add_argument(
    "-n",
    "--num_tests",
    metavar="NUM_TESTS",
    type=int,
    default=100,
    help="number of tests to run"
)
parser.add_argument(
    "-c",
    "--create",
    metavar="SUMMARY",
    type=str,
    help="create new Jira task if UUID not found with SUMMARY as summary"
)

args = parser.parse_args()

level = logging.DEBUG if args.debug else logging.INFO
logging.basicConfig(
    level=level,
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s"
)
log = logging.getLogger(__name__)

# create XerxesNetwork object
if args.debug:
    port = DebugSerial(args.port, timeout=args.timeout)
else:
    port = Serial(args.port, timeout=args.timeout)
    
def test_leaf(leaf: Leaf) -> str:
    test_output = {}
    pvs = [[] for _ in range(4)]
    for _ in range(args.num_tests):
        pvs[0].append(leaf.pv0)
        pvs[1].append(leaf.pv1)
        pvs[2].append(leaf.pv2)
        pvs[3].append(leaf.pv3)
    
    # calculate stdev and mean for each PV
    for i in range(4):
        stdev = statistics.stdev(pvs[i])
        mean = statistics.mean(pvs[i])
        # test_output += f"PV{i} mean: {mean:.4f}\n"
        # test_output += f"PV{i} stdev: {1000*stdev:.4f} /1000\n"
        test_output[f"PV{i} mean"] = mean
        test_output[f"PV{i} stdev [x1000]"] = 1000*stdev
    
    return test_output   
    
    
XN = XerxesNetwork(port)
XN.init(timeout=args.timeout)

# create XerxesRoot object
XR = XerxesRoot(0xfe, XN)

# create Leaf object
if args.address:
    leaf = Leaf(args.address, XR)
else:
    for i in range(32):
        leaf = Leaf(i, XR)
        try:
            ping_reply = leaf.ping()
            log.info(f"Found leaf at address {i}, latency: {ping_reply.latency} ms")
            break
        except Exception as e:
            pass
log.debug(f"Leaf parameters: {dir(leaf)}")

# log.info(f"Pinging leaf ... {leaf.ping()}")

# construct GET_INFO request
payload = bytes(MsgIdMixin(0x0005))  # MSGID_GET_INFO

reply = leaf.exchange(payload)

log.info(f"Reply latency: {reply.latency} ms")

import json

device_info = json.loads(reply.payload.decode())

log.info(f"Device info: {device_info}")
log.info("Running tests ...")
test_output = test_leaf(leaf)
log.info("###############################################################")
log.info(f"Test output: \n\n{test_output}")
log.info("###############################################################")

device_info["Test output"] = test_output

device_info = json.dumps(device_info, indent=4)

# By default, the client will connect to a Jira instance started from the Atlassian Plugin SDK
# (see https://developer.atlassian.com/display/DOCS/Installing+the+Atlassian+Plugin+SDK for details).

jira = JIRA(
    server="https://rubint.atlassian.net",
    # basic_auth=("admin", "admin"),  # a username/password tuple [Not recommended]
    basic_auth=(os.getenv("JIRA_EMAIL"), os.getenv("JIRA_API_TOKEN")),  # Jira Cloud: a username/token tuple
    # token_auth="API token",  # Self-Hosted Jira (e.g. Server): the PAT token
    # auth=("admin", "admin"),  # a username/password tuple for cookie auth [Not recommended]
)

# parse UUID:
pattern = pattern = r'"UUID": "(\d+)"'

uuid = re.search(pattern, device_info)
log.debug(f"UUIDs found: {uuid}")
log.info(f"Updating device with UUID: {uuid[1]}")

assert(uuid), "UUID not found in device info"

uuid=uuid[1]
# get all tasks in "SN" project matching the UUID
issues = jira.search_issues(f"project=SN AND text~{uuid}")
if len(issues) == 0:
    log.warning("UUID not found")
    if args.create:
        log.info(f"Creating new task for {uuid}")       
        # try to create a new issue:
        issue_dict = {
            'project': {'key': "SN"},
            'summary': args.create,
            'description': device_info,
            'issuetype': {'name': 'Device'},
        }
        
        new_issue = jira.create_issue(fields=issue_dict)  # uncomment to create
        log.info(f"New Issue key: {new_issue.key}")
elif len(issues) > 1:
    log.error(f"Found more than one task matching the UUID: {uuid}")
else:
    issue = issues[0]
    log.info(f"Found task {issue.key} - {issue.fields.summary}")
    issue.update(fields={"description": device_info})
    log.info(f"Updated task {issue.key} - {issue.fields.summary}")    
