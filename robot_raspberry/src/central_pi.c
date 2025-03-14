#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <pthread.h>

#if defined(WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <time.h>
#endif

#include "utils.h"
#include "marvelmind.h"

# define CRTSCTS 020000000000 /* Flow control.  */
#define IP_SIZE 16 
#define MAXOCTETS 500
#define MAXSERIAL 100
#define DELAY_USLEEP 10000 // 10ms

bool terminateProgram = false;

typedef struct {
    int computer_sd;
    int port;
    char *waypoints;
    double *xm;
    double *ym;
} thread_args_t;

pthread_mutex_t coordinates_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t waypoints_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t write_waypoints_to_aduino_mutex = PTHREAD_MUTEX_INITIALIZER;

#if defined(WIN32) || defined(_WIN64)
BOOL CtrlHandler(DWORD fdwCtrlType)
{
    if ((fdwCtrlType == CTRL_C_EVENT ||
         fdwCtrlType == CTRL_BREAK_EVENT ||
         fdwCtrlType == CTRL_CLOSE_EVENT ||
         fdwCtrlType == CTRL_LOGOFF_EVENT ||
         fdwCtrlType == CTRL_SHUTDOWN_EVENT) &&
        (terminateProgram == false))
    {
        terminateProgram = true;
        return true;
    }
    else
        return false;
}
#else
void CtrlHandler(int signum)
{
    terminateProgram = true;
}
#endif

#if defined(WIN32) || defined(_WIN64)
void sleep(unsigned int seconds)
{
    Sleep(seconds * 1000);
}
#endif

#if defined(WIN32) || defined(_WIN64)
HANDLE ghSemaphore;
DWORD dwSemWaitResult;
void semCallback()
{
    ReleaseSemaphore(
        ghSemaphore, // handle to semaphore
        1,           // increase count by one
        NULL);
}
#else
// Linux
static sem_t *sem;
struct timespec ts;
void semCallback()
{
    sem_post(sem);
}
#endif // WIN32

#ifdef _WIN64
const wchar_t *GetWC(const char *c)
{
    const size_t cSize = strlen(c) + 1;
    wchar_t *wc = malloc(sizeof(wchar_t) * cSize);
    mbstowcs(wc, c, cSize);

    return wc;
}
#endif


int openSerialPort(const char* portStr) {
    int port;
    CHECK(port = open(portStr, O_RDWR | O_NOCTTY | O_NDELAY), "msg: Unable to open serial");
    fcntl(port, F_SETFL, 0);
    return port;
}

void setSerialPort(int port){
    struct termios options;
    tcgetattr(port, &options);

    // Set the baud rates to 115200...
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);

    // Enable the receiver and set local mode...
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

    // Disable hardware flow control
    options.c_cflag &= ~CRTSCTS;
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    
    CHECK(tcsetattr(port, TCSANOW, &options), "msg: Unable to set serial port");
}

void writeSerial(int port, const char* data){
    CHECK(write(port, data, strlen(data)), "msg: Write to serial failed!");
}

void readSerial(int port, char* buffer, int size){
    char single_char;
    int index = 0;
    while (1)
    {
        int reception = read(port, &single_char, 1); // Lire 1 caractère
        if (reception > 0)
        {
            buffer[index++] = single_char;
            DEBUG_PRINT("Caractere recu : %c\n", single_char); 

            if (single_char == '\n' || index >= size - 1) break;

        }
        else if (reception == -1)
        {
            perror("Erreur lecture port serie");
            break;
        }
        else
        {
            // Aucune donnée reçue, attendre un peu avant de réessayer
            usleep(DELAY_USLEEP);
        }
    }

    // Terminer la chaîne de caractères
    buffer[index] = '\0';

    // Afficher le message reçu
    if (index > 0)
    {
        printf("Message reçu : %s\n", buffer);
    }
    else
    {
        printf("Aucune donnee recue.\n");
    }

    DEBUG_PRINT("Reception finie.\n");
}

void closeSerialPort(int port){
    CHECK(close(port), "msg: Unable to close serial port");
}


