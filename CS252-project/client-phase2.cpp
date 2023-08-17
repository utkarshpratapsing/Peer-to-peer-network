#include <bits/stdc++.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
//#include <filesystem>
#include <dirent.h>
#include <algorithm>
using namespace std;
//namespace fs = std::filesystem;
#define TRUE   1 
#define FALSE  0 
//#define PORT 8888 

int filepresent(vector<string> p, string file) {
    for (int i = 0; i < p.size(); i++) {
        if (file == p[i]) {
            return 1;
        }
    }
    return 0;
}

string convToStr(char* a, int b, int e) {
    int i;
    string s = "";
    for (i = b; i < e; i++) {
        s = s + a[i];
    }
    return s;
}


class Neighbor {
public:
    int id;
    int unique_id = -1;
    int port_listening_on;
    bool connected = false;
    int ws;
    int ls;
    Neighbor(int val, int plo) {
        id = val;
        port_listening_on = plo;
    }
    int getid() {
        return id;
    }
    int getport() {
        return port_listening_on;
    }
    bool getstat() {
        return connected;
    }
};
//#define PORT "9034"   // port we're listening on

// get sockaddr, IPv4 or IPv6:
class File_s {
public:
    string name;
    int id = -1; //corresponding to smallest uid
    int uid = INT_MAX; //smallest uid
    int depth;
    File_s(string i) {
        name = i;
        depth = 0;
    }
};

//after accept we have uid passed if it actually has that file
//so what we will do is

//finally if

//}


