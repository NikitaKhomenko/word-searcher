#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFFER_SIZE 100000
#define url_length 40
#define portNum 80  //no need for different port numbers, site will redirect to correct port.
static int numOfSites;

typedef struct site
{
    char url[url_length];  // site address
    int wordNumOfOccurrences;

} SITE;

int getConnectedSocket(char *host, in_port_t port);
SITE* FillSiteArray(SITE* siteArray, char* fname);
int CountWordOccurrencesInSite(char *buffer, char *word);
void SortAndPrintArray(SITE* siteArray);
int compareByWordNumOfOccurrences( const void* a, const void* b);


//// program receives 2 arguments: 1.word to search. 2.data file. argv[0] is the name of the program ////
int main(int argNum, char *argv[])
{
    SITE* siteArray;
    siteArray = FillSiteArray(siteArray, argv[2]);
    int fd; // = file-descriptor
    char buffer[BUFFER_SIZE];

    fprintf(stderr, "Calculating...\n");

    int wordCounter = 0;
    for (int site = 0; site < numOfSites; ++site)
    {
        fd = getConnectedSocket(siteArray[site].url, portNum);

        // get the root file from the site
        write(fd, "GET /\r\n", strlen("GET /\r\n"));

        bzero(buffer, BUFFER_SIZE);

        // fill the buffer
        read(fd, buffer, BUFFER_SIZE - 1);

        wordCounter = CountWordOccurrencesInSite(buffer, argv[1]); // argv[1] = keyword
        siteArray[site].wordNumOfOccurrences = wordCounter;


        bzero(buffer, BUFFER_SIZE);
    }

    SortAndPrintArray(siteArray);
    free(siteArray);

    shutdown(fd, SHUT_RDWR);
    close(fd);

    return 0;
}

SITE* FillSiteArray(SITE* siteArray, char* fname)
{
    numOfSites = 0;
    int sizeArr = 10;
    siteArray = malloc(sizeArr*sizeof(SITE));

    FILE *dataFile = fopen(fname, "r");
    if (dataFile == NULL) {
        fprintf(stderr, "Failed opening the file. Exiting!\n");
        exit(1);
    }
    // scan the following host address, exit loop after scanning last string
    while (fscanf(dataFile, "%s\n", siteArray[numOfSites].url) == 1)
    {
        if(numOfSites == sizeArr)
        {
            siteArray = realloc(siteArray, numOfSites*2*sizeof(SITE));
            sizeArr *= 2;
        }

        numOfSites++;
    }

    fclose(dataFile);
    return siteArray;

}

int CountWordOccurrencesInSite(char *buffer, char *word)
{
    int wordCounter = 0;
    char *tmp = buffer;
    while(tmp = strstr(tmp, word))
    {
        wordCounter++;
        tmp += strlen(word);
    }
    return wordCounter;
}

void SortAndPrintArray(SITE* siteArray)
{
    qsort(siteArray, (size_t) numOfSites, sizeof(SITE), compareByWordNumOfOccurrences );

    for (int site = 0; site < numOfSites; ++site)
    {
        fprintf(stderr, "%s  ", siteArray[site].url);
        fprintf(stderr, "%i\n", siteArray[site].wordNumOfOccurrences);
    }
}

int compareByWordNumOfOccurrences( const void* s1, const void* s2)
{
    SITE *site1= (SITE *)s1;
    SITE *site2 = (SITE *)s2;


    if ( site1->wordNumOfOccurrences == site2->wordNumOfOccurrences )
        return 0;
    else if ( site1->wordNumOfOccurrences < site2->wordNumOfOccurrences )
        return 1;
    else
        return -1;
}



int getConnectedSocket(char *host, in_port_t port)
{
    struct hostent *hp;
    struct sockaddr_in addr;
    int on = 1, sock;

    if((hp = gethostbyname(host)) == NULL)
    {
        herror("gethostbyname");
        exit(1);
    }
    bcopy(hp->h_addr, &addr.sin_addr, (size_t) hp->h_length);
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));
    if(sock == -1)
    {
        perror("setsockopt");
        exit(1);
    }
    if(connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1)
    {
        perror("connect");
        exit(1);
    }
    return sock;
}
