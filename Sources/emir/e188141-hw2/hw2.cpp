#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string>
#include <cstdio>
#include <errno.h>
#include <vector>
#include <sstream>

using namespace std;

#define ADDRESS     "mysocket"
#define MAXSIZE 100000

pthread_mutex_t sendLock;

class RegionWatch{
	public:
		pthread_cond_t condLock;
		pthread_condattr_t attrcond;
		double x;
		double y;
		double w;
		double h;
		double rx, ry, rw, rh;
		int rtype, risLock;
		int owner;
		int id;
		bool active;

		void initCond() {
			pthread_condattr_init(&attrcond);
			pthread_condattr_setpshared(&attrcond, PTHREAD_PROCESS_SHARED);
			pthread_cond_init(&condLock, &attrcond);
			active = false;
		};
		RegionWatch(int id, int owner, double x, double y, double w, double h) {
			this->id = id;
			this->owner = owner;
			this->x = x;
			this->y = y;
			this->w = w;
			this->h = h;
		};

};

class RegionLock {
	public:
		pthread_cond_t condLock;
		pthread_condattr_t attrcond;
		double x;
		double y;
		double w;
		double h;
		int owner;
		int id;
		int type;
		bool active;

		void initCond() {
			pthread_condattr_init(&attrcond);
			pthread_condattr_setpshared(&attrcond, PTHREAD_PROCESS_SHARED);
			pthread_cond_init(&condLock, &attrcond);
			active = false;
		};

		RegionLock() {
			
			pthread_condattr_init(&attrcond);
			pthread_condattr_setpshared(&attrcond, PTHREAD_PROCESS_SHARED);
			pthread_cond_init(&condLock, &attrcond);
			active = false;
			
		};
		
		RegionLock(int owner, int id, int type, double x, double y, double w, double h) {
			
			pthread_condattr_init(&attrcond);
			pthread_condattr_setpshared(&attrcond, PTHREAD_PROCESS_SHARED);
			pthread_cond_init(&condLock, &attrcond);
			active = false;
			this->owner = owner;
			this->type = type;
			this->id = id;
			this->x = x;
			this->y = y;
			this->w = w;
			this->h = h;
		};
		RegionLock operator=(const RegionLock& other) {
			
			owner = other.owner;
			type = other.type;
			id = other.id;
			x = other.x;
			y = other.y;
			w = other.w;
			h = other.h;
			active = other.active;
		};
		bool isIntersecting(RegionLock r) {
			double xmax = x+w;
			double ymax = y+h;
			double rxmin = r.x; double rxmax = r.x + r.w;
			double rymin = r.y; double rymax = r.y + r.h;
			if ((	(rxmin >= x 	&& rxmin <= xmax) ||
					(rxmin <= x 	&& rxmax >= xmax) ||
					(rxmax >= x 	&& rxmax <= xmax) ) && 
				(	(rymin >= y 	&& rymin <= ymax) ||
					(rymin <= y 	&& rymax >= ymax) ||
					(rymax >= y 	&& rymax <= ymax) ))
				return true;
			else
				return false;
		};
		bool isBlocking(RegionLock r) {
			double xmax = x+w;
			double ymax = y+h;
			double rxmin = r.x; double rxmax = r.x + r.w;
			double rymin = r.y; double rymax = r.y + r.h;
			if (isIntersecting(r)) {
				if (type == 1)
					return true;
				else if (r.type == 1)
					return true;
				else
					return false;
			} else
				return false;
		};
		void setActive() {
			active = true;
		}
		void setDeactive() {
			active = false;
		}
		bool isActive() {
			return active;
		}
};

class Environment{
	public:

		pthread_mutex_t envlock;
		pthread_mutexattr_t attrmutex;
		int size;
		int nextOwner;
		int nextLockId;
		RegionLock regions[];
		
		Environment() {
		};
		