void *communication_ordi_central(void *arg){
    thread_args_t *args = (thread_args_t *)arg;

    int computer_sd = args->computer_sd;
    char *waypoints = args->waypoints;

    free(arg);  // On libère la mémoire après avoir copié les valeurs

    char buffer_recv[MAXOCTETS + 1] = "";
    char buffer_send[MAXOCTETS + 1] = "";

    while(true){
        int nb_car = recv(computer_sd, buffer_recv, MAXOCTETS, 0);
        CHECK_ERROR(nb_car, -1, "\nProblème de réception !!!\n");
        if (nb_car == 0) { // Si le serveur se déconnecte
            pthread_exit(NULL);
        }
        buffer_recv[nb_car] = '\0';

        strcpy(waypoints, buffer_recv);

        // waypoints est non nul, le thread serial peut répondre à l'arduino lorsque celui ci demande les waypoints
        pthread_mutex_unlock(&write_waypoints_to_aduino_mutex);

        
        // on attend que l'arduino est fini de faire ses waypoints
        // on essaie de lock la mutex, elle est unlock dans le thread
        // communication serial arduino lorsque 'f' est recu de l'arduino
        pthread_mutex_lock(&waypoints_mutex);
        strcpy(waypoints, ""); // on remet waypoints vide
        
        strcpy(buffer_send, "DONE");
        // on transmet à l'ordinateur central que le trajet a ete effectuel
        CHECK_ERROR(send(computer_sd, buffer_send, strlen(buffer_send) + 1, 0), -1, "\nProblème d'émission !!!\n");
    }
    return NULL;
}

void *communication_serial_arduino(void *arg){
    thread_args_t *args = (thread_args_t *)arg;
    int port = args->port;
    char *waypoints = args->waypoints;
    double *xm = args->xm;
    double *ym = args->ym;

    free(arg);

    char buffer_recv[MAXSERIAL + 1] = "";
    char buffer_send[MAXSERIAL + 1] = "";
    while(true){
        readSerial(port, buffer_recv, MAXSERIAL);
        if (strlen(buffer_recv) <= 0){
            continue;
        }
        switch (buffer_recv[0])
        {

            // Le robot demande a avoir les waypoints qu'il doit faire
            // Le robot sera en attente d'une réponse avant de faire quoi que ce soit d'autre
            case 'w':

                // on attend de recevoir les waypoints de l'ordinateur central
                // lorsque le thread de communication avec l'ordinateur central a reçu les waypoints
                // il unlock la mutex write_waypoints_to_aduino_mutex
                pthread_mutex_lock(&write_waypoints_to_aduino_mutex);
                // ici les waypoints sont non nuls
                printf("%s\n", waypoints); 
                writeSerial(port, waypoints);
                break;

            // l'arduino a fini de faire son trajet
            case 'f':
                pthread_mutex_unlock(&waypoints_mutex);
                break;

            // l'arduino demande sa position
            case 'p':
                pthread_mutex_lock(&coordinates_mutex);
                sprintf(buffer_send, "x:%.0f,y:%.0f\n", *xm,*ym);  // Formatage correct 
                printf("x:%.0f,y:%.0f\n", *xm, *ym);
                pthread_mutex_unlock(&coordinates_mutex);
                printf("buffer_send : %s\n", buffer_send);
                writeSerial(port, buffer_send);
                break;
            
            default:
                break;
        }
        
    }
    return NULL;
}

