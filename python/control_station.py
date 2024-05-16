import socket
from enum import Enum
import threading
from audio_process import record_rms
import time

verbose = True

HOST = '0.0.0.0' # Listen on all available interfaces
PORT = 8080

# STATUS VARIABLES
class Rounds(Enum):
        CHECKINS = 1
        READY = 2
        LEADERMAZE = 3
        ALLACTIVE = 4
        WAITFORMAZE = 5
        STRAIGHTAWAY = 6
        COMPLETE = 7
        

class State():
        def __init__(self):
                self.status = Rounds.CHECKINS
                self.message_count = 0
                
                self.LeaderCheckIn = False
                self.Robot1CheckIn = False
                self.Robot2CheckIn = False

                self.LeaderRecentRequest = "None"
                self.Robot1RecentRequest = "None"
                self.Robot2RecentRequest = "None"
                
                self.message = "leader:reset,robot1:reset,robot2:reset,|"
                self.leader_message = "reset"
                
                self.audio_process_left = 0
                self.audio_process_right = 0
                self.audio_status = "start"
                self.sound_direction = "neither"
                
currentState = State()

audio_result = None
audio_finished_event = threading.Event()


def audio_process(message):
    global currentState
    if currentState.audio_status == "start" and "audio_right" in message:
        print("BEGINNING AUDIO RECORD (RIGHT)")
        currentState.audio_process_right = record_rms()
        print("COMPLETED AUDIO RECORD")
        currentState.audio_status = "moving"
        print("REQUESTING ROBOT MOVE")
        currentState.leader_message = "side_right_complete"
    elif currentState.audio_status == "moving":
        if "audio_left" in message:
            print("ROBOT MOVE COMPLETE")
            currentState.audio_status = "audio_left"
        print("CONTINUING TO REQUEST ROBOT MOVE")
        currentState.leader_message = "side_right_complete"
        
    elif currentState.audio_status == "audio_left":
        print("BEGINNING AUDIO RECORD (LEFT)")
        currentState.audio_process_left = record_rms()
        print("COMPLETED AUDIO RECORD")
        if currentState.audio_process_left > currentState.audio_process_right:
            currentState.sound_direction = "left"
        else:
            currentState.sound_direction = "right"
        currentState.audio_status = "complete"
        print("DETERMINED SOUND DIRECTION:" + currentState.sound_direction)
        currentState.leader_message = ("direction_" + currentState.sound_direction)
    elif currentState.audio_status == "complete":
        print("AUDIO COMPLETE. "+currentState.sound_direction)
        currentState.leader_message = "direction_"+currentState.sound_direction


def handleStatus():
        global currentState
        match currentState.status:
                case Rounds.CHECKINS:
                        if (currentState.LeaderCheckIn == True):
                                currentState.status = Rounds.READY
                                print(" ---------------------->> All Robots Ready- Press Enter to Begin")
                                readyBlock = input()
                case Rounds.READY:
                        if verbose: print("Status -> READY")
                        currentState.leader_message = "begin"
                        currentState.status = Rounds.LEADERMAZE
                case Rounds.ALLACTIVE:
                        return
                case Rounds.WAITFORMAZE:
                        return
                case Rounds.STRAIGHTAWAY:
                        return
                case Rounds.COMPLETE:
                        return
        currentState.message = "leader:"+currentState.leader_message+",|"

def handleMessage(message):
    global currentState
    if verbose and len(str(message).strip()) != 0: print("RECD " + str(currentState.message_count) + ": " +  str(message))
    message = str(message.strip())

    #CHECKINS
    if "check" in message:
        if "leader" in message:
            currentState.LeaderCheckIn = True

    #AFTER CHECKINS
    if currentState.status != Rounds.CHECKINS:
        if "leader" in message:
            if "audio" in message:
                    if verbose: print("Requested Audio Process")
                    audio_process(message)
        elif "robot1" in message:
                print("Received message from robot1.")
        elif "robot2" in message:
                print("Received message from robot2.")
    handleStatus()
    currentState.message_count += 1

def main():
    global currentState
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        if verbose: print("Beginning to Accept Connections...")
        s.bind((HOST, PORT))
        s.listen()
        while True:
            conn, addr = s.accept()
            with conn:
                try:
                        while True:
                                data = conn.recv(1024)
                                try:
                                        conn.sendall((currentState.message).encode())
                                        if verbose and len(str(data.decode()).strip()) != 0: print("SENT: "+currentState.message)
                                except BrokenPipeError:
                                        print("Message Did Not Make it...")
                                if not data:
                                        break
                                handleMessage(data.decode())
                except ConnectionResetError:
                        print("Connection Reset Error :( ")    
            conn.close()

if __name__ == "__main__":
    main()
