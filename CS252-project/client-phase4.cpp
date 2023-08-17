#include <bits/stdc++.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

using namespace std;

class Neighbour
{
public:

    int id; //id of the neighbour
    int uid; //unique id of the neighbour
    int port; //port the Neighbour is listening on
    bool connected;
    int ws; //Writing socket
    int ls; //Listening socket

    Neighbour(int val, int port)
    {
        connected = false;
        id = val;
        this->port = port;
    }
};

struct File_s
{
    string filename;
    int owner_id;
    int owner_uid; //Smallest UID of the owner of the file
    int depth;
    bool requested; //true if requested and false if just informing that file is present
    File_s(string filename)
    {
        this->filename = filename;
        owner_id = -1;
        owner_uid = 0;
        depth = 0;
    }
};

string convToStr(char* a, int b, int e) {
    int i;
    string s = "";
    for (i = b; i < e; i++) {
        s = s + a[i];
    }
    return s;
}

int main(int argc, char* argv[])
{
    //Step 1: Reading command line arguments and getting list of files of which I am the owner
    assert(argc == 3);
    string config_filename = argv[1];
    const char* dir_path = argv[2];

    vector<string> my_files;

    DIR* dr;
    dirent* en;
    dr = opendir(dir_path);
    if (dr)
    {
        while ((en = readdir(dr)) != NULL)
        {
            string filename = en->d_name;
            if (filename != "." && filename != "..")
            {
                my_files.push_back(filename);
                // cout << "Adding to my_files: " << filename << endl; 
            }
        }
        closedir(dr);
    }
    sort(my_files.begin(), my_files.end());

    //Step 2: Processing Config file information
    int my_id;
    int my_uid;
    int my_port;
    unsigned int num_neighbours;

    unsigned int num_req_files;
    vector<File_s> required_files;

    vector<Neighbour> neighbours;
    ifstream config_file;
    config_file.open(config_filename);
    //Reading through the file
    unsigned int line_number = 0;
    if (config_file.is_open()) {
        unsigned int line_no = 0;
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
                my_id = stoi(tokens[0]);
                my_port = stoi(tokens[1]);
                my_uid = stoi(tokens[2]);
            }
            if (line_no == 1) {
                num_neighbours = stoi(line);

            }
            if (line_no == 2) {
                istringstream iss(line);
                vector<string> tokens;
                copy(istream_iterator<string>(iss),
                    istream_iterator<string>(),
                    back_inserter(tokens));
                for (int i = 0; i < num_neighbours; i++) {
                    neighbours.push_back(Neighbour(stoi(tokens[2 * i]), stoi(tokens[2 * i + 1])));
                }
            }

            if (line_no > 3) {
                istringstream iss(line);
                vector<string> tokens;
                copy(istream_iterator<string>(iss),
                    istream_iterator<string>(),
                    back_inserter(tokens));

                required_files.push_back(File_s(tokens[0]));
            }
            line_no++;
        }
        config_file.close();
    }
    //Step 3: Setting up socket connections
    fd_set master; // master file descriptor list
    fd_set read_fds; // temp file descriptor list for select()
    struct sockaddr_in myaddr; // server address
    int fdmax; // maximum file descriptor number
    int listener; // listening socket descriptor
    int newfd; // newly accept()ed socket descriptor
    char buf[1024]; // buffer for client data
    int nbytes;
    int yes = 1; // for setsockopt() SO_REUSEADDR, below
    int addrlen;
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
    myaddr.sin_port = htons(my_port);
    memset(&(myaddr.sin_zero), '\0', 8);

    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 500000;

    //bind
    if (bind(listener, (struct sockaddr*)&myaddr, sizeof(myaddr)) == -1) {
        perror("bind");
        exit(1);
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

    //Step 4: Main loop to get UID of the neighbours and neighbour files
    int currid; // id of current connected neighbor
    int currport;
    int conn_counter = 0;
    int conn_acc = 0;
    vector<File_s> neighbour_files; //list of files which my neighbours have

    while (true) {
        for (int i = 0; i < neighbours.size(); i++) {
            if (neighbours[i].connected == false) {
                //cout << "Andar ghusa for " << neigh[i].getid() << neigh[i].connected << endl;
                struct sockaddr_in remoteaddr;
                remoteaddr.sin_family = AF_INET;
                remoteaddr.sin_addr.s_addr = INADDR_ANY;//inet_addr("127.0.0.1");
                remoteaddr.sin_port = htons(neighbours[i].port);

                int fd = socket(AF_INET, SOCK_STREAM, 0);


                if (connect(fd, (struct sockaddr*)&remoteaddr, sizeof(remoteaddr)) < 0) {
                    //cout << "unable to connect to"<< neigh[i].getid() << endl;
                    close(fd);
                    continue;
                }
                else {
                    //cout << "Idhar ghus rha hai for neigh: "<<neigh[i].getid()<<neigh[i].connected<<endl;
                    neighbours[i].connected = true;
                    neighbours[i].ws = fd;
                    conn_counter++;
                    //cout << "Idhar rha hai for neigh: " << neigh[i].getid()<<neigh[i].connected << endl;
                    FD_SET(fd, &master); // add to master set
                    if (fd > fdmax) { // keep track of the maximum
                        fdmax = fd;
                    }
                    string sending_mes = to_string(my_id) + " " + to_string(my_uid);
                    if (send(fd, sending_mes.c_str(), strlen(sending_mes.c_str()), 0) == -1) {
                        close(fd);
                        perror("send");
                    }


                    currid = neighbours[i].id;
                    currport = neighbours[i].port;
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
        for (int i = 0; i <= fdmax; i++) {
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
                            for (int k = 0; k < neighbours.size(); k++) {
                                if (neighbours[k].id == w) {

                                    neighbours[k].uid = stoi(x.substr(end + 1, x.length() - 1 - end));
                                    neighbours[k].ls = newfd;
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
        if (conn_counter == num_neighbours && conn_acc == num_neighbours) {
            break;
        }
    }
    // cout << "connections done" << endl;
    cout << flush;
    for (Neighbour i : neighbours) {
        // cout << "Focusing on neighbour: " << i.id << endl;
        cout << flush;
        if (i.id > my_id) {
            string send_message = "";
            for (int j = 0; j < my_files.size() - 1; j++)
            {
                send_message += my_files[j] + "$";
            }
            send_message += my_files[my_files.size() - 1] + "*";
            for (int j = 0; j < required_files.size(); ++j)
            {
                send_message += required_files[j].filename + "$";
            }
            if (send(i.ws, send_message.c_str(), strlen(send_message.c_str()), 0) == -1)
            {
                perror("Send");
            }
            // cout << "Sending message " << send_message << endl;
            if ((nbytes = recv(i.ls, buf, sizeof(buf), 0)) <= 0) {
                if (nbytes < 0) {
                    perror("recv");
                }
            }
            else {
                string x = convToStr(buf, 0, nbytes);
                // cout << "Received message: \t" << x << " from " << i.id << endl;
                int word_start = 0;
                bool star_seen = false;
                for (int k = 0; k < x.length(); ++k)
                {
                    if (x[k] == '*')
                    {
                        neighbour_files.push_back(File_s(x.substr(word_start, k - word_start)));
                        neighbour_files[neighbour_files.size() - 1].owner_id = i.id;
                        neighbour_files[neighbour_files.size() - 1].owner_uid = i.uid;
                        neighbour_files[neighbour_files.size() - 1].requested = false;
                        // cout << "Pushed back neighbour file " << neighbour_files[neighbour_files.size() - 1].filename << " with owner ID " << neighbour_files[neighbour_files.size() - 1].owner_id << " requested status " << neighbour_files[neighbour_files.size() - 1].requested << endl;
                        word_start = k + 1;
                        star_seen = true;
                    }
                    else if (x[k] == '$')
                    {
                        neighbour_files.push_back(File_s(x.substr(word_start, k - word_start)));
                        neighbour_files[neighbour_files.size() - 1].owner_id = i.id;
                        neighbour_files[neighbour_files.size() - 1].owner_uid = i.uid;
                        if (star_seen)
                        {
                            neighbour_files[neighbour_files.size() - 1].requested = true;
                        }
                        else
                        {
                            neighbour_files[neighbour_files.size() - 1].requested = false;
                        }
                        // cout << "Pushed back neighbour file " << neighbour_files[neighbour_files.size() - 1].filename << " with owner ID " << neighbour_files[neighbour_files.size() - 1].owner_id << " requested status " << neighbour_files[neighbour_files.size() - 1].requested << endl;
                        word_start = k + 1;
                    }
                }
            }

        }
        else {

            if ((nbytes = recv(i.ls, buf, sizeof(buf), 0)) <= 0) {
                if (nbytes < 0) {
                    perror("recv");
                }
            }
            else {
                string x = convToStr(buf, 0, nbytes);
                // cout << "Received message: \t" << x << " from " << i.id << endl;
                int word_start = 0;
                bool star_seen = false;
                for (int k = 0; k < x.length(); ++k)
                {
                    if (x[k] == '*')
                    {
                        neighbour_files.push_back(File_s(x.substr(word_start, k - word_start)));
                        neighbour_files[neighbour_files.size() - 1].owner_id = i.id;
                        neighbour_files[neighbour_files.size() - 1].owner_uid = i.uid;
                        neighbour_files[neighbour_files.size() - 1].requested = false;
                        // cout << "Pushed back neighbour file " << neighbour_files[neighbour_files.size() - 1].filename << " with owner ID " << neighbour_files[neighbour_files.size() - 1].owner_id << " requested status " << neighbour_files[neighbour_files.size() - 1].requested << endl;
                        word_start = k + 1;
                        star_seen = true;
                    }
                    else if (x[k] == '$')
                    {
                        neighbour_files.push_back(File_s(x.substr(word_start, k - word_start)));
                        neighbour_files[neighbour_files.size() - 1].owner_id = i.id;
                        neighbour_files[neighbour_files.size() - 1].owner_uid = i.uid;
                        if (star_seen)
                        {
                            neighbour_files[neighbour_files.size() - 1].requested = true;
                        }
                        else
                        {
                            neighbour_files[neighbour_files.size() - 1].requested = false;
                        }
                        // cout << "Pushed back neighbour file " << neighbour_files[neighbour_files.size() - 1].filename << " with owner ID " << neighbour_files[neighbour_files.size() - 1].owner_id << " requested status " << neighbour_files[neighbour_files.size() - 1].requested << endl;
                        word_start = k + 1;
                    }
                }
            }
            string send_message = "";
            for (int j = 0; j < my_files.size() - 1; j++)
            {
                send_message += my_files[j] + "$";
            }
            send_message += my_files[my_files.size() - 1] + "*";
            for (int j = 0; j < required_files.size(); ++j)
            {
                send_message += required_files[j].filename + "$";
            }
            if (send(i.ws, send_message.c_str(), strlen(send_message.c_str()), 0) == -1)
            {
                perror("Send");
            }
            // cout << "Sending message " << send_message << endl;
        }
    }
    // cout << "Step 4 completed" << endl;
    cout << flush;

    //Step 5: Sending messages for final answer
    for (Neighbour i : neighbours) {
        //cout << "Focusing on neighbour: " << i.getid() << endl;
        if (i.id > my_id) {
            string sending_message = "";
            for (File_s req_file : neighbour_files)
            {
                bool found = false;
                // cout << "Checking for file " << req_file.filename << endl;
                if (req_file.requested && req_file.owner_id == i.id)
                {
                    for (File_s sent_file : my_files)
                    {
                        if (req_file.filename == sent_file.filename)
                        {
                            sending_message += req_file.filename + " " + to_string(my_uid) + " " + "1 *";
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        for (File_s sent_file : neighbour_files)
                        {
                            if (req_file.filename == sent_file.filename && sent_file.requested == false)
                            {
                                sending_message += req_file.filename + " " + to_string(sent_file.owner_uid) + " " + "2 *";
                                found = true;
                                break;
                            }
                        }
                    }
                    if (!found)
                    {
                        sending_message += "$ " + req_file.filename + "*";
                        // cout << "Could not find " << req_file.filename << " for " << req_file.owner_id << endl; 
                    }
                }
            }
            if (sending_message != "")
            {
                if (send(i.ws, sending_message.c_str(), strlen(sending_message.c_str()), 0) == -1)
                {
                    perror("Send");
                }
            }
            // cout << "Sending message\t" << sending_message << " to " << i.id << endl;
            if ((nbytes = recv(i.ls, buf, sizeof(buf), 0)) <= 0) {
                if (nbytes < 0) {
                    perror("recv");
                }
            }
            else {
                string x = convToStr(buf, 0, nbytes);
                // cout << "Received message: \t" << x << endl;
                // int word_start = 0;
                // int count = 0;
                // if(x[0] != '$')
                // {
                //     string ack_filename;
                //     int ack_uid;
                //     int ack_depth;
                //     for(int k = 0; k < x.length(); ++k)
                //     {
                //         if(x[k] == ' ')
                //         {
                //             if(count == 0)
                //             {
                //                 ack_filename = x.substr(word_start, k - word_start);
                //                 cout << "Received filename is " << ack_filename << endl;
                //             }
                //             else if (count == 1)
                //             {
                //                 ack_uid = stoi(x.substr(word_start, k - word_start));
                //                 cout << "Received file owner UID is " << ack_uid << endl;
                //             }
                //             else
                //             {
                //                 ack_depth = stoi(x.substr(word_start, k - word_start));
                //                 cout << "Received file depth is " << ack_depth << endl;
                //             }
                //             word_start = k + 1;
                //             count++;
                //         }
                //     }
                //     for(int k = 0; k < required_files.size(); k++)
                //     {
                //         if(required_files[k].filename == ack_filename)
                //         {
                //             if(required_files[k].depth == 0)
                //             {
                //                 required_files[k].depth = ack_depth;
                //                 required_files[k].owner_uid = ack_uid;
                //             }
                //             else if(required_files[k].depth == 1)
                //             {
                //                 if(ack_depth == 1)
                //                 {
                //                     required_files[k].owner_uid = min(required_files[k].owner_uid, ack_uid);
                //                 }
                //             }
                //             else
                //             {
                //                 if(ack_depth == 1)
                //                 {
                //                     required_files[k].depth = ack_depth;
                //                     required_files[k].owner_uid = ack_uid;
                //                 }
                //                 else
                //                 {
                //                     required_files[k].owner_uid = min(required_files[k].owner_uid, ack_uid);
                //                 }
                //             }
                //         }
                //     }
                // }
                int msg_start = 0;
                for (int k = 0; k < x.length(); k++)
                {
                    if (x[k] == '*')
                    {
                        // cout << "Found * at position " << k << " and msg_start = " << msg_start << endl;
                        // cout << "Processing " << x.substr(msg_start, k - msg_start) << endl;
                        if (x[msg_start] != '$')
                        {
                            int count = 0;
                            int word_start = msg_start;
                            string ack_filename;
                            int ack_uid;
                            int ack_depth;
                            for (int l = msg_start; l < k; l++)
                            {
                                if (x[l] == ' ') {
                                    if (count == 0)
                                    {
                                        ack_filename = x.substr(word_start, l - word_start);
                                        // cout << "Received filename is " << ack_filename << endl;
                                    }
                                    else if (count == 1)
                                    {
                                        ack_uid = stoi(x.substr(word_start, l - word_start));
                                        // cout << "Received file owner UID is " << ack_uid << endl;
                                    }
                                    else
                                    {
                                        ack_depth = stoi(x.substr(word_start, l - word_start));
                                        // cout << "Received file depth is " << ack_depth << endl;
                                    }
                                    word_start = l + 1;
                                    count++;
                                }
                            }
                            for (int l = 0; l < required_files.size(); l++)
                            {
                                if (required_files[l].filename == ack_filename)
                                {
                                    if (required_files[l].depth == 0)
                                    {
                                        required_files[l].depth = ack_depth;
                                        required_files[l].owner_uid = ack_uid;
                                    }
                                    else if (required_files[l].depth == 1)
                                    {
                                        if (ack_depth == 1)
                                        {
                                            required_files[l].owner_uid = min(required_files[l].owner_uid, ack_uid);
                                        }
                                    }
                                    else
                                    {
                                        if (ack_depth == 1)
                                        {
                                            required_files[l].depth = ack_depth;
                                            required_files[l].owner_uid = ack_uid;
                                        }
                                        else
                                        {
                                            required_files[l].owner_uid = min(required_files[l].owner_uid, ack_uid);
                                        }
                                    }
                                }
                            }
                        }
                        msg_start = k + 1;
                    }
                }

            }

        }
        else {
            if ((nbytes = recv(i.ls, buf, sizeof(buf), 0)) <= 0) {
                if (nbytes < 0) {
                    perror("recv");
                }
            }
            else {
                string x = convToStr(buf, 0, nbytes);
                // cout << "Received message: \t" << x << endl;
                // int word_start = 0;
                // int count = 0;
                // if(x[0] != '$')
                // {
                //     string ack_filename;
                //     int ack_uid;
                //     int ack_depth;
                //     for(int k = 0; k < x.length(); ++k)
                //     {
                //         if(x[k] == ' ')
                //         {
                //             if(count == 0)
                //             {
                //                 ack_filename = x.substr(word_start, k - word_start);
                //                 cout << "Received filename is " << ack_filename << endl;
                //             }
                //             else if (count == 1)
                //             {
                //                 ack_uid = stoi(x.substr(word_start, k - word_start));
                //                 cout << "Received file owner UID is " << ack_uid << endl;
                //             }
                //             else
                //             {
                //                 ack_depth = stoi(x.substr(word_start, k - word_start));
                //                 cout << "Received file depth is " << ack_depth << endl;
                //             }
                //             word_start = k + 1;
                //             count++;
                //         }
                //     }
                //     for(int k = 0; k < required_files.size(); k++)
                //     {
                //         if(required_files[k].filename == ack_filename)
                //         {
                //             if(required_files[k].depth == 0)
                //             {
                //                 required_files[k].depth = ack_depth;
                //                 required_files[k].owner_uid = ack_uid;
                //             }
                //             else if(required_files[k].depth == 1)
                //             {
                //                 if(ack_depth == 1)
                //                 {
                //                     required_files[k].owner_uid = min(required_files[k].owner_uid, ack_uid);
                //                 }
                //             }
                //             else
                //             {
                //                 if(ack_depth == 1)
                //                 {
                //                     required_files[k].depth = ack_depth;
                //                     required_files[k].owner_uid = ack_uid;
                //                 }
                //                 else
                //                 {
                //                     required_files[k].owner_uid = min(required_files[k].owner_uid, ack_uid);
                //                 }
                //             }
                //         }
                //     }
                // }
                int msg_start = 0;
                for (int k = 0; k < x.length(); k++)
                {
                    if (x[k] == '*')
                    {
                        // cout << "Found * at position " << k << " and msg_start = " << msg_start << endl;
                        // cout << "Processing " << x.substr(msg_start, k - msg_start) << endl;
                        if (x[msg_start] != '$')
                        {
                            int count = 0;
                            int word_start = msg_start;
                            string ack_filename;
                            int ack_uid;
                            int ack_depth;
                            for (int l = msg_start; l < k; l++)
                            {
                                if (x[l] == ' ') {
                                    if (count == 0)
                                    {
                                        ack_filename = x.substr(word_start, l - word_start);
                                        // cout << "Received filename is " << ack_filename << endl;
                                    }
                                    else if (count == 1)
                                    {
                                        ack_uid = stoi(x.substr(word_start, l - word_start));
                                        // cout << "Received file owner UID is " << ack_uid << endl;
                                    }
                                    else
                                    {
                                        ack_depth = stoi(x.substr(word_start, l - word_start));
                                        // cout << "Received file depth is " << ack_depth << endl;
                                    }
                                    word_start = l + 1;
                                    count++;
                                }
                            }
                            for (int l = 0; l < required_files.size(); l++)
                            {
                                if (required_files[l].filename == ack_filename)
                                {
                                    if (required_files[l].depth == 0)
                                    {
                                        required_files[l].depth = ack_depth;
                                        required_files[l].owner_uid = ack_uid;
                                    }
                                    else if (required_files[l].depth == 1)
                                    {
                                        if (ack_depth == 1)
                                        {
                                            required_files[l].owner_uid = min(required_files[l].owner_uid, ack_uid);
                                        }
                                    }
                                    else
                                    {
                                        if (ack_depth == 1)
                                        {
                                            required_files[l].depth = ack_depth;
                                            required_files[l].owner_uid = ack_uid;
                                        }
                                        else
                                        {
                                            required_files[l].owner_uid = min(required_files[l].owner_uid, ack_uid);
                                        }
                                    }
                                }
                            }
                        }
                        msg_start = k + 1;
                    }
                }
            }
            string sending_message = "";

            for (File_s req_file : neighbour_files)
            {
                bool found = false;
                if (req_file.requested && req_file.owner_id == i.id)
                {
                    // cout << "Checking for file " << req_file.filename << endl;
                    for (File_s sent_file : my_files)
                    {
                        if (req_file.filename == sent_file.filename)
                        {
                            // cout << "My id is " << my_id << " neighbour id is " << i.id << " the filename I m sending is " << req_file.filename << endl;
                            sending_message += req_file.filename + " " + to_string(my_uid) + " " + "1 *";
                            found = true;
                        }
                    }
                    if (!found)
                    {
                        for (File_s sent_file : neighbour_files)
                        {
                            if (req_file.filename == sent_file.filename && sent_file.requested == false)
                            {
                                sending_message += req_file.filename + " " + to_string(sent_file.owner_uid) + " " + "2 *";
                                found = true;
                            }
                        }
                    }
                    // cout << "Value of found for " << req_file.filename << " = " << found << endl;
                    if (!found)
                    {
                        sending_message += "$ " + req_file.filename + "*";
                        // cout << "Could not find " << req_file.filename << " for " << req_file.owner_id << endl; 
                    }
                }
            }
            if (sending_message != "")
            {
                if (send(i.ws, sending_message.c_str(), strlen(sending_message.c_str()), 0) == -1)
                {
                    perror("Send");
                }
            }
            // cout << "Sending message\t" << sending_message << " to " << i.id << endl;
        }
    }
    // cout << "Step 5 completed " << endl;

    //Step 6: Printing the final information
    for (File_s file : required_files)
    {
        cout << "Found " << file.filename << " at " << file.owner_uid << " with MD5 0 at depth " << file.depth << endl;
    }

    // cout << "\n\nPrinting neighbour files for my id: " << my_id << "\n\n";
    // for(int i = 0; i < neighbour_files.size(); i++)
    // {
    //     cout << "Filename " << neighbour_files[i].filename << " File owner ID " << neighbour_files[i].owner_id << " File owner UID " << neighbour_files[i].owner_uid << " File requested or not " << neighbour_files[i].requested << endl;
    // }


    return 0;
}
