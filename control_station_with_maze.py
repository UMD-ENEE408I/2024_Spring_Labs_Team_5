import socket
from enum import Enum
import threading
from audio_process import record_rms
import time
import cv2
import numpy as np
import imutils

verbose = True

HOST = '0.0.0.0' # Listen on all available interfaces
PORT = 8080

# maze constants
RIGHT = 'right'
LEFT = 'left'
DOWN = 'down'
UP = 'up'
F = 'forward'
B = 'backward'
R = RIGHT
L = LEFT

# color masks
lower_blue = np.array([100, 225, 75])
upper_blue = np.array([115, 255, 255])
lower_green = np.array([40, 70, 180])
upper_green = np.array([80, 255, 255])
lower_red = np.array([160, 50, 50])
upper_red = np.array([180, 255, 255])
lower_grey = np.array([0, 0, 50])
upper_grey = np.array([179, 20, 230])
cap = cv2.VideoCapture(0)
kernel_size = 5
distance_threshold = 3

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
                self.FollowerCheckIn = False
                self.Robot2CheckIn = False

                self.LeaderRecentRequest = "None"
                self.Robot1RecentRequest = "None"
                self.Robot2RecentRequest = "None"
                
                self.message = "leader:reset,follower:reset,|"
                self.leader_message = "reset"
                self.follower_message = "rest"
                
                self.audio_process_left = 0
                self.audio_process_right = 0
                self.audio_status = "start"
                self.sound_direction = "neither"
                
                # maze variables
                self.x_pos = 0
                self.y_pos = 0
                self.max_board = 6
                self.traversed = [[0 for _ in range(self.max_board)] for _ in range(self.max_board)]
                self.facing = RIGHT
                self.movement_queue = []
                self.bot = 1
                self.maze_status = "start"
                self.maze_counter = 0
                
                
currentState = State()

audio_result = None
audio_finished_event = threading.Event()

