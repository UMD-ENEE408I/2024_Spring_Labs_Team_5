import cv2


cap = cv2.VideoCapture(0)
    
while True: 
    ret,img=cap.read()
    
    haar_cascade = cv2.CascadeClassifier('haarcascade_frontalface_default.xml')
    face = haar_cascade.detectMultiScale(img, 1.1, 4)
    for (x, y, w, h) in face:
        cv2.rectangle(img, (x, y), (x+w, y+h), (255, 0, 0), 2)

    cv2.imshow('frame', img)
    if(cv2.waitKey(10) & 0xFF == ord('b')):
        break