/*---------------------------------------------   MAIN   ----------------------------------------------*/
int main(int argc, char *argv[])
{
    bool communication = false;
    pthread_mutex_lock(&waypoints_mutex);
    pthread_mutex_lock(&write_waypoints_to_aduino_mutex);
    char *waypoints = malloc(MAXSERIAL + 1);  // Allocation mémoire
    strcpy(waypoints, "");
    double xm, ym;

    int port = openSerialPort("/dev/ttyS0");
    setSerialPort(port);

    if (argc == 4){
        communication = true;
        printf("Processing with connexion to ordinateur central\n");
        u_int16_t port_ord = (u_int16_t) atoi(argv[2]);

        struct sockaddr_in addr_ord, addr_raspberry;
        socklen_t addr_len = sizeof(addr_ord);
        
        int sd = socket(AF_INET, SOCK_STREAM, 0);
        CHECK_ERROR(sd, -1, "Erreur socket non cree !!! \n");

        // Set up client address (bind to specific IP)
        memset(&addr_raspberry, 0, sizeof(addr_raspberry));
        addr_raspberry.sin_family = AF_INET;
        addr_raspberry.sin_port = htons(0); // Let OS assign a random port
        addr_raspberry.sin_addr.s_addr = inet_addr(argv[3]); // raspberry IP from CLI

        // Bind client socket
        int erreur = bind(sd, (struct sockaddr *)&addr_raspberry, sizeof(addr_raspberry));
        CHECK_ERROR(erreur, -1, "Erreur lors du bind du client !!!");
        
        addr_ord.sin_family = AF_INET;
        addr_ord.sin_port = htons(port_ord);
        addr_ord.sin_addr.s_addr = inet_addr(argv[1]);
        erreur = connect(sd, (const struct sockaddr *)&addr_ord, addr_len);
        CHECK_ERROR(erreur, -1, "Erreur de connexion !!!\n");
        printf("Connected to server %s:%d\n", argv[1], port_ord);

        // création du thread de communication avec l'ordinateur central
        pthread_t communication_ordi_central_thread;
        thread_args_t *args_ordi = malloc(sizeof(thread_args_t));
        args_ordi->computer_sd = sd;
        args_ordi->waypoints = waypoints;
        pthread_create(&communication_ordi_central_thread, NULL, communication_ordi_central, (void*) args_ordi);
        pthread_detach(communication_ordi_central_thread);
        
        pthread_t communication_serial_arduino_thread;
        thread_args_t *args_serial = malloc(sizeof(thread_args_t));
        args_serial->port = port;
        args_serial->waypoints = waypoints;
        args_serial->xm = &xm;
        args_serial->ym = &ym;
        pthread_create(&communication_serial_arduino_thread, NULL, communication_serial_arduino, (void*) args_serial);
        pthread_detach(communication_serial_arduino_thread);
    }
    else{
        printf("Processing without connexion to ordinateur central\n");
        printf("If connexion is desired please do: %s <IP_ORD> <PORT_ORD> <RASPBERRY_IP>\n", argv[0]);
    }

// get port name from command line arguments (if specified)
#ifdef _WIN64
    const wchar_t *ttyFileName;
    if (argc == 2)
        ttyFileName = GetWC(argv[1]);
    else
        ttyFileName = TEXT(DEFAULT_TTY_FILENAME);
#else
    const char *ttyFileName;
    if (argc == 2)
        ttyFileName = argv[1];
    else
        ttyFileName = DEFAULT_TTY_FILENAME;
#endif

    // Init
    struct MarvelmindHedge *hedge = createMarvelmindHedge();
    if (hedge == NULL)
    {
        puts("Error: Unable to create MarvelmindHedge");
        return -1;
    }
    hedge->ttyFileName = ttyFileName;
    hedge->verbose = true; // show errors and warnings
    hedge->anyInputPacketCallback = semCallback;
    startMarvelmindHedge(hedge);

    // Set Ctrl-C handler
#if defined(WIN32) || defined(_WIN64)
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);
#else
    signal(SIGINT, CtrlHandler);
    signal(SIGQUIT, CtrlHandler);
#endif

#if defined(WIN32) || defined(_WIN64)
    ghSemaphore = CreateSemaphore(
        NULL,  // default security attributes
        10,    // initial count
        10,    // maximum count
        NULL); // unnamed semaphore
    if (ghSemaphore == NULL)
    {
        printf("CreateSemaphore error: %d\n", (int)GetLastError());
        return 1;
    }
#else
    // Linux
    sem = sem_open(DATA_INPUT_SEMAPHORE, O_CREAT, 0777, 0);
#endif

    // Main loop
    while ((!terminateProgram) && (!hedge->terminationRequired))
    {
// sleep (3);
#if defined(WIN32) || defined(_WIN64)
        dwSemWaitResult = WaitForSingleObject(
            ghSemaphore, // handle to semaphore
            1000);       // time-out interval
#else
        // Linux
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        {
            printf("clock_gettime error");
            return -1;
        }
        ts.tv_sec += 2;
        sem_timedwait(sem, &ts);
#endif

        if (hedge->haveNewValues_)
        {
            struct PositionValue position;

            getPositionFromMarvelmindHedge(hedge, &position);
            if (position.ready && position.address == 11)
            {
                pthread_mutex_lock(&coordinates_mutex);
                xm = -((double)position.x) ; // Envoi en négatif à cause du repère à l'envers
                ym = ((double)position.y) ;
                pthread_mutex_unlock(&coordinates_mutex);
                if (!communication){
                    char buffer_send[20];
                    sprintf(buffer_send, "x:%.0f,y:%.0f\n", xm,ym);  // Formatage correct 
                    writeSerial(port, buffer_send);
                    printf("x:%.0f,y:%.0f\n", xm, ym);
                }
                hedge->haveNewValues_ = false;
            }
        }
    }

    // Exit
    stopMarvelmindHedge(hedge);
    destroyMarvelmindHedge(hedge);
    return 0;
}
