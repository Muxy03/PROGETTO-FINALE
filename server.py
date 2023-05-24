#! /usr/bin/env python3

import struct, socket,logging,os,argparse,subprocess,signal,concurrent.futures,threading,sys

def main(host,port,nthreads):
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
                    executor.submit(gestisci_connessione, conn,addr)
        except KeyboardInterrupt:
            pass
    print('Va bene smetto...')

def gestisci_connessione(conn,addr): 
  # L'uso di with serve solo a garantire che
  # conn venga chiusa all'uscita del blocco
  print(f"Contattato da {addr}")

  with conn:
    typecon = conn.recv(1).decode('utf-8')
    if typecon == "0":
        data = conn.recv(2048)
        linea = data.decode('utf-8')
        with open("capolet","a") as f:
            f.write(linea)
    elif typecon == "1":
       pass
    print("fine connessione")

def recv_all(conn,n):
  chunks = b''
  bytes_recd = 0
  while bytes_recd < n:
    chunk = conn.recv(min(n - bytes_recd, 2048))
    if len(chunk) == 0:
      raise RuntimeError("socket connection broken")
    chunks += chunk
    bytes_recd = bytes_recd + len(chunk)
  return chunks

if __name__ == '__main__':

    if(os.path.exists("./capolet")) == False:
        os.mkfifo("capolet")
    
    if(os.path.exists("./caposc")) == False:
        os.mkfifo("caposc")
    
    if(os.path.exists("./server.log")) == False:
        open("server.log", "x")

    logging.basicConfig(filename='./server.log',level=logging.DEBUG,filemode='w',datefmt='%d/%m/%y %H:%M:%S', format='%(asctime)s - %(levelname)s - %(message)s')
    Description = "Server"

    HOST = "127.0.0.1"
    PORT = 57943 # MAT:637943

    lock = threading.Lock()

    parser = argparse.ArgumentParser(description=Description, formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("-r",type=int,help="numero thread lettori",default=3)
    parser.add_argument("-w",type=int,help="numero thread scrittori",default=3)
    parser.add_argument("-v",help="chiamare il programma archivio mediante valgrind con opzioni valgrind --leak-check=full --show-leak-kinds=all --log-file=valgrind-%p.log",action='store_true')
    parser.add_argument("nthread",type=int,help="numero massimo di thread che il server deve utilizzare contemporanemente per la gestione dei client")
    args = parser.parse_args()
    if args.v:
        p = subprocess.Popen(["valgrind","--leak-check=full", 
        "--show-leak-kinds=all", 
                    "--log-file=valgrind-%p.log", 
                    "./archivio", f"{args.w}", f"{args.r}"])
    else:
        p = subprocess.Popen(["./archivio", f"{args.w}", f"{args.r}"])

    main(nthreads=args.nthread,host=HOST,port=PORT)
    
