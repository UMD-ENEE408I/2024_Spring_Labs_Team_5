# 3.2 Numpy

import numpy as np

# q3
array = np.array([1, 2, 3, 4])
print(array)
print()

#q4
ones_array = np.ones([3, 4])
zeros_array = np.zeros([4, 3])

print(ones_array)
print(zeros_array)
print()

#q5
A = np.array([[1, 2, 3], [4, 5, 6]])
B = np.array([[1, 6, 6, 4], [2, 4, 6, 8], [1, 3, 5, 7]])
result = A @ B
print(result)
print()

#q6 
matrix = np.array([[3, 1], [1, 2]])
_, eigenvectors = np.linalg.eig(matrix)
print(eigenvectors)