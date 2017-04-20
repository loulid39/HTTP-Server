#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <signal.h>
#include <wait.h>

/*control deni de service*/
int seuil;//valeur du seuil pour deni de service (init au démarrage)
struct adr_ctrl
{
	char ip[10];
	int quant;
	struct adr_ctrl *next;
	
} *list;

pthread_mutex_t deni_mtx = PTHREAD_MUTEX_INITIALIZER;//pour acceder au list
//

/*socket du serveur*/
int sock;
/*gestion logs*/
int fdlog;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
/*Fin*/

struct requete{
	char dir[50];
	char mval[50];
	char *ligne1;
	int valid;
};

struct creat
{
	int fd;
	char msg[64];
};

int start(int, int);
int send_file(char *,int );
void *n_connexion(void *);
void loopS();
char *getContentType(char *);
char* get_entete(char *);
//struct requete parse_requete(char *);
//void parse_requete(char *,struct requete *);

char *getFullPath(char *filename){
	FILE *fp;
	char cmd[32];
	cmd[0] = '\0';
	strcpy(cmd,"readlink -f ");
	strcat(cmd,filename);
	char path[1035], *p = malloc(1035 * sizeof(char));
	if((fp = popen(cmd, "r")) == NULL){
		perror("open");
		return NULL;
	}
	while (fgets(path, sizeof(path)-1, fp) != NULL) {
  	strcpy(p, path);
  }
  pclose(fp);
  return p;
}

/*init la connexion*/
int start(int port,int nb_client){
	struct sockaddr_in sin;

	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		return 0;
	}
	
	memset((char*)&sin, 0, sizeof(sin));
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(port);
	sin.sin_family = AF_INET;

	if(bind(sock,(struct sockaddr*)&sin,sizeof(sin)) < 0){
		return 0;
	}

	/*nb_client connectent simultanement*/
	listen(sock, nb_client);

	/*creation du fichier logs*/
	if((fdlog = open("/tmp/http3500130.log", O_RDWR | O_CREAT | O_APPEND,0600)) == -1){
		perror("open");
		exit(0);
	}

return 1;
}
/*fonction d'envoi d'un fichier au client*/
int send_file(char *file,int sock){
	int sizeDATA = 0;
	char msg[256];
	int fd = open(file, O_RDONLY, 0600);
	if(fd){
			while(read(fd,&msg,sizeof(msg)) > 0){
			sizeDATA += (int)strlen(msg);
			write(sock,msg, strlen(msg));
		}
	}
	close(fd);

	return sizeDATA;
}

/*fonction traiter premiere requete*/
struct requete parse_requete(char *msg){
	struct requete r;
	/*traitement de la requete*/
	int i;
	int y = 0;
	char req[50],type[50],dir[50],mval[50],mtype[50];
	char p[256],q[256],tmp[512];
	char s[2] = "\n";
	char *token;
	
	r.valid = 0;
	int pos = 0,nb_saut=0;
	strcpy(tmp, msg);
	p[0] = '\0';
	q[0] = '\0';
	/*separer les deux premier ligne de la requete*/
	nb_saut = 0;
	int ytmp;
	for(i=0;i<strlen(tmp);i++){
		if(tmp[i] != '\n'){	
			if(nb_saut == 0) p[y] = tmp[i];
			if(nb_saut == 1) q[y] = tmp[i];
			y++;
		} else {
			
			
			if(nb_saut == 0){
				ytmp = y-1;
				p[y] = '\0';
			}
			if(nb_saut == 1){
				q[y] = '\0';
			}
			y = 0;
			nb_saut++;
		
		if(nb_saut == 2) break;
	}
	}
	


	/*traitement de chaque ligne*/
	for(i=0;i<strlen(p);i++){
		if(p[i] != ' '){
			if(pos == 0) req[y] = p[i];
			if(pos == 1) dir[y] = p[i];
			if(pos == 2) type[y] = p[i];			
			y++;
		} else {
			if(pos == 0) req[y] = '\0';
			if(pos == 1) dir[y] = '\0';			
			pos++;
			y = 0;
		}
	}
	if(pos == 2) type[y] = '\0';
	pos = 0;
	y = 0;
	for(i=0;i<strlen(q);i++){
		if(q[i] != ' '){
			if(pos == 0) mtype[y] = q[i];
			if(pos == 1) mval[y] = q[i];			
			y++;
		} else{
			if(pos == 0) mtype[y] = '\0';			
			pos++;
			y=0;		
		}
	}	
	if(pos == 1) mval[y] = '\0';

