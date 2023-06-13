#! /usr/bin/env python3

import socket

HOST = "127.0.0.1"
PORT = 57943 #637943

# creazione del server socket
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))
    s.listen()   
    while True:
      print(f"In attesa di un client su porta {PORT}...")
      # mi metto in attesa di una connessione
      conn, addr = s.accept()
      # lavoro con la connessione appena ricevuta 
      with conn:  
         print(f"Contattato da {addr}")
         tipo = conn.recv(1)
         if tipo.decode() == "0":
            print("Connessione A")
            parola = conn.recv(2048).decode().strip("\x00").rstrip('\n')
            print(f"Parola ricevuta:{parola}")
            with open("capolet","wb") as f:
               f.write(str(len(parola)).encode())
               f.write(parola.encode())
            print("chiusa connessione")

         elif tipo.decode() == "1":
            totseq = 0
            print("connessione B")
            while True:
               parola = conn.recv(2048).decode().strip("\x00").rstrip('\n')
               print(f"Parola ricevuta:{parola}")
               if parola == "":
                  print(totseq)
                  conn.sendall(totseq.to_bytes(4, byteorder='big'))
                  break
               totseq += 1
               with open("caposc","wb") as f:
                  f.write(str(len(parola)).encode())
                  f.write(parola.encode())
            print("chiusa connessione")