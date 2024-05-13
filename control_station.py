import socket
from enum import Enum
import threading
from audio_process_alone import audio_process as audio_process_ext

verbose = False

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

def audio_process():
    global currentState
    if currentState.audio_status == "start":
            print("BEGINNING AUDIO")
            currentState.audio_status = "right"
    elif currentState.audio_status == "right":
            print("RECORDING AUDIO FOR RIGHT")
            currentState.audio_status = "move_left"
    elif currentState.audio_status == "move_left":
        print("MOVING TO LEFT")
        currentState.audio_status = "left"
    elif currentState.audio_status == "left":
            print("RECORDING AUDIO FOR LEFT")
            currentState.audio_status = "compare"
    elif currentState.audio_status == "compare":
        print("COMPARING")
        if currentState.audio_process_left > currentState.audio_process_right:
            currentState.sound_direction = "left"
        else:
            currentState.sound_direction = "right"
        print("SOUND DIRECTION:" + currentState.sound_direction)
        currentState.audio_status = "complete"
    elif currentState.audio_status == "complete":
        print("COMPLETE. "+currentState.sound_direction)
        currentState.leader_message = "sound_is_"+currentState.sound_direction


def handleStatus():
        global currentState
        match currentState.status:
                case Rounds.CHECKINS:
                        if (currentState.LeaderCheckIn == True):
                                if verbose: print("Moving to READY state.")
                                if verbose: print("")
                                currentState.status = Rounds.READY
                                print(" ---------------------->> All Robots Ready- Press Enter to Begin")
                                readyBlock = input()
                case Rounds.READY:
                        if verbose: print("We are READY. Leader Message is now Begin")
                        currentState.leader_message = "begin"
                        currentState.status = Rounds.LEADERMAZE
                case Rounds.LEADERMAZE:
                        if verbose: print("leader_maze")
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
    if verbose: print("------ HANDLING MESSAGE " + str(currentState.message_count) + ": " +  str(message) + " -----")
    message = str(message.strip())

    #CHECKINS
    if "check" in message:
        if "leader" in message:
            if verbose: print("Received Check-in from Leader")
            currentState.LeaderCheckIn = True

    #AFTER CHECKINS
    if currentState.status != Rounds.CHECKINS:
        if "leader" in message:
            if "audio" in message:
                    if verbose: print("Requested Audio Process")
                    audio_process()
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
                                        if verbose: print("SENT BACK: "+currentState.message)
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