	if((strcmp(type,"HTTP/1.1") ) && (strcmp(mtype,"Host:") == 0)) r.valid = 1;
	/*supprimer la première '/' du chemin*/
	for(i=0;i<strlen(dir);i++) dir[i] = dir[i+1];
		dir[i] = '\0';
	r.ligne1 = malloc(256 * sizeof(char));
	p[ytmp] = '\0';
	r.ligne1 = p;
	//r.mval = malloc(256 * sizeof(char));
	strcpy(r.dir,dir);
	strcpy(r.mval,mval);
	/*Fin de traitement*/
	return r;
}
/*Fin*/

/*Execute app & script*/
int executeInSrvEx(int soc, char *file, char *lg){
	pid_t fils;
	char cod[3];
	int fd,reslt = 0,size = 0;
	char mlog[512];
	mlog[0] = '\0';
	strcat(mlog, lg);
	void kill_fils(int sig){
		kill(fils, SIGTERM);
	}

	signal(SIGALRM,(void(*)(int))kill_fils);
	if((fils = fork()) == 0){
		FILE *fp;
		char ligne[1026],msg[250],m[250];
		msg[0] = '\0';
		ligne[0] = '\0';
		if((fp = popen(getFullPath(file), "r")) == NULL){
			return -1;
		}
		
		while(fgets(msg, sizeof(msg)-1, fp) != NULL){
			//write(soc,linge,strlen(linge)+1);
			strcat(ligne,msg);
			strcat(ligne, "<br>");
		}

		if((int)strlen(ligne) > 0){
			strcpy(m,"HTTP/1.1 200 OK");
    		strcat(m,"\nContent-Type: text/html\n\n");
    		//strcat(m, "\nContent-Length: ");
    		char rr[3];
    		rr[0] = '\0';
    		sprintf(rr,"%ld",strlen(ligne));
    		//strcat(m,rr);
    		//strcat(m,"\n");
    		write(soc, m, strlen(m)+1);
    		char nn[3];
    		nn[0] = '\0';
    		sprintf(nn, "%d", 200);
    		strcat(mlog, nn);
    		strcat(mlog, " ");
    		size = (int) write(soc,ligne,strlen(ligne)+1);
    		sprintf(nn, "%d", size);
    		strcat(mlog, nn);
    		strcat(mlog, "\n");
		} else reslt = -1;
		
		if(reslt == 0){
			shutdown(soc, 2);
			close(soc);
			pthread_mutex_lock(&mutex);
			write(fdlog,mlog,strlen(mlog));
			pthread_mutex_unlock(&mutex);
		}
		
		pclose(fp);
		exit(reslt);

	} else {
	
	int status;
	alarm(10);
	wait(&status);
	if(WIFEXITED(status)){
		/*terminer normal*/
		reslt = 0;

	} else{
		/*reception SIGKILL ou crash*/
		reslt = -1;
	}
}
	return reslt;
}
/*Fin*/

char *filterIP(char *ip){
	int i;
	for(i=0;i<(int)strlen(ip);i++){
		if(ip[i] == ':'){
			ip[i] = '\0';
			return ip;
		}
	}
	return NULL;
}
/*gerer deni de service*/
void exit_thread(char *ip, int qunt){//memoriser quantiter de donner envoyer 
struct adr_ctrl *tmp = malloc(sizeof(struct adr_ctrl));
int found = 0;
	pthread_mutex_lock(&deni_mtx);
	tmp = list; 

	while(tmp != NULL){
		if(strcmp(tmp->ip, ip) == 0){
			tmp->quant += qunt;
			found = 1;
		} 
		tmp = tmp->next;
	}
	if(found == 0){
		tmp = malloc(sizeof(struct adr_ctrl));
		strcpy(tmp->ip,ip);
		tmp->quant = qunt;
		tmp->next = list;
		list = tmp;
	}
	
	pthread_mutex_unlock(&deni_mtx);

}

