#! /usr/bin/env python3

import socket, concurrent.futures, argparse, os, subprocess, logging, signal, struct

HOST = "127.0.0.1"
PORT = 57943 #637943

def receive_data(conn):
    data_len_bytes = conn.recv(4)
    data_len = sorted(data_len_bytes,key=lambda b:b==0)[0]
    data = conn.recv(data_len)
    if data.decode("utf-8") == "":
        return None
    else:
        return (data_len,data)

def main(max_threads):    
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            try:
                s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s.bind((HOST, PORT))
                s.listen()
                with concurrent.futures.ThreadPoolExecutor(max_workers=max_threads) as executor:
                    while True:
                        conn, _ = s.accept()
                        executor.submit(gestisci_connessione, conn)
            except KeyboardInterrupt:
                s.shutdown(socket.SHUT_RDWR)
                print("Chiusura server in corso...")
                p.send_signal(signal.SIGTERM)
                p.wait()
                os.unlink("capolet")
                os.unlink("caposc")
                print("Server chiuso")

def gestisci_connessione(conn):

    tipo = conn.recv(1).decode()
    totaleb = 0

    if tipo == "0":
        parola = receive_data(conn)
        if parola is not None:
            with open("capolet", "w") as fifo:
                st="0"*(4 - len(str(parola[0]))) + str(parola[0])
                fifo.write(st)
                fifo.write(parola[1].decode('utf-8'))
                totaleb += len(parola[1])

    elif tipo == "1":
        
        totales = 0
        with open("caposc", "w") as fifo:
            while True:
                parola = receive_data(conn)
                if parola is None:
                    break
                else:
                    st="0"*(4 - len(str(parola[0]))) + str(parola[0])
                    fifo.write(st)
                    fifo.write(parola[1].decode('utf-8'))
                    totales+=1
                    totaleb += len(parola[1])

        conn.send(struct.pack('i', totales))
        
    print("chiudo connessione")

    logging.debug("Client con connessione di tipo %s ha scritto %d bytes", tipo, totaleb)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Server", formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("t", default=1, type=int, help="Threads")
    parser.add_argument("-w", type=int, default=3, help="Writers")
    parser.add_argument("-r", type=int, default=3, help="Readers")
    parser.add_argument("-v", action="store_true", help="Valgrind")
    args = parser.parse_args()

    logging.basicConfig(filename="server.log",level=logging.DEBUG, datefmt='%d/%m/%y %H:%M:%S',format='%(asctime)s - %(levelname)s - %(message)s')

    if not os.path.exists("caposc"):
        os.mkfifo("caposc")

    if not os.path.exists("capolet"):
        os.mkfifo("capolet")

    with open("lettori.log","w") as f:
        pass

    if args.v :
        p = subprocess.Popen(["valgrind","--leak-check=full","--show-leak-kinds=all","--log-file=valgrind-%p.log","./archivio",f"{args.w}",f"{args.r}"])
    else :
        p = subprocess.Popen(["./archivio",f"{args.w}",f"{args.r}"])
    
    main(args.t)
    
   
