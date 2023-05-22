#! /usr/bin/env python3

import struct, socket,logging,os,argparse,subprocess,signal,concurrent.futures

Description = "Server"
logging.basicConfig(filename='server.log',level=logging.DEBUG,  datefmt='%d/%m/%y %H:%M:%S', format='%(asctime)s - %(levelname)s - %(message)s')

HOST = "127.0.0.1"
PORT = "57943" # MAT:637943

def main(host=HOST,port=PORT,nthreads=0):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
        try:
        # permette di riutilizzare la porta se il server viene chiuso
            server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server.bind((host, port))
            server.listen()
            with concurrent.futures.ThreadPoolExecutor(max_workers=5) as executor:
                while True:
                    print("In attesa di un client...")
                    # mi metto in attesa di una connessione
                    conn, addr = server.accept()
                    # lavoro con la connessione appena ricevuta
                    # durante la gestione il server non accetta nuove connessioni
                    executor.submit(gestisci_connessione, conn,addr)
        # questa eccezione permette di chiudere il server
        except KeyboardInterrupt:
            server.shutdown(socket.SHUT_RDWR)
            os.unlink("capolet")
            os.unlink("caposc")
            p.send_signal(signal.SIGTERM)
        print('Va bene smetto...')
        # shutdown del server (la close viene fatta dalla with)  

def gestisci_connessione(conn,addr): 
  # L'uso di with serve solo a garantire che
  # conn venga chiusa all'uscita del blocco
  with conn:  
    totdseq = 0
    totb = 0
    print(f"Contattato da {addr}")
    typecon = conn.recv(1)
    if typecon == b'0':
        data = conn.recv(2048)
        assert(len(data) <= 2048)
        linea = struct.unpack(f"!{len(data)}s",data)[0]
        with open("capolet","a") as f:
            f.write(linea.decode()+'\n')
        logging.debug(f"Connessine di tipo A, scritti {len(data)} bytes in capolet\n")
    elif typecon == b'1':
        while conn.recv(2048) > 0:
            data = conn.recv(2048)
            assert(len(data) <= 2048)
            totdseq+=1
            totb+=len(data)
            linea = struct.unpack(f"!{len(data)}s",data)[0]
            with open("caposc","a") as f:
                f.write(linea.decode()+'\n')
        logging.debug(f"Connessine di tipo B, scritti {totb} bytes in caposc\n")
        
        conn.sendall(struct.pack("!i",totdseq))

if __name__ == '__main__':

    if(os.path.exists("capolet")) == False:
        os.mkfifo("capolet")
    
    if(os.path.exists("caposc")) == False:
        os.mkfifo("caposc")

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


# AGGIUNGERE THREAD