void *tgest_den(void *arg){
	int i;
	//handler, sigalarm chaque minute 
	struct adr_ctrl *tmp = malloc(sizeof(struct adr_ctrl)); 
	void gest_den(int sig){
		pthread_mutex_lock(&deni_mtx);
		while(list != NULL){
			tmp = list;
			list = list->next;
			free(tmp);
		}
		pthread_mutex_unlock(&deni_mtx);
		alarm(60);
		signal(SIGALRM,(void(*)(int))gest_den);
	}

	/*init var gestion deni de srvice*/
	pthread_mutex_lock(&deni_mtx);
	list = malloc(sizeof(struct adr_ctrl));
	list = NULL;
	pthread_mutex_unlock(&deni_mtx);
	alarm(60);
	signal(SIGALRM,(void(*)(int))gest_den);

	while(1){//apres chaque minute, init les compteurs
		sleep(10);
	}

}

int canConnect(char* ip){
	int i,res = 1;
	pthread_mutex_lock(&deni_mtx);
	while(list != NULL){
		if(strcmp(list->ip,ip) == 0){
			if(list->quant > seuil) res = 0;
			break;
		}
	}
	pthread_mutex_unlock(&deni_mtx);
	return res;
}

//
/*fonction de thread pour gere la connexion par client */
void *n_connexion(void *arg){
	printf("connexion %u\n",(unsigned int)pthread_self());
	char msg[512],mlog[1024],nb[5],nn[5];
	int sizeDATA = 0;
	struct creat cr = *((struct creat*)(arg));
	int soc = cr.fd;//
	mlog[0] = '\0';
	strcpy(mlog,cr.msg);
	sprintf(nb,"%u",(unsigned int)pthread_self());
	strcat(mlog,nb);
	strcat(mlog," ");
	if(read(soc, &msg, sizeof(msg)) < 0){
		pthread_exit((void*)0);
	}
	struct requete r = parse_requete(msg);
	strcat(mlog,r.ligne1);
	strcat(mlog," ");
	if(r.valid == 1){//on accepte que des requetes HTTP/1.1
	char mrep[512];
	char *ent ;
	/*preparation header*/
	int fd;
	char msg[256];
	struct stat buf;
	char *cont = malloc(sizeof(char) * 50);
	cont = getContentType(r.dir);	
	int rectv;
	if((strcmp(cont,"application/x-executable") == 0) || (strcmp(cont,"text/x-shellscript") == 0)){
		rectv = executeInSrvEx(soc, r.dir, mlog);
		if(rectv != 0){
		 strcpy(msg,"HTTP/1.1 500 Internal Server Error");
		 strcat(msg,"\nContent-Type: text/html\n\n");
		 strcat(msg,"Erreur 500 : Internal Server Error");
		 write(soc, msg, strlen(msg)+1);
		 sprintf(nn, "%d", 500);
		 strcat(mlog,nn);
		 strcat(mlog, " ");
		 strcat(mlog,"0\n");
		 pthread_mutex_lock(&mutex);
		 write(fdlog,mlog, strlen(mlog));
		 pthread_mutex_unlock(&mutex);
		 		}
		 	shutdown(soc, 2);
			close(soc);	
			printf("deconnexion %u\n\n",(unsigned int)pthread_self());
		 	pthread_exit((void*)0);
		
			} 
			else{
	fd = open(r.dir, O_RDONLY, 0600);

	if(fd > 0){
		if(fstat(fd,&buf) == -1) {
			perror("fstat");
			pthread_exit((void*)0);
		}
		
			/*on est la parce que fichier (!executable & !script)*/
		if(buf.st_mode & S_IROTH){
			 strcpy(msg,"HTTP/1.1 200 OK");
			 strcat(msg,"\nContent-Type: ");
			 strcat(msg,cont);
			 strcat(msg, "\nContent-Length: ");
			 char ll[3];
			 ll[0] = '\0';
			 sprintf(ll,"%ld",buf.st_size);
			 strcat(msg, ll);
			 }
			else{
			strcpy(msg,"HTTP/1.1 403 Forbidden\nContent-Type: text/html");
			}
		
	} else {
		strcpy(msg,"HTTP/1.1 404 Not Found\nContent-Type: text/html");
	}
	strcat(msg,"\n");
	close(fd);
	/*fin prepararion header*/
	ent = msg;
	nn[0] = '\0';
	if(strstr(ent,"200") != NULL){
		strcat(ent,"\n");
		if(write(soc,ent,strlen(ent)) == -1){
			perror("write");
		}
		sizeDATA = send_file(r.dir, soc);
		sprintf(nn,"%d",200);
		/**/
		char *qs = getFullPath(r.dir);
		/**/

	} else if(strstr(ent,"404") != NULL) {
		sprintf(nn,"%d",404);
		strcat(ent,"\nErreur 404 : File Not Found");
		if(write(soc,ent,strlen(ent)) == -1){
			perror("write");
		}

	} else {
		sprintf(nn,"%d",403);
		strcat(ent,"\nErreur 403 : Forbidden");
		if(write(soc,ent,strlen(ent)) == -1){
			perror("write");
		}
	}
	strcat(mlog,nn);
	strcat(mlog," ");

	char sd[5];
	sprintf(sd,"%d",sizeDATA);
	strcat(mlog,sd);
	strcat(mlog,"\n");
	shutdown(soc, 2);
	close(soc);
	if(r.valid == 1){
	pthread_mutex_lock(&mutex);
	if(write(fdlog,mlog,strlen(mlog)) == -1){
		perror("write");
	}
	fsync(fdlog);
	pthread_mutex_unlock(&mutex);
	char *adr = filterIP(r.mval);
	exit_thread(adr,sizeDATA);
	}
	
}}
shutdown(soc, 2);
close(soc);
printf("deconnexion %u\n\n",(unsigned int)pthread_self());
pthread_exit((void*)0);
}

