import cv2

img = cv2.imread("opencv_pic_mbappe.jpg")

#q1
greyscale = cv2.cvtColor(img, cv2.COLOR_RGB2GRAY)
cv2.imshow("greyscale", greyscale)
cv2.waitKey(0)

#q2
edges = cv2.Canny(greyscale, 50, 150)
cv2.imshow('edges', edges)
cv2.waitKey(0)

#q3
haar_cascade = cv2.CascadeClassifier('haarcascade_frontalface_default.xml')
face = haar_cascade.detectMultiScale(greyscale, 1.1, 4)
for (x, y, w, h) in face:
    cv2.rectangle(img, (x, y), (x+w, y+h), (255, 0, 0), 2)

cv2.imshow('Image with Faces', img)
cv2.waitKey(0)

