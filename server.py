import socket

server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind(('192.168.230.191', 8000))  # Bind to all interfaces on port 12345
server_socket.listen()

print("Server is listening for connections...")

while True:
    client_socket, address = server_socket.accept()
    print(f"Connection from {address} has been established.")

    client_socket.send(bytes("Welcome to the server!", "utf-8"))
    while True:
        data = client_socket.recv(1024)
        if not data:
            break 
        print("Received:", data.decode("utf-8"))
        client_socket.send(data)
    client_socket.close()