/* loop du serveur */
void loopS(){
	int soc,flen ;
	struct sockaddr_in exp;
	flen = sizeof(exp);
	pthread_t tid,t_id;

	/*thread pour gerer deni de service*/
	if(pthread_create(&t_id, NULL, tgest_den,NULL) == -1){
		perror("pthread deni");
		exit(0);
	}

	//
	while(1){
		sleep(2);//pour traiter les requetes dans l'ordre
		soc = accept(sock,(struct  sockaddr*)&exp,&flen);
		char* ip = inet_ntoa(exp.sin_addr);
		int pass = canConnect(ip);
		if(pass == 0){
			char msg[100];
			msg[0] = '\0';
			strcpy(msg,"HTTP/1.1 403 Forbidden\nContent-Type: text/html\n\nErreur 403 : Forbidden");
			if(write(soc,msg,strlen(msg)) == -1){
			perror("write");
		}
			shutdown(soc, 2);
			close(soc);
		} else {
		struct creat t;
		t.fd = soc;		
		t.msg[0] = '\0';
		strcat(t.msg,ip);
		strcat(t.msg," ");
		time_t ctime = time(0);
		struct tm *mtime = localtime(&ctime);
		char nb[10];
		sprintf(nb, "%d",mtime->tm_mday);
		strcat(t.msg,nb);
		strcat(t.msg,"/");
		sprintf(nb, "%d",mtime->tm_mon+1);
		strcat(t.msg,nb);
		strcat(t.msg,"/");
		sprintf(nb, "%d",mtime->tm_year + 1900);
		strcat(t.msg,nb);
		strcat(t.msg," ");
		sprintf(nb, "%d",mtime->tm_hour);
		strcat(t.msg,nb);
		strcat(t.msg,":");
		sprintf(nb, "%d",mtime->tm_min);
		strcat(t.msg,nb);
		strcat(t.msg,":");
		sprintf(nb, "%d",mtime->tm_sec);
		strcat(t.msg,nb);
		strcat(t.msg," ");
		sprintf(nb,"%d",((int)getpid()));
		strcat(t.msg,nb);
		strcat(t.msg," ");

		if(soc > 0){
			if (pthread_create(&tid, NULL, n_connexion, &t) != 0)
			{
				perror("pthread_create");
			}
		}
	}
}
	pthread_join(tid,NULL);
}


/*fonction qui renvoie le Content-Type d'un fichier*/
char *getContentType(char *filename){
		FILE *in;
	extern FILE *popen();
	char buff[512];
	
	char type[32],t[10] = "file -i ";
	strcat(t,filename);	
	
	if(!(in = popen(t, "r"))){
		exit(1);
	}
	
	fgets(buff, sizeof(buff), in);	
	pclose(in);

	int y = 0,i,sp = 0;
	for(i=0;i<strlen(buff);i++){
		if(buff[i] == ';') {
			type[y] = '\0';
			break;
		}
			
		if(sp == 1){
			type[y] = buff[i];
			y++;
		}

		if(buff[i] == ' '){
			sp++;
		}
	}
	char *res  = malloc(32 * sizeof(char));
	strcpy(res , type);
	return res;
}