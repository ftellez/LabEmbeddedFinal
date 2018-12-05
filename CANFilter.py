import can 
import binascii
import sys
import select
import time
import serial
import gspread
from oauth2client.service_account import ServiceAccountCredentials

scope = ['https://spreadsheets.google.com/feeds',
         'https://www.googleapis.com/auth/drive']

credentials = ServiceAccountCredentials.from_json_keyfile_name('credentials.json', scope)

gc = gspread.authorize(credentials)
wks = gc.open("LabEmbedded").sheet1
row = 2

# PGNs
EEC1_Filter = 0xF004
DD_Filter = 0xFEFC
ET_Filter = 0x00FEEE
DL_Filter = 0x00ff14

ser = serial.Serial("/dev/ttyACM0",
                    baudrate=57600,
                    parity = serial.PARITY_NONE,
                    bytesize = serial.EIGHTBITS,
                    timeout=1)


def read_serial():
        while 1:
                if ser.inWaiting() > 0:
                        break
                time.sleep(0.5)
        return ser.readline()


def printSPN(PGN):
    numSPNs = DBC[PGN].keys()
DBC = {}
DBC[EEC1_Filter] = [(899, 0, 1), (512, 1, 1), (513, 2, 1), (190, 4, 2), (1483, 5, 1), (1675, 6, 1), (24327, 7, 1)]
DBC[DD_Filter] = [(80, 0, 1), (96, 1, 1), (95, 2, 1), (99, 3, 1), (169, 5, 2), (38, 6, 1), (7471, 7, 1)]
DBC[ET_Filter] = [(110, 0, 1), (174, 1, 1), (175, 3, 2), (176, 5, 2), (52, 6, 1), (1134, 7, 1)]
DBC[DL_Filter] = [(0, 0, 1), (1, 1, 1), (2, 2, 1), (3, 3, 1), (4, 4, 1), (5, 5, 1), (6, 6, 1), (7, 7, 1)]

PGNNames = {}
PGNNames[EEC1_Filter] = 'Electronic Engine Controller 1'
PGNNames[DD_Filter] = 'Dashboard Display'
PGNNames[ET_Filter] = 'Engine Temperature'
PGNNames[DL_Filter] = 'Dashboard Lamps'

SPNNames = {}
SPNNames[899] = 'Engine Torque Mode'
SPNNames[512] = 'Drivers Demand Engine % Torque'
SPNNames[513] = 'Actual Engine - % Torque'
SPNNames[190] = 'Engine Speed'
SPNNames[1483] = 'Src Addr of Controlling Dev for Eng Ctrl'
SPNNames[1675] = 'Engine Starter Mode'
SPNNames[2432] = 'Engine Demand - % Torque'

SPNNames[80] = 'Washer Fluid Level'
SPNNames[96] = 'Fuel Level'
SPNNames[95] = 'Eng Fuel Filter Dif Pressure'
SPNNames[99] = 'Eng Oil Filter Dif Pressure'
SPNNames[169] = 'Cargo Ambient Temp'
SPNNames[38] = 'Fuel Level 2'
SPNNames[7471] = 'Eng Oil Filter Diff Pressure Ext Rng'

SPNNames[110] = 'Eng Cool Temp'
SPNNames[174] = 'Eng Fuel Temp'
SPNNames[175] = 'Eng Oil Temp'
SPNNames[176] = 'Eng Turbocharger Oil Temp'
SPNNames[52] = 'Eng Intercooler Temp'
SPNNames[1134] = 'Eng Intercooler Therm Opening'

SPNNames[0] = 'Turn Lft Ind'
SPNNames[1] = 'Turn Ri Ind'
SPNNames[2] = 'Hi Beam'
SPNNames[3] = 'PTO Engaged'
SPNNames[4] = 'Electro Hyd Hitch'
SPNNames[5] = 'Battery Charging'
SPNNames[6] = 'Eng Air Cleaner Restr'
SPNNames[7] = 'Eng Oil Pressure'

def getMsgId(msg):
    return (msg.arbitration_id & 0x00FFFF00) >> 8

def PrintCANMessage(msg):
    data = "".join("%02X" % b for b in msg.data)
    listSPNS = []
    msgId = getMsgId(msg)
    print("0x%X"% msgId + '\t' + PGNNames[msgId])
    for SPNid, index, numBytes in DBC[msgId]:
        spn = ""
        print(str(SPNid) + ' - ' , end='')
        for i in range(2*index, 2*index-2*numBytes, -2):
            spn += data[i]+data[i+1]
        print(spn)
        listSPNS.append(str(int(spn, 16)))
    tsRTC = read_serial()
    while len(tsRTC) < 5:
        tsRTC = read_serial()
    tsRTC = str(tsRTC, "utf-8").replace('\n', '').replace('\r', '')
    print('fecha: ' + tsRTC)
    print()
    sendToSpreadsheet(listSPNS, msgId, tsRTC)

def sendToSpreadsheet(listSPNS, msgId, timestamp):
    global row
    #newRows = []
    for countSPN in range(len(listSPNS)):
        newRow = [
            timestamp,
            str(msgId),
            DBC[msgId][countSPN][0],
            listSPNS[countSPN],
            "0x" + str(msgId) + "00",
            "0xFFFFFFFF"]
        wks.append_row(newRow)
        #wks.update_acell('A'+str(row), str(timestamp))
        #wks.update_acell('B'+str(row), str(msgId))
        #wks.update_acell('C'+str(row), DBC[msgId][countSPN][0])
        #wks.update_acell('D'+str(row), listSPNS[countSPN])
        #wks.update_acell('E'+str(row), "0x" + str(msgId) + "00")
        #wks.update_acell('F'+str(row), "0xFFFFFFFF")
        #row += 1
    #result = 
    
canIf = 'can0'
bus = can.interface.Bus(canIf, bustype='socketcan_native')

mainMenu = '''Selecciona un PGN
    (1) 0xF004 Electrical Engine Controller 1
    (2) 0xFEFC Dashboard Display
    (3) 0xFEEE Engine Temperature
    (4) 0xFF14 Display Lamps
    (5) Salir'''

def optionInput():
    if sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
      line = sys.stdin.readline()
      if line:
        return line
      else: # an empty line means stdin has been closed
        exit(0)
    else:
        time.sleep(.1)
        return optionInput()
    
option = '0'
while option != '5':
    print(mainMenu)
    print('PGN:')
    
    option = optionInput()[0:-1]
    # If there's input ready, do something, else do something
    # else. Note timeout is zero so select won't block at all.
    
  
    if option == '1':
        pgn = 0xf004
    elif option == '2':
        pgn = 0xfefc
    elif option == '3':
        pgn = 0xfeee
    elif option == '4':
        pgn = 0xff14
    else:
        pgn = -1

    if pgn != -1:
        while 1:
            msg = bus.recv()
            msgId = getMsgId(msg)
            while msgId != pgn:    
                msg = bus.recv()
                msgId = getMsgId(msg)
            PrintCANMessage(msg)
            if sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
              line = sys.stdin.readline()
              if line:
                break
              else: # an empty line means stdin has been closed
                exit(0)