		void initVariables() {
			size = 0; 
			nextOwner = 1;
			nextLockId = 1;
			pthread_mutexattr_init(&attrmutex);
			pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);
			pthread_mutex_init(&envlock, &attrmutex);
			for (int i=0; i<MAXSIZE; i++){
				regions[i].initCond();
			}
		};

		int getFirstAvailableInd() {
			for (int i = 0; i<MAXSIZE; i++){
				if (!regions[i].isActive())
					return i;
			}
		}
};

class Northwatch {
	public:

		pthread_mutex_t wlock;
		pthread_mutexattr_t attrw;
		int size;
		RegionWatch watches[];

		Northwatch() {};
		void initVariables() {
			size = 0;
			pthread_mutexattr_init(&attrw);
			pthread_mutexattr_setpshared(&attrw, PTHREAD_PROCESS_SHARED);
			pthread_mutex_init(&wlock, &attrw);
			for (int i=0; i<MAXSIZE; i++){
				watches[i].initCond();
			}
		};
		int getFirstAvailableInd() {
			for (int i = 0; i<MAXSIZE; i++){
				if (!watches[i].active)
					return i;
			}
		}
};


vector<string> split(const string &s, char delim) {
    vector<string> elems;
    //split(s, delim, elems);
    stringstream ss;
    ss.str(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

void *watchRegion(void *threadid) {
   long tid;
   tid = (long)threadid;
   cout << "Hello World! Thread ID, " << tid << endl;
   pthread_exit(NULL);
}

string iToS(int i) {
	stringstream c;
	c << i;
	return c.str();
}

string dToS(double d) {
	stringstream c;
	c << d;
	return c.str();
}

vector<RegionLock> sortVector(bool withID, vector<RegionLock> &regs){
	vector<RegionLock> result;
	if (withID) {
		while (regs.size() != 0) {
			int min = -1, ind = -1;
			for (int i=0; i<regs.size(); i++) {
				if (min < 0 || regs[i].id < min) {
					min = regs[i].id;
					ind = i;
				}
			}
			result.push_back(regs[ind]);
			regs.erase(regs.begin() + ind);
		}
	} else {
		while (regs.size() != 0) {
			int ind = -1, mint = -1;
			double minx = -1.0, miny = -1.0, minw = -1.0, minh = -1.0;
			for (int i=0; i<regs.size(); i++) {
				if (	ind < 0 || 
						regs[i].x < minx || 
						(regs[i].x == minx && regs[i].y < miny) ||
						(regs[i].x == minx && regs[i].y == miny && regs[i].w < minw) ||
						(regs[i].x == minx && regs[i].y == miny && regs[i].w == minw && regs[i].h < minh) ||
						(regs[i].x == minx && regs[i].y == miny && regs[i].w == minw && regs[i].h == minh && regs[i].type < mint)
					) {
					ind = i;
					mint = regs[i].type; minx = regs[i].x; miny = regs[i].y; minw = regs[i].w; minh = regs[i].h;
				}
				
			}
			result.push_back(regs[ind]);
			regs.erase(regs.begin() + ind);
		}
	}
	return result;
}



void sendMessage(int &s, string m) {
	
	char *cstr = new char[m.length() + 2];
	strcpy (cstr, m.c_str());
	cstr[m.length()] = '\n';
	cstr[m.length()+1] = '\0';
	pthread_mutex_lock(&sendLock);
	send(s, cstr, strlen(cstr), 0);
	pthread_mutex_unlock(&sendLock);
}

void sendMessageForGetlocks(int &s, vector<RegionLock> v) {
	for (int i = 0; i<v.size(); i++){
		string m = "";
		m = m + (v[i].type ? "W" : "R") + " " + dToS(v[i].x) + " " + dToS(v[i].y) + " " + dToS(v[i].w) + " " + dToS(v[i].h);
		sendMessage(s, m);
	}
}

void sendMessageForMylocks(int &s, vector<RegionLock> v) {
	for (int i = 0; i<v.size(); i++){
		string m = "";
		m = m + iToS(v[i].id) + " " + (v[i].type ? "W" : "R") + " " + dToS(v[i].x) + " " + dToS(v[i].y) + " " + dToS(v[i].w) + " " + dToS(v[i].h);
		sendMessage(s, m);
	}
}

/*void sendMessageForWatches(int &s, vector<RegionWatch> v) {
	for (int i = 0; i<v.size(); i++){
		string m = "";
		m = m + iToS(v[i].id) + " " + dToS(v[i].x) + " " + dToS(v[i].y) + " " + dToS(v[i].w) + " " + dToS(v[i].h);
		sendMessage(s, m);
	}
}*/

bool doesIntersect(RegionWatch w, RegionLock l) {
	double xmin = w.x; double xmax = w.x+w.w;
	double ymin = w.y; double ymax = w.y+w.h;
	double rxmin = l.x; double rxmax = l.x + l.w;
	double rymin = l.y; double rymax = l.y + l.h;
	if ((	(rxmin >= xmin 	&& rxmin <= xmax) ||
			(rxmin <= xmin 	&& rxmax >= xmax) ||
			(rxmax >= xmin 	&& rxmax <= xmax) ) && 
		(	(rymin >= ymin 	&& rymin <= ymax) ||
			(rymin <= ymin 	&& rymax >= ymax) ||
			(rymax >= ymin 	&& rymax <= ymax) ))
		return true;
	else
		return false;
}

/*void checkWatches(vector<RegionWatch> *vec, RegionLock r, int &s, bool isLock) {

	for (int i=0; i<vec->size(); i++){
		if (doesIntersect(vec[0][i], r)) {
			sendMessage(s, "Watch " + iToS(vec[0][i].id) + " " + dToS(r.x) + " " + dToS(r.y) + " " + dToS(r.w) + " " + dToS(r.h) + " " + (r.type ? "W" : "R") + " " + (isLock ? "locked" : "unlocked"));
		}
	}
}*/

void checkWatchesRevise(Northwatch* nw, RegionLock r, int &s, bool isLock){
	
	int index = 0, i = 0;
	
	pthread_mutex_lock(&nw->wlock);
	while (i < nw->size) {
		if (nw->watches[index].active){
			if (doesIntersect(nw->watches[index], r) ){
				nw->watches[index].rx = r.x;
				nw->watches[index].ry = r.y;
				nw->watches[index].rw = r.w;
				nw->watches[index].rh = r.h;
				nw->watches[index].rtype = r.type;
				if (isLock)
					nw->watches[index].risLock = 1;
				else
					nw->watches[index].risLock = 0;
				pthread_cond_broadcast(&nw->watches[index].condLock);
			}
			i++;
			index++;
		}
		else{
			index++;
		}
	}
	pthread_mutex_unlock(&nw->wlock);

}

struct passVal {
	int index;
	int socket;
	Northwatch* p;
};

void *ListenRegion(void *sn) {
	struct passVal *ptr = (struct passVal *)sn;
	int index = ptr->index;
	int socket = ptr->socket;
	Northwatch* p = ptr->p;
	//cout << "I'm listening: " << index << " fai: " << p->getFirstAvailableInd() << " inx: " << p->watches[index].w << endl;

	while (p->watches[index].active) {
		pthread_mutex_lock(&p->wlock);
		pthread_cond_wait(&p->watches[index].condLock, &p->wlock);
		if (!p->watches[index].active){
			pthread_mutex_unlock(&p->wlock);
			break;
		}
		sendMessage(socket, "Watch " + iToS(p->watches[index].id) + " " + dToS(p->watches[index].rx) + " " + dToS(p->watches[index].ry) + " " + dToS(p->watches[index].rw) + " " + dToS(p->watches[index].rh) + " " + (p->watches[index].rtype ? "W" : "R") + " " + (p->watches[index].risLock ? "locked" : "unlocked"));

		pthread_mutex_unlock(&p->wlock);
	}

	
};

int main (int argc, char* argv[]) {

	if (argc != 2) {
		
		cout << "usage: " << argv[0] << " <socketpath>" << endl;
		return 0;
	}

	pthread_mutex_init(&sendLock, NULL);

	char* socketpath = argv[1];

	// Shared Memory
	int key = 9999;
	int segment_id;

	segment_id = shmget(key, sizeof(Environment) + (MAXSIZE * sizeof(RegionLock)) , IPC_CREAT | 0775);
	
	if (segment_id == -1) {
		cout << "Error shmget : " << errno << endl;

		exit(1);
	}
	//else
	//	cout << "Segment id is : " << segment_id << endl;
	// * Shared memory

	// Shared Memory for watches
	int wkey = 9998;
	int segment_id_watch;

	segment_id_watch = shmget(wkey, sizeof(Northwatch) + (MAXSIZE * sizeof(RegionWatch)) , IPC_CREAT | 0775);
	
	if (segment_id_watch == -1) {
		cout << "Error shmget : " << errno << endl;

		exit(1);
	}
	//else
	//	cout << "Segment id is : " << segment_id_watch << endl;
	// * Shared memory for watches

	// Socket Settings
	FILE *fp;
	int s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s < 0) {
		cout << "Error : Server socket"; 
		exit(1);
	}

	struct sockaddr_un saun;
	saun.sun_family = AF_UNIX;
	strcpy(saun.sun_path, socketpath);

	unlink(socketpath);
	socklen_t len = sizeof(saun.sun_family) + strlen(saun.sun_path);
	
	
	if (bind(s, (struct sockaddr *)&saun, len) < 0) {
		perror("server: bind");
		exit(1);
	}
	if (listen(s, 5) < 0) {
		perror("server: listen");
		exit(1);
	}
	// * Socket Settings

	struct sockaddr_un fsaun;
	socklen_t fromlen;
	int ns;

	// environment
	Environment* ptr = (Environment *)shmat(segment_id, NULL, 0);
	ptr[0] = Environment();

	ptr->initVariables();
	for (int i=0; i<MAXSIZE; i++){
		ptr->regions[i].initCond();
	}
	if (shmdt(ptr) < 0)
		perror("shared memory env: detach");
	
	// watches
	Northwatch* watchptr = (Northwatch *) shmat(segment_id_watch, NULL, 0);
	watchptr[0] = Northwatch();
	watchptr->initVariables();
	for (int i=0; i<MAXSIZE; i++){
		watchptr->watches[i].initCond();
	}
	if (shmdt(watchptr) < 0)
		perror("shared memory watches: detach");


	while (true) {

		
		if ((ns = accept(s, (struct sockaddr *)&fsaun, &fromlen)) < 0) {
			perror("server: accept");
			exit(1);
		}
		if (fork()) 
			continue;

		
		fp = fdopen(ns, "r");

		Environment* env = (Environment *)shmat(segment_id, NULL, 0);
		Northwatch* nwatch = (Northwatch *)shmat(segment_id_watch, NULL, 0);

		pthread_mutex_lock(&env->envlock);
		//cout << "Attached: " << env[0].size << " " << env[0].nextOwner << endl;
		int ownerID = env[0].nextOwner++;
		pthread_mutex_unlock(&env->envlock);

		vector<RegionWatch> myWatches;
		int watchID = 1;
		// ipcs -m  &  ipcrm -M 9998
		char c;
		while (1) {
			string command = "";
			string response = "";
	        while ((c = fgetc(fp)) != EOF) {

	            if (c == '\n') {
	                break;
	            } else
	            	command += c;
	        }

	        //cout << "Arriving message: " << command << endl;
	        if (command.empty()){
	        	//cout << "Client closed socket" << endl;
	        	break;
	        }

	        vector<string> sp = split(command, ' ');
	        
	        // PARSE THE COMMANDS
	        if (sp[0] == "BYE") {
	        	//cout << "bye process" << endl;
	        	break;
	        }
	        else if (sp[0] == "TRYLOCKR") {
	        	if (sp.size() != 5) {
	        		cout << "WRONG ARGUMENT" << endl;
	        		continue;
	        	}
	        	double x, y, w, h;
	        	stringstream cx(sp[1]), cy(sp[2]), cw(sp[3]), ch(sp[4]);
	        	cx >> x; cy >> y; cw >> w; ch >> h;
	        	// cout << x << " " << y << " " << w+ h << endl;
	        	
				pthread_mutex_lock(&env->envlock);

				int nextL = env->nextLockId++;
	        	RegionLock* req = new RegionLock(ownerID, nextL, 0, x, y, w, h);
				bool isOK;
				
				isOK = true;
				int index = 0, i = 0;
				while (i < env->size) {
					if (env->regions[index].isActive()){
						if (env->regions[index].isBlocking(req[0])){
							isOK = false;
							break;
						}
						i++;
						index++;
					}
					else{
						index++;
					}
				}
				
				if (isOK) {
					int avInd = env->getFirstAvailableInd();
					env->regions[avInd] = *req;
					env->regions[avInd].setActive();
					env->size++;
					//cout << "New lock is added ID: " << nextL << endl;
					response += iToS(nextL);
				} else {
					//cout << "Failed" << endl;
					env->nextLockId--;
					response += "Failed";
				}
				
				pthread_mutex_unlock(&env->envlock);
				sendMessage(ns, response);
				if (isOK) //checkWatches(&myWatches, *req, ns, true);
					checkWatchesRevise(nwatch, *req, ns, true);
			}
			else if (sp[0] == "TRYLOCKW") {
	        	if (sp.size() != 5){
	        		cout << "WRONG ARGUMENT" << endl;
	        		continue;
	        	}
	        	double x, y, w, h;
	        	stringstream cx(sp[1]), cy(sp[2]), cw(sp[3]), ch(sp[4]);
	        	cx >> x; cy >> y; cw >> w; ch >> h;
	        	// cout << x << " " << y << " " << w+ h << endl;
	        	
				pthread_mutex_lock(&env->envlock);
				
				int nextL = env->nextLockId++;
	        	RegionLock* req = new RegionLock(ownerID, nextL, 1, x, y, w, h);
				bool isOK;
				
				isOK = true;
				int index = 0, i = 0;
				while (i < env->size) {
					if (env->regions[index].isActive()){
						if (env->regions[index].isBlocking(req[0])){
							isOK = false;
							break;
						}
						i++;
						index++;
					}
					else{
						index++;
					}
				}
				
				if (isOK) {
					int avInd = env->getFirstAvailableInd();
					env->regions[avInd] = *req;
					env->regions[avInd].setActive();
					env->size++;
					//cout << "New lock is added ID: " << nextL << endl;
					response += iToS(nextL);
				} else {
					//cout << "Failed" <<  endl;
					env->nextLockId--;
					response += "Failed";
				}
				
				pthread_mutex_unlock(&env->envlock);
				sendMessage(ns, response);
				if (isOK) //checkWatches(&myWatches, *req, ns, true);
					checkWatchesRevise(nwatch, *req, ns, true);
			}
	        else if (sp[0] == "LOCKR") {
	        	if (sp.size() != 5){
	        		cout << "WRONG ARGUMENT" << endl;
	        		continue;
	        	}
	        	double x, y, w, h;
	        	stringstream cx(sp[1]), cy(sp[2]), cw(sp[3]), ch(sp[4]);
	        	cx >> x; cy >> y; cw >> w; ch >> h;
	        	// cout << x << " " << y << " " << w+ h << endl;
	        	
				pthread_mutex_lock(&env->envlock);
				int nextL = env->nextLockId++;
	        	RegionLock* req = new RegionLock(ownerID, nextL, 0, x, y, w, h);
				bool isOK;
				
				while(1) {
					isOK = true;
					int index = 0, i = 0;
					while (i < env->size) {
						if (env->regions[index].isActive()){
							if (env->regions[index].isBlocking(req[0])){
								isOK = false;
								break;
							}
							i++;
							index++;
						}
						else{
							index++;
						}
					}
					
					if (isOK) {
						int avInd = env->getFirstAvailableInd();
						env->regions[avInd] = *req;
						env->regions[avInd].setActive();
						env->size++;
						//cout << "New lock is added ID: " << nextL << endl;
						response += iToS(nextL);
						break;
					} else {
						//cout << index << " indexed lock is blocking. Waiting for unlock: " << &(env->regions[index].condLock) << endl;
						pthread_cond_wait(&env->regions[index].condLock, &env->envlock);
						//cout << "Waiting is over checking again. OwnerID: " << ownerID << " Index: " << index << endl;
					}
				}
				pthread_mutex_unlock(&env->envlock);
				sendMessage(ns, response);
				if (isOK) //checkWatches(&myWatches, *req, ns, true);
					checkWatchesRevise(nwatch, *req, ns, true);
			}
			else if (sp[0] == "LOCKW") {
	        	if (sp.size() != 5){
	        		cout << "WRONG ARGUMENT" << endl;
	        		continue;
	        	}
	        	double x, y, w, h;
	        	stringstream cx(sp[1]), cy(sp[2]), cw(sp[3]), ch(sp[4]);
	        	cx >> x; cy >> y; cw >> w; ch >> h;
	        	// cout << x << " " << y << " " << w+ h << endl;
				pthread_mutex_lock(&env->envlock);
				int nextL = env->nextLockId++;
	        	RegionLock* req = new RegionLock(ownerID, nextL, 1, x, y, w, h);
				bool isOK;
				while(1) {
					isOK = true;
					int index = 0, i = 0;
					while (i < env->size) {
						if (env->regions[index].isActive()){
							if (env->regions[index].isBlocking(req[0])){
								isOK = false;
								break;
							}
							i++;
							index++;
						}
						else{
							index++;
						}
					}
					
					if (isOK) {
						int avInd = env->getFirstAvailableInd();
						env->regions[avInd] = *req;
						env->regions[avInd].setActive();
						env->size++;
						//cout << "New lock is added ID: " << nextL << endl;
						response += iToS(nextL);
						break;
					} else {
						//cout << index << " indexed lock is blocking. Waiting for unlock: " << &(env->regions[index].condLock) << endl;
						pthread_cond_wait(&env->regions[index].condLock, &env->envlock);
						//cout << "Waiting is over checking again. OwnerID: " << ownerID << " Index: " << index << endl;
					}
				}
				pthread_mutex_unlock(&env->envlock);
				sendMessage(ns, response);
				if (isOK) //checkWatches(&myWatches, *req, ns, true);
					checkWatchesRevise(nwatch, *req, ns, true);
			}
			else if (sp[0] == "UNLOCK") {
				if (sp.size() != 2){
	        		cout << "WRONG ARGUMENT" << endl;
	        		continue;
				}
	        	int id;
	        	stringstream cx(sp[1]);
	        	cx >> id;

	        	pthread_mutex_lock(&env->envlock);
				// cout << "got lock" << endl;
				int i = 0, index = 0; bool isOK;
				while (i < env->size) {
					// cout << index << " " << i << endl;
					if (env->regions[index].isActive()){
						if (env->regions[index].id == id && env->regions[index].owner == ownerID)
							break;
						index++;
						i++;
					}
					else{
						index++;
					}
					
				}
				// cout << "out of while" << endl;
				if (i != env->size) {
					// cout << "about to broadcast " << endl;
					isOK = true;
					pthread_cond_broadcast(&env->regions[index].condLock);
					//cout << index << " broadcasted " << &(env->regions[index].condLock) <<  endl;
					
					env->regions[index].setDeactive();
					env->size--;
					
					//cout << "Lock is deleted new size: " << env->size << endl;
					response += "Ok";
					
				} else {
					isOK = false;
					//cout << "Lock couldnt found" << endl;
					response += "Failed";
				}
				pthread_mutex_unlock(&env->envlock);
				sendMessage(ns, response);
				if (isOK) //checkWatches(&myWatches, env->regions[index], ns, false);
					checkWatchesRevise(nwatch, env->regions[index], ns, false);
			}
			else if (sp[0] == "MYLOCKS") {
				pthread_mutex_lock(&env->envlock);

				vector<RegionLock> regions, sortedRegions;
				
				int index = 0, i = 0;
				while (i < env->size) {
					if (env->regions[index].isActive()){
						if (env->regions[index].owner == ownerID){
							regions.push_back(env->regions[index]);
						}
						index++;
						i++;
					}
					else
						index++;
				}

				sortedRegions = sortVector(true, regions);

				/*for (int i=0; i<sortedRegions.size(); i++)
					cout << sortedRegions[i].id << " " << (sortedRegions[i].type ? "W" : "R") <<  " " << sortedRegions[i].x 
					<< " " << sortedRegions[i].y << " " << sortedRegions[i].w << " " << sortedRegions[i].h << endl;
				*/
				pthread_mutex_unlock(&env->envlock);
				sendMessageForMylocks(ns, sortedRegions);
				// sendMessage(ns, "u tried");
			}
			else if (sp[0] == "GETLOCKS") {
	        	if (sp.size() != 5) {
	        		cout << "WRONG ARGUMENT" << endl;
	        		continue;
	        	}
	        	double x, y, w, h;
	        	stringstream cx(sp[1]), cy(sp[2]), cw(sp[3]), ch(sp[4]);
	        	cx >> x; cy >> y; cw >> w; ch >> h;
	        	// cout << x << " " << y << " " << w+ h << endl;
				pthread_mutex_lock(&env->envlock);
				RegionLock* req = new RegionLock(ownerID, 0, 0, x, y, w, h);
				vector<RegionLock> regions, sortedRegions;
				int index = 0, i = 0;
				while(i<env->size){
					if (env->regions[index].isActive()){
						if (env->regions[index].isIntersecting(req[0])){
							regions.push_back(env->regions[index]);
						}
						index++;
						i++;
					} else {
						index++;
					}
				}
				
				sortedRegions = sortVector(false, regions);

				/*
				for (int i=0; i<sortedRegions.size(); i++)
					cout << (sortedRegions[i].type ? "W" : "R") <<  " " << sortedRegions[i].x 
					<< " " << sortedRegions[i].y << " " << sortedRegions[i].w << " " << sortedRegions[i].h << endl;
					*/
				pthread_mutex_unlock(&env->envlock);
				sendMessageForGetlocks(ns, sortedRegions);
				// sendMessage(ns, "u tried");
			}
			else if (sp[0] == "WATCH"){
				if (sp.size() != 5) {
	        		cout << "WRONG ARGUMENT" << endl;
	        		continue;
				}
	        	double x, y, w, h;
	        	stringstream cx(sp[1]), cy(sp[2]), cw(sp[3]), ch(sp[4]);
	        	cx >> x; cy >> y; cw >> w; ch >> h;

	        	/*
	        	RegionWatch *rew = new RegionWatch(watchID, ownerID, x, y, w, h);
	        	cout << myWatches.size() << endl;
	        	myWatches.push_back(*rew);
	        	cout << myWatches.size() << endl;
				*/
				pthread_mutex_lock(&nwatch->wlock);
	        	int find = nwatch->getFirstAvailableInd();
	        	nwatch->watches[find].id = watchID;
	        	nwatch->watches[find].owner = ownerID;
	        	nwatch->watches[find].x = x;
	        	nwatch->watches[find].y = y;
	        	nwatch->watches[find].w = w;
	        	nwatch->watches[find].h = h;
	        	nwatch->watches[find].active = true;
	        	nwatch->size++;

	        	struct passVal arguments;
	        	arguments.index = find;
	        	arguments.socket = ns;
	        	arguments.p = nwatch;
	        	
	        	pthread_t listenthread;
			    int rc;
			    rc = pthread_create(&listenthread, NULL, ListenRegion, (void*)&arguments);
			    if (rc) {
			    	cout << "Error:unable to create thread," << rc << endl;
			    }

	        	sendMessage(ns, iToS(watchID));
	        	watchID++;
	        	pthread_mutex_unlock(&nwatch->wlock);

			}
			else if (sp[0] == "UNWATCH"){
				if (sp.size() != 2) {
	        		cout << "WRONG ARGUMENT" << endl;
	        		continue;
				}
	        	int id;
	        	stringstream cx(sp[1]);
	        	cx >> id;

	        	bool isDeleted = false;
	        	/*cout << myWatches.size() << endl;
	        	for (int i = 0; i<myWatches.size(); i++) {
	        		cout << "id: " << myWatches[i].id << endl;
	        		if (myWatches[i].id == id) {
	        			myWatches.erase(myWatches.begin() + i);
	        			isDeleted = true;
	        			break;
	        		}
	        	}
	        	*/
			    pthread_mutex_lock(&nwatch->wlock);

	        	int index = 0, i = 0;
				while (i < nwatch->size) {
					if (nwatch->watches[index].active){
						if (nwatch->watches[index].owner == ownerID && nwatch->watches[index].id == id){
							nwatch->watches[index].active = false;
							nwatch->size--;
							pthread_cond_broadcast(&nwatch->watches[index].condLock);
							isDeleted = true;
							break;
						}
						i++;
						index++;
					}
					else{
						index++;
					}
				}

	        	if (isDeleted)
	        		sendMessage(ns, "Ok");
	        	else
	        		sendMessage(ns, "Failed");
			    pthread_mutex_unlock(&nwatch->wlock);

	        }
	        else if (sp[0] == "WATCHES"){

			    pthread_mutex_lock(&nwatch->wlock);

				int index = 0, i = 0;
				while (i < nwatch->size) {
					if (nwatch->watches[index].active){
						if (nwatch->watches[index].owner == ownerID ){
							string m = "";
							m = m + iToS(nwatch->watches[index].id) + " " + dToS(nwatch->watches[index].x) + " " + dToS(nwatch->watches[index].y) + " " + dToS(nwatch->watches[index].w) + " " + dToS(nwatch->watches[index].h);
							sendMessage(ns, m);
						}
						i++;
						index++;
					}
					else{
						index++;
					}
				}

			    pthread_mutex_unlock(&nwatch->wlock);

	        }
			
			else {
				sendMessage(ns, "Command not found");
			}



			
			/*
			else if (sp[0] == "WAITX") {
				pthread_mutex_lock(&env->envlock);

				cout << "hello " << &env->regions[0].condLock << " " << &env->regions[0].attrcond << endl;
				pthread_cond_wait(&env->regions[5].condLock, &env->envlock);
				cout << "world" << endl;
				pthread_mutex_unlock(&env->envlock);
			}
			else if (sp[0] == "NOTIFX") {

				pthread_mutex_lock(&env->envlock);
				//cout << "notifing " << &envcond  << endl;
				cout << "BR " << &env->regions[0].condLock << " " << &env->regions[0].attrcond << endl;
				pthread_cond_broadcast(&env->regions[5].condLock);
				//cout << "index: " << id << " - " <<  &env->regions[id] << " " << env->regions[id].owner << endl;
				cout << "OAD " << &env->regions[0].condLock << " " << &env->regions[0].attrcond << endl;

				pthread_mutex_unlock(&env->envlock);
			}

			else if (sp[0] == "LCK") {
				pthread_mutex_lock(&env->envlock);
				cout << "i got lock" << endl;
			}
			else if (sp[0] == "TRY") {
				pthread_mutex_lock(&env->envlock);
				cout << "got lock printing this" << endl;
				pthread_mutex_unlock(&env->envlock);
			}
			else if (sp[0] == "UNLCK") {
				cout << "im unlocked" << endl;
				pthread_mutex_unlock(&env->envlock);

			}*/
	        
	    }

		if (shmdt(env) < 0)
			cout << "shared memory : detach failed" << endl;

        //cout <<  "closing child" << endl;
        close(s);
		exit(0);
    }

		

   }

   /*for( i=0; i < NUM_THREADS; i++ ){
	  cout << "main() : creating thread, " << i << endl;
	  rc = pthread_create(&threads[i], NULL, PrintHello, (void *)i);
	
	  if (rc){
		 cout << "Error:unable to create thread," << rc << endl;
		 exit(-1);
	  }
   }*/