def traverse_maze(counter):
        global currentState
        
        if counter != currentState.maze_counter:
                return

        def update():
               currentState.traversed[currentState.y_pos][currentState.x_pos] += 1

        def object_detection():
                
                def find_marker(image):
                        gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
                        gray = cv2.GaussianBlur(gray, (kernel_size, kernel_size), 0)
                        edged = cv2.Canny(gray, 125, 250)
                        cv2.imshow('canny', edged)
                        cnts = cv2.findContours(edged.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
                        cnts = imutils.grab_contours(cnts)

                        try:
                                cnts = max(cnts, key = cv2.contourArea)
                        except ValueError:
                                return None
                        return cv2.minAreaRect(cnts)

                def distance(perWidth):
                        return (24 * 11) / perWidth
                
                ret, frame = cap.read()

                height, width, _ = frame.shape
                
                # crop image
                cropped_dim = 300
                cropped_height_start = int(height / 2 - (cropped_dim / 2))
                cropped_width_start = int(width / 2 - (cropped_dim / 2))
                frame = frame[cropped_height_start:cropped_height_start + cropped_dim, cropped_width_start:cropped_width_start + cropped_dim]
    
                frame = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

                factor = 1.5 
                frame[:, :, 1] = np.clip(frame[:, :, 1] * factor, 0, 255)

                blue_mask = cv2.inRange(frame, lower_blue, upper_blue)
                green_mask = cv2.inRange(frame, lower_green, upper_green)
                red_mask = cv2.inRange(frame, lower_red, upper_red)
                
                frame_red = frame.copy()
                frame_blue = frame.copy()
                frame_green = frame.copy()

                frame_red = cv2.bitwise_and(frame_red, frame_red, mask=red_mask)
                frame_blue = cv2.bitwise_and(frame_blue, frame_blue, mask=blue_mask)
                frame_green = cv2.bitwise_and(frame_green, frame_green, mask=green_mask)

                red_marker = find_marker(frame_red)
                #red_box = np.int0(cv2.boxPoints(red_marker))

                blue_marker = find_marker(frame_blue)
                #blue_box = np.int0(cv2.boxPoints(blue_marker))

                green_marker = find_marker(frame_green)
                #green_box = np.int0(cv2.boxPoints(green_marker))

                d_red = -1
                d_blue = -1
                d_green = -1

                if(red_marker != None):
                        d_red = distance(red_marker[1][0])
                else:
                       d_red = -1

                if(blue_marker != None):
                        d_blue = distance(blue_marker[1][0])
                else:
                       d_blue = -1

                if(green_marker != None):
                        d_green = distance(green_marker[1][0])
                else:
                       d_green = -1


                if(d_red < distance_threshold or d_blue < distance_threshold or d_green < distance_threshold):
                        return True
                else:
                        return False

        def avoid_object():
                if currentState.facing == RIGHT or currentState.facing == DOWN:
                        if currentState.y_pos == 0 and currentState.x_pos == 2:
                                currentState.movement_queue = [B, R, F, L, F, R] + currentState.movement_queue[1:]
                        else:
                                currentState.movement_queue = [B, R, F, L, F, F, L, F, R] + currentState.movement_queue[1:]
                elif currentState.facing == LEFT:
                        if currentState.y_pos == 5:
                                currentState.movement_queue = [B, R, F, L, F, R] + currentState.movement_queue[1:]
                        elif currentState.y_pos == 1:
                                currentState.movement_queue = [B, L, F, R, F, L] + currentState.movement_queue[1:]
                elif currentState.facing == UP:
                        currentState.movement_queue = [B, L, F, R, F, F, R, F, L] + currentState.movement_queue[1:]

        def rotate(direction):
                if direction == RIGHT:
                        if currentState.facing == UP:
                                currentState.facing = RIGHT
                        elif currentState.facing == RIGHT:
                                currentState.facing = DOWN
                        elif currentState.facing == DOWN:
                                currentState.facing = LEFT
                        elif currentState.facing == LEFT:
                                currentState.facing = UP

                elif direction == LEFT:
                        if currentState.facing == UP:
                                currentState.facing = LEFT
                        elif currentState.facing == LEFT:
                                currentState.facing = DOWN
                        elif currentState.facing == DOWN:
                                currentState.facing = RIGHT
                        elif currentState.facing == RIGHT:
                                currentState.facing = UP

        def move_backward():
                if currentState.facing == DOWN:
                        if 0 <= currentState.y_pos - 1 < currentState.max_board: 
                                currentState.y_pos -= 1
                elif currentState.facing == UP:
                        if 0 <= currentState.y_pos + 1 < currentState.max_board:
                                currentState.y_pos += 1
                elif currentState.facing == RIGHT:
                        if 0 <= currentState.x_pos - 1 < currentState.max_board:
                                currentState.x_pos -= 1
                elif currentState.facing == LEFT:
                        if 0 <= currentState.x_pos + 1 < currentState.max_board:
                                currentState.x_pos += 1
                update()

        def move_forward():
                if currentState.facing == UP:
                        if 0 <= currentState.y_pos - 1 < currentState.max_board: 
                                currentState.y_pos -= 1
                elif currentState.facing == DOWN:
                        if 0 <= currentState.y_pos + 1 < currentState.max_board:
                                currentState.y_pos += 1
                elif currentState.facing == LEFT:
                        if 0 <= currentState.x_pos - 1 < currentState.max_board:
                                currentState.x_pos -= 1
                elif currentState.facing == RIGHT:
                        if 0 <= currentState.x_pos + 1 < currentState.max_board:
                                currentState.x_pos += 1

                update()
               
        def move(command):
                if command == F:
                        move_forward()
                elif command == B:
                        move_backward()
                elif command == R:
                        rotate(RIGHT)
                elif command == L:
                        rotate(LEFT)

                if(object_detection() == True):
                        avoid_object()

        if(len(currentState.movement_queue) == 0):
                if(currentState.bot == 1):
                        for i in range(5):
                                currentState.movement_queue.append(F)
                        for row in currentState.traversed:
                                break
                                row[:3] = [1] * 3
                else:
                        for i in range(2):
                                currentState.movement_queue.append(F)
                        for row in currentState.traversed:
                                break
                                row[-3:] = [1] * 3

                currentState.movement_queue = currentState.movement_queue + [R, F, F, F, F, F, R, F, R, F, F, F, F, L, F, L, F, F, F, F]

        command = currentState.movement_queue.pop(0)
        print('command popped: ' + str(command))
        move(command)
        if len(currentState.movement_queue) == 0:
                currentState.follower_message = "maze_complete"
        else:
                currentState.follower_message = command+str(counter)


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
                        if (currentState.FollowerCheckIn == True):
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
        currentState.message = "leader:"+currentState.leader_message+",follower:"+currentState.follower_message+",|"

def handleMessage(message):
    global currentState
    if verbose and len(str(message).strip()) != 0: print("RECD " + str(currentState.message_count) + ": " +  str(message))
    message = str(message.strip())

    #CHECKINS
    if "check" in message:
        if "leader" in message:
            currentState.LeaderCheckIn = True
        if "follower" in message:
                currentState.FollowerCheckIn = True

    #AFTER CHECKINS
    if currentState.status != Rounds.CHECKINS:
        if "leader" in message:
            if "audio" in message:
                    if verbose: print("Requested Audio Process")
                    audio_process(message)
        elif "follower" in message:
                if "nav_" in message:
                        print("EXTRACTED COUNTER #"+str(message[message.find("nav_"):message.find("nav_")+1]))
                        traverse_maze(int(message[message.find("nav_"):message.find("nav_")+1]))
                print("Received message from robot1.")
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
