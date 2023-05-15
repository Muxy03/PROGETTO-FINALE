#! /usr/bin/env python3

import sys, struct, socket,logging,os

HOST = "127.0.0.1"
PORT = "57943" # MAT:637943

def main(host=HOST,port=PORT):

    if(os.path.exists("capolet") == False):
        os.mkfifo("capolet")
    
    if(os.path.exists("caposc") == False):
        os.mkfifo("caposc")
    
    with socket.socket(socket.AF_INET,socket.SOCK_STREAM) as server:
        try:
            server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server.bind((host, port))
            server.listen()
            while True:
                conn, addr = server.accept()
                gestione_connessione(conn,addr)
        except KeyboardInterrupt:
            pass
        server.shutdown(socket.SHUT_RDWR)


def gestione_connessione(conn,addr):
    with conn:
        client_t = conn.recv(1)
        if client_t == b'0': # client tipo A
            data = recv_all(conn,2048)
            if len(data) == 0:
                raise RuntimeError("errore connessione socket\n")
            line = struct.unpack(f"!{len(data)}s",data)[0]
            with open("capolet","w") as fifoL:
                fifoL.write(line)
        elif client_t == b'1': # client tipo B
            pass

def recv_all(conn,n):
  chunks = b''
  bytes_recd = 0
  while bytes_recd < n:
    chunk = conn.recv(min(n - bytes_recd, 2048))
    if len(chunk) == 0:
      raise RuntimeError("socket connection broken")
    chunks += chunk
    bytes_recd = bytes_recd + len(chunk)
    assert bytes_recd == len(chunks)
  return chunks