#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>


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

bool terminateProgram = false;

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
            usleep(100000); // Délai de 100 ms
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



/*---------------------------------------------   MAIN   ----------------------------------------------*/
int main(int argc, char *argv[])
{

    int port = openSerialPort("/dev/ttyS0");
    setSerialPort(port);
    char buffer_send[20];

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

    double xm, ym;

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
            xm = ((double)position.x) ;
            ym = ((double)position.y) ;

            if (position.ready)
            {
                if (position.highResolution)
                {
                    sprintf(buffer_send, "x:%.0f,y:%.0f\n", xm,ym);  // Formatage correct 
                    writeSerial(port, buffer_send);
                    printf("x:%.0f,y:%.0f\n", xm, ym);
                }
                else
                {
                    printf("X: %.2f, Y: %.2f\n", xm, ym);
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
