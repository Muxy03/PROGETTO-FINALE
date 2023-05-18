#! /usr/bin/env python3

import struct, socket,logging,os,argparse,subprocess,signal,time,concurrent.futures

Description = "Server"
logging.basicConfig(filename='server.log',level=logging.DEBUG,  datefmt='%d/%m/%y %H:%M:%S', format='%(asctime)s - %(levelname)s - %(message)s')

HOST = "127.0.0.1"
PORT = "57943" # MAT:637943

def main(host=HOST,port=PORT,nthreads=0):
    
    if(os.path.exists("capolet") == False):
        print("creo fifo capolet")
        os.mkfifo("capolet")
    
    if(os.path.exists("caposc") == False):
        print("creo fifo caposc")
        os.mkfifo("caposc")
    
    # creiamo il server socket
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:  
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)            
            s.bind((host, port))
            s.listen()
            with concurrent.futures.ThreadPoolExecutor(max_workers=nthreads) as executor:
                while True:
                    print("In attesa di un client...")
                    # mi metto in attesa di una connessione
                    conn, addr = s.accept()
                     # l'esecuzione di submit non è bloccante
                    # fino a quando ci sono thread liberi
                    executor.submit(gestione_connessione, conn,addr)
        except KeyboardInterrupt:
            s.shutdown(socket.SHUT_RDWR)
            os.unlink("capolet")
            os.unlink("caposc")
            p.send_signal(signal.SIGTERM)


def gestione_connessione(conn,addr):
    with conn:
        client_t = conn.recv(1)
        if client_t == b'0': # client connessione tipo A
            print(f"connessione tipo A da {addr}")
            data = recv_all(conn,2048)
            if len(data) == 0:
                raise RuntimeError("errore connessione socket")
            line = struct.unpack(f"!{len(data)}s",data)[0]
            with open("capolet","w") as fifoL:
                print(f"scrivo su fifo capolet la linea {line}")
                fifoL.write(line)
            logging.debug(f"connessione tipo A, ricevuti {len(data)} bytes")
        elif client_t == b'1': # client connessione tipo B
            nLineTot  = 0
            while True:
                lenLine = recv_all(conn,2048)
                if len(lenLine) == 0:
                    break
                nLineTot += lenLine
                data = conn.recv(lenLine)
                line = struct.unpack(f"!{lenLine}s",data)[0]
                with open("caposc","w") as fifoS:
                    print(f"scrivo su fifo caposc la linea {line}")
                    fifoS.write(line)
            logging.debug(f"connessione tipo B, ricevuti {nLineTot} bytes")
            conn.sendall(struct.pack("!i",int(nLineTot)))

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

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=Description, formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("nthread",type=int,help="numero massimo di thread che il server deve utilizzare contemporanemente per la gestione dei client")
    parser.add_argument("-r",type=int,help="numero thread lettori",default=3)
    parser.add_argument("-w",type=int,help="numero thread scrittori",default=3)
    parser.add_argument("-v",help="chiamare il programma archivio mediante valgrind con opzioni valgrind --leak-check=full --show-leak-kinds=all --log-file=valgrind-%p.log")
    args = parser.parse_args()
    if args.v:
        p = subprocess.Popen(["valgrind","--leak-check=full", 
        "--show-leak-kinds=all", 
                    "--log-file=valgrind-%p.log", 
                    "archivio", f"{args.w}", f"{args.r}"])
    else:
        p = subprocess.Popen(["archivio", f"{args.w}", f"{args.r}"])

    main(nthreads=args.nthread)