int main(int argc, char const* argv[]) {
    int no_of_neighbors = 0;
    int self_port;
    int self_id;
    //const char t='A';
    //const char* self_port =&t;
    int self_uid;
    if (argc != 3) {
        cout << "Wrong input format\n";
        return 0;
    }

    const char* dir_path = argv[2];
    string config_file_path = argv[1];
    vector<string> files_names;


    DIR* dpdf;
    struct dirent* epdf;

    dpdf = opendir(dir_path);
    if (dpdf != NULL) {
        while (epdf = readdir(dpdf)) {
            files_names.push_back(epdf->d_name);

            //printf("Filename: %s",epdf->d_name);
            // std::cout << epdf->d_name << std::endl;
        }
    }
    closedir(dpdf);
    //for (const auto & entry : fs::directory_iterator(dir_path)){

    //}

    vector<Neighbor> neigh;

    ifstream config_file;
    config_file.open(config_file_path);

    vector<string> files_to_search;
    
    vector<File_s> fvec;

    if (config_file.is_open()) {
        int line_no = 0;
        string line;
        vector<string> words{};
        string space = " ";
        while (!config_file.eof()) {
            getline(config_file, line);
            if (line_no == 0) {
                istringstream iss(line);
                vector<string> tokens;
                copy(istream_iterator<string>(iss),
                    istream_iterator<string>(),
                    back_inserter(tokens));
                self_id = stoi(tokens[0]);
                self_port = stoi(tokens[1]);
                //const char * x=to_string(stoi(tokens[1])).c_str();
                //self_port=x;
                self_uid = stoi(tokens[2]);
            }
            if (line_no == 1) {
                no_of_neighbors = stoi(line);

            }
            if (line_no == 2) {
                istringstream iss(line);
                vector<string> tokens;
                copy(istream_iterator<string>(iss),
                    istream_iterator<string>(),
                    back_inserter(tokens));
                for (int i = 0; i < no_of_neighbors; i++) {
                    neigh.push_back(Neighbor(stoi(tokens[2 * i]), stoi(tokens[2 * i + 1])));
                }
            }

            if(line_no>3){
                istringstream iss(line);
                vector<string> tokens;
                copy(istream_iterator<string>(iss),
                    istream_iterator<string>(),
                    back_inserter(tokens));
               
                files_to_search.push_back(tokens[0]);
                //fvec.push_back(tokens[0]);
            }
            line_no++;

        }
        config_file.close();
    }
    sort(files_to_search.begin(), files_to_search.end());
    //for (int i = 0; i < files_names.size(); i++) {
    //    cout << files_names[i] << endl;
    //}
    for (int i = 0; i < files_to_search.size(); i++) {
        fvec.push_back(File_s(files_to_search[i]));
    }

    fd_set master; // master file descriptor list
    fd_set read_fds; // temp file descriptor list for select()
    struct sockaddr_in myaddr; // server address
    //struct sockaddr_in remoteaddr; // client address
    int fdmax; // maximum file descriptor number
    int listener; // listening socket descriptor
    int newfd; // newly accept()ed socket descriptor
    char buf[256]; // buffer for client data
    int nbytes;
    int yes = 1; // for setsockopt() SO_REUSEADDR, below
    int addrlen;
    int i, j;
    FD_ZERO(&master); // clear the master and temp sets
    FD_ZERO(&read_fds);
    // get the listener
    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    // lose the pesky "address already in use" error message
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    // bind
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = INADDR_ANY;//inet_addr("127.0.0.1");
    myaddr.sin_port = htons(self_port);

    
    memset(&(myaddr.sin_zero), '\0', 8);
    //memset(&(remoteaddr.sin_zero), '\0', 8);

    struct timeval tv;
    //fd_set readfds;
    tv.tv_sec = 2;
    tv.tv_usec = 500000;

    if (bind(listener, (struct sockaddr*)&myaddr, sizeof(myaddr)) == -1) {
        perror("bind");
        exit(1);
    }
    else { //cout << "bind complete" << endl;
    }
    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(1);
    }
    // add the listener to the master set
    FD_SET(listener, &master);
    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it’s this one
    // main loop
    int currid; // id of current connected neighbor
    int currport;
    int conn_counter = 0;
    int conn_acc = 0;


    while (true) {
        for (int i = 0; i < neigh.size(); i++) {
            if (neigh[i].connected == false) {
                //cout << "Andar ghusa for " << neigh[i].getid() << neigh[i].connected << endl;
                struct sockaddr_in remoteaddr;
                remoteaddr.sin_family = AF_INET;
                remoteaddr.sin_addr.s_addr = INADDR_ANY;//inet_addr("127.0.0.1");
                remoteaddr.sin_port = htons(neigh[i].port_listening_on);

                int fd = socket(AF_INET, SOCK_STREAM, 0);


                if (connect(fd, (struct sockaddr*)&remoteaddr, sizeof(remoteaddr)) < 0) {
                    //cout << "unable to connect to"<< neigh[i].getid() << endl;
                    close(fd);
                    continue;
                }
                else {
                    //cout << "Idhar ghus rha hai for neigh: "<<neigh[i].getid()<<neigh[i].connected<<endl;
                    neigh[i].connected = true;
                    neigh[i].ws = fd;
                    conn_counter++;
                    //cout << "Idhar rha hai for neigh: " << neigh[i].getid()<<neigh[i].connected << endl;
                    FD_SET(fd, &master); // add to master set
                    if (fd > fdmax) { // keep track of the maximum
                        fdmax = fd;
                    }
                    string sending_mes = to_string(self_id) + " " + to_string(self_uid);
                    if (send(fd, sending_mes.c_str(), strlen(sending_mes.c_str()), 0) == -1) {
                        close(fd);
                        perror("send");
                    }

                    
                    currid = neigh[i].getid();
                    currport = neigh[i].getport();
                    //send on fd 


                }

            }
            else {
                //cout << "True hai for i: " << neigh[i].getid()<<endl;
            }
        }


        read_fds = master; // copy it
        if (select(fdmax + 1, &read_fds, NULL, NULL, &tv) == -1) {
            perror("select");
            exit(1);
        }
        // run through the existing connections looking for data to read
        for (i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof(myaddr);
                    if ((newfd = accept(listener, (sockaddr*)&myaddr, (socklen_t*)&addrlen)) == -1) {
                        perror("accept");
                    }
                    else {

                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) { // keep track of the maximum
                            fdmax = newfd;
                        }
                        conn_acc++;
                        

                        if ((nbytes = recv(newfd, buf, sizeof(buf), 0)) <= 0) {
                            // got error or connection closed by client
                            if (nbytes == 0) {
                                
                            }
                            else {
                                perror("recv");
                            }
                            close(newfd); // bye!
                            FD_CLR(newfd, &master); // remove from master setr
                        }
                        else {
                            // we got some data from a client
                            string x = buf;
                            //char* token = strtok(x, "$");
                            int start = 0;
                            int end;
                            for (int j = 0; j < x.length(); j++) {
                                if (x[j] == ' ') {
                                    end = j;
                                }
                            }

                            // Keep printing tokens while one of the
                            // delimiters present in str[].
                            int w = stoi(x.substr(0, end));
                            for (int k = 0; k < neigh.size(); k++) {
                                if (neigh[k].getid() == w) {

                                    neigh[k].unique_id = stoi(x.substr(end + 1, x.length() - 1 - end));
                                    neigh[k].ls = newfd;
                                }
                            }
                            


                        }
                        
                    }
                }
                else {

                } // it’s SO UGLY!
            }
        }
        //cout << conn_counter;
        if (conn_counter == no_of_neighbors && conn_acc == no_of_neighbors) {
            break;
        }
    }
    //cout<<"Finished phase-1"<<endl;
    for (Neighbor i : neigh) {
        //cout << "Focusing on neighbour: " << i.getid() << endl;
        if (i.id > self_id) {
            //cout << "I am asking queries first\n";
            for (int x = 0; x < files_to_search.size(); x++) {
                if(send(i.ws, files_to_search[x].c_str(), strlen(files_to_search[x].c_str()), 0) == -1) {
                    
                    perror("send");
                }
                //cout << "sent query: " << files_to_search[x] << endl;
                int recbytes;
                if ((recbytes = recv(i.ls, buf, sizeof(buf), 0)) <= 0) {
                    // got error or connection closed by client
                    if (recbytes == 0) {
                        // connection closed
                        //printf("selectserver: socket %d hung up\n", i);
                    }
                    else {
                        perror("recv");
                    }
                    
                    
                }
                else {
                    // we got some data from a client
                    string t = convToStr(buf,0,recbytes);
                    //cout << "Received response " << t << endl;
                    if (t == "yes") {
                        fvec[x].depth = 1;
                        fvec[x].uid = min(fvec[x].uid, i.unique_id);
                        if (i.unique_id < fvec[x].uid) {
                            fvec[x].id = i.id;
                        }
                            
                    }


                }
            }
        
            string stop = "stop";
            if (send(i.ws, stop.c_str(), strlen(stop.c_str()), 0) == -1) {

                perror("send");
            }
            //cout << "Sent stop falg\n";
            while (true) {
                if ((nbytes = recv(i.ls, buf, sizeof(buf), 0)) <= 0) {
                    // got error or connection closed by client
                    if (nbytes == 0) {
                        // connection closed
                        //printf("selectserver: socket %d hung up\n", i);
                    }
                    else {
                        perror("recv");
                    }
                    
                }
                else {
                    // we got some data from a client
                    string x =convToStr(buf,0,nbytes);

                    //cout << "Received message: " << x << endl;//char* token = strtok(x, "$");
                    if (x == "stop") {
                        //cout << "Received stop flag\n";
                        break;

                    }
                    int present=filepresent( files_names, x);
                    if (present == 1) {
                        string yesno = "yes";
                        if (send(i.ws, yesno.c_str(), strlen(yesno.c_str()), 0) == -1) {

                            perror("send");
                        }


                    }
                    else {
                        string yesno = "no";
                        if (send(i.ws, yesno.c_str(), strlen(yesno.c_str()), 0) == -1) {

                            perror("send");
                        }

                    }
                    



                }
                //recv()
                    
                //search()
                //send yes or no

            }

        }
        else {
            //cout << "I am abswering queries now\n";
            while (true) {
                if ((nbytes = recv(i.ls, buf, sizeof(buf), 0)) <= 0) {
                    // got error or connection closed by client
                    if (nbytes == 0) {
                        // connection closed
                        //printf("selectserver: socket %d hung up\n", i);
                    }
                    else {
                        perror("recv");
                    }

                }
                else {
                    // we got some data from a client
                    string x = convToStr(buf, 0, nbytes);
                    //cout << "Received message: " << x << endl;
                    //char* token = strtok(x, "$");
                    if (x == "stop") {
                        //cout << "Received stop flag\n";
                        break;

                    }
                    int present = filepresent(files_names, x);
                    if (present == 1) {
                        string yesno = "yes";
                        if (send(i.ws, yesno.c_str(), strlen(yesno.c_str()), 0) == -1) {

                            perror("send");
                        }


                    }
                    else {
                        string yesno = "no";
                        if (send(i.ws, yesno.c_str(), strlen(yesno.c_str()), 0) == -1) {

                            perror("send");
                        }

                    }




                }
                //recv()

                //search()
                //send yes or no

            }

            

            for (int x = 0; x < files_to_search.size(); x++) {
                if (send(i.ws, files_to_search[x].c_str(), strlen(files_to_search[x].c_str()), 0) == -1) {

                    perror("send");
                }
                int recbytes;
                if ((recbytes = recv(i.ls, buf, sizeof(buf), 0)) <= 0) {
                    // got error or connection closed by client
                    if (recbytes == 0) {
                        // connection closed
                        //printf("selectserver: socket %d hung up\n", i);
                    }
                    else {
                        perror("recv");
                    }


                }
                else {
                    // we got some data from a client
                    string t = convToStr(buf, 0, recbytes);
                    if (t == "yes") {
                        fvec[x].depth = 1;
                        fvec[x].uid = min(fvec[x].uid, i.unique_id);
                        if (i.unique_id < fvec[x].uid) {
                            fvec[x].id = i.unique_id;
                        }

                    }


                }
            }
            string stop = "stop";
            if (send(i.ws, stop.c_str(), strlen(stop.c_str()), 0) == -1) {

                perror("send");
            }
        }
    }
    for (File_s i : fvec) {
        int p;
        if (i.uid == INT_MAX) {
            p = 0;
        }
        else {
            p = i.uid;
        }
        cout << "Found " << i.name << " at " << p << " with MD5 0 at depth " << i.depth<<endl;
    }
    return 0;
}
