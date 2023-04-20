#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>
#define STDIN 0
#define _OPEN_SYS_ITOA_EXT
#define TTL 6
#define VERSION "8"
#define MAXSEQNUMBER 100
#define DEBUG 0
#define MAXMESSAGESSTORED 5
#define TIMEOUT 20

int ttl = TTL;
int configNum=0;


// toPort:50015 msg:"hello 15" version:8
// msg:"hello 15" toPort:50015 version:8
// toPort:50007 move:7 version:8

struct configNode {
  char ip[20];
  int port;
  int location;
  int seqNumber;
  bool hasACKed[MAXSEQNUMBER];
  bool sentACK[MAXSEQNUMBER];
};
struct node {
  char key[50];
  char value[50];
  struct node *next;
};

struct messageDetails {
  char messageString[1000];
  int ttl;
  int seqNum;
  int toPort;
  int fromPort;
};

struct configNode configNodes[50]; /* array of structs holding config info */
struct node *head = NULL;

/******************************************************************************************************************************************************************************/
/*                                                                                                                                                                            */
/*                                                                                                                                                                            */
/*                                                                                                                                                                            */
/*                                                                                                                                                                            */
/*                                                                     method prototypes                                                                                      */
/*                                                                                                                                                                            */
/*                                                                                                                                                                            */
/*                                                                                                                                                                            */
/*                                                                                                                                                                            */
/******************************************************************************************************************************************************************************/
/******************************************************************************************************************************************************************************/
/*                                                                    Linked list methods                                                                                     */
/******************************************************************************************************************************************************************************/
void addToLinkedListFront(char k[50], char v[50]);
int findPositionInLinkedList(char k[50]);
void removeFromLinkedList(char k[50]);
void printLinkedList(int myLoc);
void reverseLinkedList(struct node **headAddress);

/******************************************************************************************************************************************************************************/
/*                                                          Checking received message methods                                                                                 */
/******************************************************************************************************************************************************************************/
int checkDistance(int loc1, int loc2, int rows, int columns);
int protocolCheck(char *myPortStr, int myLocation, int *senderLocation, int *distance, int rows, int cols);
int findToPortLL();
void getMessageDetailsFromLinkedList(int *ttlVal, int *fromPort, int *seqNum);
int foundTypeACK();
bool isMoveMessage(int *newLoc);
int storeSendPath(int ports[50]);
void addPortToSendPath(int myPort);
bool checkIfACKedPreviousSeqNum(int fromPort, int seqNum);
bool isDuplicateMessage(int fromPort, int seqNum, int ACKFlag);
void printMessageForDuplicate(int fromPort, int seqNum, int ACKFlag);
void updateSeqNumberRecords(int fromPort, int seqNum);
void updateSentACKRecords(int fromPort, int seqNum);
void updateHasACKedRecords(int fromPort, int seqNum);


/******************************************************************************************************************************************************************************/
/*                                                                 Param checking methods                                                                                     */
/******************************************************************************************************************************************************************************/
void checkNumParams(int argc);
void checkValidIP(char *ip_addr);
void checkPortNum(char *input, int *portNumber);
void checkFilename(FILE **fptr, char filename[]);
void checkRowsAndCols(int *rows, int *columns, int myLocation);

/******************************************************************************************************************************************************************************/
/*                                                                         Socket methods                                                                                     */
/******************************************************************************************************************************************************************************/
int createSocket();
void fillServerAddress1(int portNumber, struct sockaddr_in *server_address);
void fillServerAddress(int portNumber, struct sockaddr_in *server_address, char *ip);
void bindToServerAddress(int sd, struct sockaddr_in *server_address);
void sendMessage(int sd, char bufferOut[], struct sockaddr_in *server_address);
void sendToConfigList(char *message, int myPort, int sd, struct sockaddr_in *server_address);
void receiveMessage(int sd, char bufferReceived[], int flags, struct sockaddr_in *from_address);
void getMessageToForward(int myLoc, char messageToSend[]);
void forwardMessage(char *message, int sendPathPorts[50], int sendPathSize, int sd, struct sockaddr_in *server_address);
int findToLocation(int fromPort);
void sendACK(int myLoc, int myPort, int fromPort, struct messageDetails arr[], int *numMessages, int sd, struct sockaddr_in *server_address);

/******************************************************************************************************************************************************************************/
/*                                                                        Parsing methods                                                                                     */
/******************************************************************************************************************************************************************************/
int getCharIndex(char c, char **ptr, char string[]);
int getMsgContents(char bufferReceived[], char msgContent[50]);
void resetKeyValue(int *k, int *v, char tempKey[], char tempValue[]);
void setFlagTo1(int *flag);
void setFlagTo0(int *flag);
void parseFullMessage(char bufferReceived[]);
void addMyLocationToMessage(int loc);
int findToPort(char bufferReceived[]);
void findMessageDetailsFromInputString(char bufferReceived[], int *seqNum, int *toPort, int *fromPort);
void appendKeyAndIntToString(char str[], char key[], int value);
void appendSeqNumToString(char str[]);
void prepareNewMessage(char str[], int myPort, int loc);

/******************************************************************************************************************************************************************************/
/*                                                                Saved messages methods                                                                                      */
/******************************************************************************************************************************************************************************/
// int getNumElementsInMsgArray(struct messageDetails arr[]);
void clearMessageDetailsStruct(struct messageDetails arr[], int i);
void initArrayWithEmptyMessages(struct messageDetails arr[]);
int findEmptySlot(struct messageDetails arr[], int numMessages);
void storeMessage(char msg[1000], struct messageDetails arr[], int *numMessages);
bool messageAlreadyStored(struct messageDetails arr[], int seqNum, int toPort, int fromPort);
void sendAllStoredMessages(struct messageDetails arr[], int *numMessages, int myPort, int sd, struct sockaddr_in *server_address);
void forwardAllStoredMessages(struct messageDetails arr[], int *numMessages, int sendPathPorts[50], int sendPathSize, int sd, struct sockaddr_in *server_address);


/******************************************************************************************************************************************************************************/
/*                                                              Reading from file methods                                                                                     */
/******************************************************************************************************************************************************************************/
void checkLineReadError(char fileType[], int error);
void setUpConfig(FILE **fptr, int myPortNumber, int *myLocation);


/******************************************************************************************************************************************************************************/
/*                                                                                                                                                                            */
/*                                                                                                                                                                            */
/*                                                                                                                                                                            */
/*                                                                                                                                                                            */
/*                                                                           main method                                                                                      */
/*                                                                                                                                                                            */
/*                                                                                                                                                                            */
/*                                                                                                                                                                            */
/*                                                                                                                                                                            */
/******************************************************************************************************************************************************************************/
int main(int argc, char *argv[])
{
  head = NULL;
  // Socket variables
  int sd; /* socket descriptor */
  struct sockaddr_in server_address; /* my address */
  struct sockaddr_in current_server_address; /* address of server when iterating through config file */
  struct sockaddr_in from_address;  /* address of sender */
  int myPortNumber = 0; /* provided by the user on the command line */

  // Configuration variables
  FILE *configPtr;
  char configFilename[] = "config.file";
  // int configFileReadError = 1; /* flag for error reading file */
  int myLocation; /* determined from config file when matching port is found */
  int myMovedLocation = -1;

  // Variables for sending messages
  size_t lineSize = 32;
  char *line = (char *)malloc(lineSize * sizeof(char)); /* where input from console is stored */
  char lineBuffer[1000]; /* char array for storing input from console */
  
  // Message storage variables
  struct messageDetails messagesToSend[MAXMESSAGESSTORED];
  struct messageDetails messagesToForward[MAXMESSAGESSTORED];
  int messagesToSendCount = 0;
  int messagesToForwardCount = 0;
  
  // Timer variables
  struct timeval timeout;
  
  // Variables for receiving messages
  char bufferReceived[1000]; // used in recvfrom
  int flags = 0; // used for recvfrom
  int forMe = 0; /* flag used to determine if message was intended for this server */
  fd_set socketFDS;
  int maxSD; /* tells the OS how many sockets are set */
  int rc;

  // Info from received messages
  int senderLocation;
  int fromPort;
  int receivedSeqNum;
  int sendPathPorts[50];
  int sendPathSize = 0;
  char messageToFwd[1000];
  int ackFlag;

  // Grid variables
  int rows;
  int columns;
  int distance;

  /******************************************************************************************************************************************************************************/
  /*                                                                 Initial set up, drone server                                                                               */
  /******************************************************************************************************************************************************************************/
  /* Check params */
  checkNumParams(argc);
  checkPortNum(argv[1], &myPortNumber); /* stores first/only input as portNumber */
  printf("my port number: %i\n", myPortNumber);
  rows = atoi(argv[2]);
  columns = atoi(argv[3]);

  /* Create a socket */
  sd = createSocket();
  /* Fill in the address data structure we use to sendto the server */
  fillServerAddress1(myPortNumber, &server_address);
  /* Bind to the address */
  bindToServerAddress(sd, &server_address);

  /******************************************************************************************************************************************************************************/
  /*                                                                   Read from config file                                                                                    */
  /******************************************************************************************************************************************************************************/
  checkFilename(&configPtr, configFilename);
  setUpConfig(&configPtr, myPortNumber, &myLocation);

  printf("current location: %i\n\n", myLocation);
  checkRowsAndCols(&rows, &columns, myLocation);

  /******************************************************************************************************************************************************************************/
  /*                                                Wait for input either through network or through console                                                                    */
  /******************************************************************************************************************************************************************************/
  initArrayWithEmptyMessages(messagesToSend);
  initArrayWithEmptyMessages(messagesToForward);
  printf("Enter a message you'd like to send: \n");
  while(1) {
    forMe = 0; /* reset flag before receiving new message */
    FD_ZERO(&socketFDS);
    FD_SET(sd, &socketFDS);
    FD_SET(STDIN, &socketFDS);
    if (STDIN > sd) {
      maxSD = STDIN;
    } else {
      maxSD = sd;
    }

    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;
    rc = select(maxSD+1, &socketFDS, NULL, NULL, &timeout); /* block until something arrives, returns rc */
    #ifdef DEBUG
      printf ("select popped rc is %d\n\n", rc);
    #endif

    if (rc == 0){ // had a timeout!
      //reSend(&messages,sd, Partners, myPort, myLoc);
      sendAllStoredMessages(messagesToSend, &messagesToSendCount, myPortNumber, sd, &current_server_address);
      // forwardAllStoredMessages(messagesToForward, &messagesToForwardCount, sendPathPorts, sendPathSize, sd, &current_server_address);
      printf ("timeout!\n");
      /* prompt user for input*/
      printf("----------------------------------------------------\n");
      printf("Enter a message you'd like to send: \n");
      continue; // start while loop over again
    }
    fflush(stdout);

    /***************************************************** Input from console *****************************************************************/
    if (FD_ISSET(STDIN, &socketFDS)) { 
      printf("RECEIVED INPUT FROM COMMAND LINE\n");

      memset (lineBuffer, 0, 1000);
      /* get input from command line */
      getline(&line, &lineSize, stdin); 
      strcpy(lineBuffer, line);
      lineBuffer[strcspn(lineBuffer, "\n")] = 0;

      /* prep message to send*/
      prepareNewMessage(lineBuffer, myPortNumber, myLocation); // append fromPort, location, TTL, send-path, and seqNumber
      storeMessage(lineBuffer, messagesToSend, &messagesToSendCount);
      // printf("line buffer: '%s'\n", lineBuffer);

      /* Send the message, exit if error occurs */
      sendToConfigList(lineBuffer, myPortNumber, sd, &current_server_address);
    }
    
    /********************************************************** Input from client *****************************************************************/
    if (FD_ISSET(sd,&socketFDS)) { 
      printf("RECEIVED INPUT FROM DRONE\n");
      memset(sendPathPorts, 0, 50);
      memset (bufferReceived, 0, 1000); 
      /* Receive message from client */
      receiveMessage(sd, bufferReceived, flags, &from_address);
      printf("received '%s'\n\n", bufferReceived);

      /* Parse message */
      parseFullMessage(bufferReceived);
      forMe = protocolCheck(argv[1], myLocation, &senderLocation, &distance, rows, columns); // argv[1] is myPortNumber as a string
      // printf("forMe: %i\n", forMe);
      getMessageDetailsFromLinkedList(&ttl, &fromPort, &receivedSeqNum); // find the TTL, fromPort, and seqNumber from the message
      
      /* Check if move message */
      if (forMe==1 && isMoveMessage(&myMovedLocation)) {
        myLocation = myMovedLocation;
        printf("My new location: %i\n", myLocation);
        // print message
        printLinkedList(myLocation);
        // resend all messages
        sendAllStoredMessages(messagesToSend, &messagesToSendCount, myPortNumber, sd, &current_server_address);
        forwardAllStoredMessages(messagesToForward, &messagesToForwardCount, sendPathPorts, sendPathSize, sd, &current_server_address);
      }
      /* Print out the message */
      else if (forMe==1 && distance < 9 && ttl >= 0) {
        // addMyLocationToMessage(myLocation);
        ackFlag = foundTypeACK();
        /* If received regular message (non-ACK)*/
        if (ackFlag==0) {
          if (isDuplicateMessage(fromPort, receivedSeqNum, ackFlag)) { // Already received this regular message
            printMessageForDuplicate(fromPort, receivedSeqNum, ackFlag); // ackFlag == 0 for non ack messages
          } else { 
            updateSeqNumberRecords(fromPort, receivedSeqNum);
            printLinkedList(myLocation); // Print this regular message upon first receiving it
          }

          /* Send acknowledgement message back */
          sendACK(myLocation, myPortNumber, fromPort, messagesToSend, &messagesToSendCount, sd, &current_server_address);
          updateSentACKRecords(fromPort, receivedSeqNum);
        } 
        /* If received ACK message*/
        else {
          if (isDuplicateMessage(fromPort, receivedSeqNum, ackFlag)) { // Already received this duplicate ACK message
            printMessageForDuplicate(fromPort, receivedSeqNum, ackFlag); // flag == 1 for ACK messages
          } else {
            printLinkedList(myLocation); // Print this ACK message upon first receiving it
            updateHasACKedRecords(fromPort, receivedSeqNum);
          }
        }
        
      } 
      /* Ignore message if for me but out of range */
      else if (forMe == 1 && distance >= 9) {
        printf("************************************************\n");
        printf("   Ignoring message.\n");
        printf("************************************************\n");
        
      } 
      /* Forward message if not for me but within range */
      else if (forMe != 1 && distance < 9 && ttl > 0) {
        /* Print out ttl */
        ttl -= 1; // decrement ttl
        
        /* Add port number to the send path */
        addPortToSendPath(myPortNumber);
        sendPathSize = storeSendPath(sendPathPorts);

        /* Convert linked list to a string */
        getMessageToForward(myLocation, messageToFwd); // changes sender location to my location
        printf("\nmessage to forward: '%s'\n", messageToFwd);
        // printf("TTL: %i\n", ttl);
        
        /* Store message to forward */
        char messageToFwdString[1000];
        strcpy(messageToFwdString, messageToFwd);
        if (!messageAlreadyStored(messagesToForward, receivedSeqNum, findToPort(messageToFwdString), fromPort)) {
          printf("Not duplicate");
          storeMessage(messageToFwdString, messagesToForward, &messagesToForwardCount);
        } else {
          printf("Duplicate Message found");
        }
        /* Send to all other ports */
        forwardMessage(messageToFwd, sendPathPorts, sendPathSize, sd, &current_server_address);

        printf("************************************************\n");
        printf("   Forwarded message.\n");
        printf("************************************************\n");
      }

      /* reset list */
      head = NULL;
    }

    /* prompt user for input*/
    printf("----------------------------------------------------\n");
    printf("Enter a message you'd like to send: \n");
  } // end of infinite while loop
  
  // close(sd);
  return 0;
} // end of main()


/******************************************************************************************************************************************************************************/
/*                                                                                                                                                                            */
/*                                                                                                                                                                            */
/*                                                                                                                                                                            */
/*                                                                                                                                                                            */
/*                                                                      method definitions                                                                                    */
/*                                                                                                                                                                            */
/*                                                                                                                                                                            */
/*                                                                                                                                                                            */
/*                                                                                                                                                                            */
/******************************************************************************************************************************************************************************/
/******************************************************************************************************************************************************************************/
/*                                                                    Linked list methods                                                                                     */
/******************************************************************************************************************************************************************************/

void addToLinkedListFront(char k[50], char v[50]) {
  int i;
  
  /* create a new node */
  struct node *link = (struct node*) malloc(sizeof(struct node));
  /* fill in the node properties */
  for (i=0; i<50; i++) {
    link->key[i] = k[i];
    link->value[i] = v[i];
  }
  link->next = head; /* have new node point to the last added node */

  head = link; /* have head pointer point to the new node */
}

int findPositionInLinkedList(char k[50]) {
  struct node *ptr = head;
  int i = 0;
  int pos;
  while(ptr!=NULL) {
    if (strcmp(k,ptr->key)==0) {
      pos = i;
    }
    ptr = ptr->next;
    i += 1;
  }
  return pos;
}

void removeFromLinkedList(char k[50]) {
  struct node *ptr = head;
  struct node *prev = head;
  int i;
  int pos;
  pos = findPositionInLinkedList(k);

  for (i=0; i<=pos; i++) {
    if (i==0 && pos==0) {
      head = head -> next;
      free(ptr);
    } else {
      if (i==pos && ptr!=NULL) {
        prev->next = ptr->next;
        free(ptr);
      } else {
        prev = ptr;
        if (prev == NULL) {
          break;
        }
        ptr = ptr->next;
      }
    }
  }
}

void printLinkedList(int myLoc) {
  struct node *ptr = head;
  printf("\n\n************************************************\n\n");
  printf("%20s%20s\n", "Name", "Value");
  while(ptr!=NULL) {
    printf("%20s%20s\n", ptr->key, ptr->value);
    ptr = ptr->next;
  }
  printf("%20s%20i\n", "myLocation", myLoc);
  printf("\n\n************************************************\n\n");
}

void reverseLinkedList(struct node **headAddress) {
   struct node *prev = NULL;
   struct node *curr = *headAddress;
   struct node *next;
	
   while (curr != NULL) {
      next = curr->next; /* advance pre cursor */
      curr->next = prev; /* switch node direction */
      prev = curr; /* advance post cursor */
      curr = next; /* advance current cursor */
   }
	
   *headAddress = prev;
}

/******************************************************************************************************************************************************************************/
/*                                                          Checking received message methods                                                                                 */
/******************************************************************************************************************************************************************************/
int checkDistance(int loc1, int loc2, int rows, int columns) {
  int x1 = (loc1-1) / columns + 1;
  int y1 = (loc1-1) % columns + 1;

  // printf("\nMy coordinates (x,y): (%i,%i)\n", x1, y1);
  int x2 = (loc2-1) / columns + 1;
  int y2 = (loc2-1) % columns + 1;

  // printf("Sender's coordinates (x,y): (%i,%i)\n", x2, y2);
  int distance = (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2);
  printf("Distance: %i\n", distance);
  if (rows*columns < loc2) {
    printf("NOT IN GRID.\n");
  } else {
    if (distance < 9) {
      printf("IN RANGE (distance < 9).\n");
    } else {
      printf("OUT OF RANGE (distance >= 9).\n");
    }
  }
  return distance;
}

int protocolCheck(char *myPortStr, int myLocation, int *senderLocation, int *distance, int rows, int cols) {
  // printf("my port: %s\n", myPortStr);
  int forMe = 0;
  // char *ttlValString = "";
  // int ttlVal = ttl;
  struct node *ptr = head;
  while(ptr!=NULL) {
    if (strcmp("toPort",ptr->key)==0) {
      if (strcmp(myPortStr,ptr->value)==0) {
        forMe = 1;
      }
    } else if (strcmp("version",ptr->key)==0) {
      if (strcmp(VERSION,ptr->value)!=0) {
        printf("Bad version. Expected version %s.\n\n", VERSION);
      }
    } else if(strcmp("location",ptr->key)==0) {
      *senderLocation = atoi(ptr->value);
    } 
    // else if(strcmp("fromPort",ptr->key)==0) {
    //   *fromPort = atoi(ptr->value);
    // } else if (strcmp("TTL",ptr->key)==0) {
    //    ttlValString = (ptr->value);
    //    ttlVal = atoi(ttlValString);
    // }
    ptr = ptr->next;
  }

  *distance = checkDistance(myLocation, *senderLocation, rows, cols);
  return forMe;
}

int findToPortLL() {
  struct node *ptr = head;
  int toPort;
  while(ptr!=NULL) {
    if(strcmp("toPort",ptr->key)==0) {
      toPort = atoi(ptr->value);
    }
    ptr = ptr->next;
  }
  return toPort;
}

void getMessageDetailsFromLinkedList(int *ttlVal, int *fromPort, int *seqNum) {
  char *ttlValString = "";
  struct node *ptr = head;
  while(ptr!=NULL) {
    if(strcmp("fromPort",ptr->key)==0) {
      *fromPort = atoi(ptr->value);
    } else if (strcmp("TTL",ptr->key)==0) {
      ttlValString = (ptr->value);
      *ttlVal = atoi(ttlValString);
    } else if(strcmp("seqNumber",ptr->key)==0) {
      *seqNum = atoi(ptr->value);
    }
    ptr = ptr->next;
  }

  return;
}

int foundTypeACK() {
  struct node *ptr = head;
  int found = 0;
  while(ptr!=NULL) {
    if (strcmp("type",ptr->key)==0 && strcmp("ACK",ptr->value)==0) {
       found = 1;
    }
    ptr = ptr->next;
  }
  return found;
}

bool isMoveMessage(int *newLoc) {
  struct node *ptr = head;
  bool found = false;
  while(ptr!=NULL) {
    if (strcmp("move",ptr->key)==0) {
       found = true;
       *newLoc = atoi(ptr->value); // update the location to move to
    }
    ptr = ptr->next;
  }
  return found;
}

int storeSendPath(int ports[50]) {
  struct node *ptr = head;
  char *restOfPathPtr;
  char restOfPathString[50];
  int i = 0;
  while(ptr!=NULL) {
    if (strcmp("send-path",ptr->key)==0) {
      strcpy(restOfPathString,ptr->value);
      // Add the first port number to the array
      restOfPathPtr = strtok(restOfPathString,",");
      ports[i] = atoi(restOfPathPtr);
      // Add the rest of the port numbers to the array
      while (restOfPathPtr != NULL) {
        i+=1;
        restOfPathPtr = strtok(NULL, ",");
        if (restOfPathPtr != NULL) {
          ports[i] = atoi(restOfPathPtr);
        }
      }
    }
    ptr = ptr->next;
  }
  return i;;
}

void addPortToSendPath(int myPort) {
  struct node *ptr = head;
  char sendPathString[1000];
  char myPortString[50];

  // Convert 'myPort' to a string
  memset (myPortString, 0, 50);
  sprintf(myPortString, "%d", myPort);

  // Find send-path key
  memset (sendPathString, 0, 1000);
  while(ptr!=NULL) {
    if (strcmp("send-path",ptr->key)==0) {
      // Add myPort to the string value
      strcat(ptr->value, ",");
      strcat(ptr->value, myPortString);
    }
    ptr = ptr->next;
  }
  return;
}

bool checkIfACKedPreviousSeqNum(int fromPort, int seqNum) {
  int i;
  bool acked = false;
  for (i=0; i<configNum;i++) {
    if (configNodes[i].port==fromPort) {
      if (configNodes[i].sentACK[seqNum-2] == true) { /* sentACK[0] corresponds to seqNum 1, sentACK[1] corresponds to seqNum 2, etc.*/
        acked = true;
      }      
    }
  }
  return acked;
}

bool isDuplicateMessage(int fromPort, int seqNum, int ACKFlag) {
  int i;
  bool duplicate = false;
  for (i=0; i<configNum;i++) {
    if (configNodes[i].port==fromPort) {
      if (ACKFlag==0) {
        // Check for duplicate non-ACK messages
        if (configNodes[i].sentACK[seqNum-1] == true) {
          duplicate = true;
        }
      } else {
        // Check for duplicate ACK messages
        if (configNodes[i].hasACKed[seqNum-1] == true) {
          duplicate = true;
        }
      }
      
    }
  }
  return duplicate;
}

void printMessageForDuplicate(int fromPort, int seqNum, int ACKFlag) {
  if (ACKFlag==0) {
    printf("************************************************\n");
    printf("   Got duplicate message.\n"); 
    printf("   FromPort: %i\n",fromPort);   
    printf("   Sequence number: %i\n",seqNum);      
    printf("************************************************\n");
  } else {
    printf("************************************************\n");
    printf("   Got duplicate ACK.\n"); 
    printf("   FromPort: %i\n",fromPort);   
    printf("   Sequence number: %i\n",seqNum);      
    printf("************************************************\n");
  }
  
}

void updateSeqNumberRecords(int fromPort, int seqNum) {
  int i;
  for (i=0; i<configNum;i++) {
    if (configNodes[i].port==fromPort) {
      if (seqNum > configNodes[i].seqNumber) {
        configNodes[i].seqNumber = seqNum;
      }
    }
  }
}

void updateSentACKRecords(int fromPort, int seqNum) {
  int i;
  for (i=0; i<configNum;i++) {
    if (configNodes[i].port==fromPort) {
      configNodes[i].sentACK[seqNum-1] = true; /* sentACK[0] corresponds to seqNum 1, sentACK[1] corresponds to seqNum 2, etc.*/
    }
  }
}

void updateHasACKedRecords(int fromPort, int seqNum) {
  int i;
  for (i=0; i<configNum;i++) {
    if (configNodes[i].port==fromPort) {
      configNodes[i].hasACKed[seqNum-1] = true; /* sentACK[0] corresponds to seqNum 1, sentACK[1] corresponds to seqNum 2, etc.*/
    }
  }
}
/******************************************************************************************************************************************************************************/
/*                                                                 Param checking methods                                                                                     */
/******************************************************************************************************************************************************************************/
void checkNumParams(int argc) {
  /* If no parameters are given, provide instructions on what info is expected */
  if (argc < 2){
    printf("usage is: server <portnumber>\n");
    exit (1); /* just leave if wrong number entered */
  }
  return;
}

void checkValidIP(char *ip_addr) {
  struct sockaddr_in inaddr; /* structures for checking addresses */
  /* this code checks to see if the ip address is a valid ip address */
  /* meaning it is in dotted notation and has valid numbers          */
  if (!inet_pton(AF_INET, ip_addr, &inaddr)){  /* uses inaddr to check that inputted ip_address is in correct notation */
    printf ("Error: bad ip address, check dotted notation. \n");
    exit (1); /* just leave if is incorrect */
  }
  // strcpy(serverIP, ip_addr); /* store the inputted ip_address into the string serverIP */
  return;
}

void checkPortNum(char *input, int *portNumber) {
  int i;  /* loop variable */

  /* Check for valid input */
  for (i=0;i<strlen(input); i++){
    if (!isdigit(input[i])) /* check every character in the first/only input to see if it is a digit*/
    {
      printf ("Error: The Portnumber isn't a number! Input must consist of all digits. \n");
      exit(1);
    }
  }

  /* Store input and check if valid port number */
  *portNumber = strtol(input, NULL, 10); /* many ways to do this, but converts string to long integer */
  if ((*portNumber > 65535) || (*portNumber < 0)){
    printf ("Error: you entered an invalid port number; input must be within the range 0-65535. \n");
    exit (1);
  }

  return;
}

void checkFilename(FILE **fptr, char filename[]) {
  *fptr = fopen(filename, "r");
  while (*fptr == NULL) {
    perror("fopen");
    printf("Please try again.\n\n");

    /* Repeatedly ask until receive valid filename */
    printf("Filename: ");
    scanf("%s", filename);
    *fptr = fopen(filename, "r");
  }
  return;
}

void checkRowsAndCols(int *rows, int *columns, int myLocation) {
  while ((*rows)*(*columns) < myLocation) { /* check that my location canfit within the grid */
    printf("Error: location does not fit in the grid given the row and column dimensions. \n");
    printf("Please enter a new number of rows: ");
    scanf("%i", &(*rows));
    printf("Please enter a new number of columns: ");
    scanf("%i", &(*columns));
  }
}

/******************************************************************************************************************************************************************************/
/*                                                                         Socket methods                                                                                     */
/******************************************************************************************************************************************************************************/
int createSocket() {
  int sd = socket(AF_INET, SOCK_DGRAM, 0); /* create a socket using domain: AF_INET, commun. type: SOCK_DGRAM (UDP), protocol: 0 */

  if (sd == -1){ /* check if socket descriptor shows an error */
    perror ("Error: Unable to create socket. \n");
    exit(1); /* just leave if wrong number entered */
  }
  return sd;
}

void fillServerAddress1(int portNumber, struct sockaddr_in *server_address) { 
  (*server_address).sin_family = AF_INET; /* use AF_INET addresses */
  (*server_address).sin_port = htons(portNumber); /* attach address to port number */
  (*server_address).sin_addr.s_addr = INADDR_ANY; /* any adapter */
  return;
}

void fillServerAddress(int portNumber, struct sockaddr_in *server_address, char *ip) { 
  (*server_address).sin_family = AF_INET; /* use AF_INET addresses */
  (*server_address).sin_port = htons(portNumber); /* attach address to port number */
  (*server_address).sin_addr.s_addr = inet_addr(ip); /* any adapter */
  return;
}

void bindToServerAddress(int sd, struct sockaddr_in *server_address) { /* bind socket to the address and port number */
  int rc; /* return code */
  rc = bind (sd, (struct sockaddr *)server_address,
	     sizeof(struct sockaddr ));
  
  if (rc < 0){
    perror("Error: Unable to bind server address. \n");
    exit (1);
  }
  return;
}

void sendMessage(int sd, char bufferOut[], struct sockaddr_in *server_address) {
  int rc; /* return code */
  rc = sendto(sd, bufferOut, strlen(bufferOut), 0,
	      (struct sockaddr *)server_address, sizeof(*server_address));
  
  /* Check the RC and figure out what to do if it is 'bad' */
  /* what is a bad RC from sendto? */
  /* sendto() should return the number of bytes sent (i.e., rc should be equal to strlen(bufferOut)) */
  if (rc < strlen(bufferOut)) { 
    perror ("sendto");
    printf ("- leaving, due to socket error on sendto\n");
    exit(1);
  }
  return;
}

void sendToConfigList(char *message, int myPort, int sd, struct sockaddr_in *server_address) {
  int i;
  for (i=0; i<configNum;i++) {
    if (configNodes[i].port != myPort) {
      fillServerAddress(configNodes[i].port, &(*server_address), configNodes[i].ip);
      sendMessage(sd, message, &(*server_address));
    }
  }
}

void receiveMessage(int sd, char bufferReceived[], int flags, struct sockaddr_in *from_address) {
  int rc; /* return code */
  socklen_t fromLength; /* length of from_address */
  
  fromLength = sizeof(*from_address); /* NOTE - you MUST MUST MUST give fromLength an initial value */
  rc = recvfrom(sd, bufferReceived, 1000, flags, 
		(struct sockaddr *)from_address, &fromLength); /* retrieve message from client */
  /* use the return code to check for any possible errors */
  if (rc <=0){
    perror ("recvfrom");
    printf ("- leaving, due to socket error on recvfrom\n");
    exit (1);
  }

  if('\n' == bufferReceived[strlen(bufferReceived) - 1])
        bufferReceived[strlen(bufferReceived) - 1] = '\0';
  return;
}

void getMessageToForward(int myLoc, char messageToSend[]) {
  struct node *ptr = head;
  char key[50];
  char value[50];

  memset (messageToSend, 0, 1000);
  while(ptr!=NULL) {
    if(strcmp("location",ptr->key)==0) {
      sprintf(ptr->value, "%d", myLoc);
    } else if (strcmp("TTL",ptr->key)==0) {
      sprintf(ptr->value, "%d", ttl); 
    }

    memset (key, 0, 50);
    strcpy(key, ptr->key);
    strcat(key,":");

    memset (value, 0, 50);
    strcpy(value, ptr->value);
    strcat(value," ");

    // append key:value pair to string
    strcat(messageToSend, key);
    strcat(messageToSend, value);
    ptr = ptr->next;
  }
  if(' ' == messageToSend[strlen(messageToSend) - 1])
    messageToSend[strlen(messageToSend) - 1] = '\0';
  return;
}

void forwardMessage(char *message, int sendPathPorts[50], int sendPathSize, int sd, struct sockaddr_in *server_address) {
  int i;
  int j;
  bool inSendPath;
  
  for (i=0; i<configNum;i++) {
    inSendPath = false;
    // Check if the drone's port number is already in the send-path
    for (j=0; j<sendPathSize; j++) {
      if (configNodes[i].port == sendPathPorts[j]) {
        inSendPath = true;
      }
    }
    // If not in the send-path, forward the message to this drone
    if (!inSendPath) {
      // printf("sending to: %i\n", configNodes[i].port);
      fillServerAddress(configNodes[i].port, &(*server_address), configNodes[i].ip);
      sendMessage(sd, message, &(*server_address));
    }
    printf("\n\n");
  }
}

int findToLocation(int fromPort) {
  int i;
  int toLoc;
  for (i=0; i<configNum;i++) {
    if (configNodes[i].port != fromPort) {
      toLoc = configNodes[i].location;
    }
  }
  return toLoc;
}

void sendACK(int myLoc, int myPort, int fromPort, struct messageDetails arr[], int *numMessages, int sd, struct sockaddr_in *server_address) {
  char messageToSendBack[1000];

  /* Convert integers to strings */
  char myPortString[20];
  memset (myPortString, 0, 20);
  sprintf(myPortString, "%d", myPort);
  char myLocString[20];
  memset (myLocString, 0, 20);
  sprintf(myLocString, "%d", myLoc);
  char fromPortString[20];
  memset (fromPortString, 0, 20);
  sprintf(fromPortString, "%d", fromPort);

  struct node *ptr = head;
  while(ptr!=NULL) {
    if(strcmp("send-path",ptr->key)==0) {
      /* Reset send-path and add myPort as the first entry of the path */
      memset(ptr->value, 0, 50);
      strcat(ptr->value, myPortString);
    } else if (strcmp("TTL",ptr->key)==0) {
      /* Reset TTL to 6, but subtract 1 as it is sent out */
      ttl = TTL-1;
      sprintf(ptr->value, "%d", ttl); 
    } else if (strcmp("toPort",ptr->key)==0) {
      /* Replace toPort value with fromPort value*/
      memset(ptr->value, 0, 50);
      strcat(ptr->value, fromPortString);
    } else if (strcmp("fromPort",ptr->key)==0) {
      /* Replace fromPort value with toPort value*/
      memset(ptr->value, 0, 50);
      strcat(ptr->value, myPortString);
    } 
    // else if (strcmp("myLocation",ptr->key)==0) {
    //   removeFromLinkedList(ptr->key);
    // }
    ptr = ptr->next;
  }

  addToLinkedListFront("type", "ACK");

  /* Convert linked list into string */
  getMessageToForward(myLoc, messageToSendBack);
  /* Store ACK message */
  storeMessage(messageToSendBack, arr, numMessages);

  printf("acknowledgement message: '%s'\n", messageToSendBack);

  /* Send to all other drones */
  sendToConfigList(messageToSendBack, myPort, sd, server_address);
  return;
}

/******************************************************************************************************************************************************************************/
/*                                                                        Parsing methods                                                                                     */
/******************************************************************************************************************************************************************************/
int getCharIndex(char c, char **ptr, char string[]) {
  *ptr = strchr(string, c);
  int index = ((*ptr) == NULL ? -1 : (*ptr) - string); /* if the char is present, return the difference of (the address of the char) - (address of first char in the string) */
  return index;
}

int getMsgContents(char bufferReceived[], char msgContent[50]) {
  int i;
  char *strAfterQM1; /* remaining string after first occurence of quotation mark */
  char *strAfterQM2;
  char afterQM1[1000]; /* string after first quotation mark */

  memset (afterQM1, 0, 1000);
  memset (msgContent, 0, 50);
  int qm1Index = getCharIndex('\"', &strAfterQM1, bufferReceived); /* look for first index/occurrence of quotation mark */
  if (qm1Index != -1) {
    strcpy(afterQM1, strAfterQM1+1);
  }
  int qm2Index = getCharIndex('\"', &strAfterQM2, afterQM1);
  
  if (qm1Index != -1 && qm2Index != -1) {
    msgContent[0] = '\"';
    for (i=1; i<qm2Index+1; i++) { /* copy all characters after the first quotation mark, up till the second quotation mark */
      msgContent[i] = afterQM1[i-1];
    }
    msgContent[i] = '\"';
  }
  return qm1Index;
}

void resetKeyValue(int *k, int *v, char tempKey[], char tempValue[]) {
  memset (tempKey, 0, 50);
  memset (tempValue, 0, 50);
  *k = 0; *v = 0;
}

void setFlagTo1(int *flag) {
  if (*flag==0) {
    *flag = 1;
  } else {
      printf("Error: missing value before deliminator ' '\n");
  }
}

void setFlagTo0(int *flag) {
  if (*flag==1) {
    *flag = 0;
  }
  //  else {
  //   printf("Error: missing key before deliminator ':'\n");
  // }
}

void parseFullMessage(char bufferReceived[]) {
  int i; int j;
  int k=0; int v=0;
  
  char tempKey[50]; /* key extracted from key:value pair */
  char tempValue[50]; /* value extracted from key:value pair */
  int buildingKey = 1;

  char msgContent[50];
  int msgStartIndex;

  msgStartIndex = getMsgContents(bufferReceived, msgContent);
  // printf("msgContent: '%s'\n", msgContent);

  memset (tempKey, 0, 50);
  memset (tempValue, 0, 50);
  for (i=0;i<strlen(bufferReceived);i++) {
    // Reached msg contents; parse separately
    if (i==msgStartIndex) {
      memset (tempValue, 0, 50);
      for (j=0; j<strlen(msgContent); j++) {
        tempValue[j] = msgContent[j];
      }
      addToLinkedListFront(tempKey, tempValue);

      resetKeyValue(&k, &v, tempKey, tempValue);
      setFlagTo1(&buildingKey); // indicate that the key needs to be parsed next
      i += strlen(msgContent);
    } 
    // Parse
    else {
      // check for end of key
      if (bufferReceived[i]==':') { 
        setFlagTo0(&buildingKey); // indicate that the value needs to be parsed next
      } 
      // check for end of key:value pair
      else if (bufferReceived[i]==' ') { 
        // printf("key: '%s'\n", tempKey);
        // printf("value: '%s'\n", tempValue);
        addToLinkedListFront(tempKey, tempValue);
        
        resetKeyValue(&k, &v, tempKey, tempValue);
        setFlagTo1(&buildingKey); // indicate that the key needs to be parsed next
      } 
      // build the key/value
      else { 
        if (buildingKey==1) {
          tempKey[k] = bufferReceived[i];
          k++;
        } else {
          tempValue[v] = bufferReceived[i];
          v++;
        }
        
      }
    }
  }

  if (strcmp(tempKey,"\0")!=0 && strcmp(tempValue,"\0")!=0) {
    addToLinkedListFront(tempKey, tempValue);
  }
  
  return;
}

void addMyLocationToMessage(int loc) {
  char locString[20];
  memset (locString, 0, 20);
  sprintf(locString, "%d", loc);
  addToLinkedListFront("myLocation",locString);
  return;
}

int findToPort(char bufferReceived[]) {
  int i,j;
  int k=0; int v=0;
  int toPortValue;

  char tempKey[50]; /* key extracted from key:value pair */
  char tempValue[50]; /* value extracted from key:value pair */
  int buildingKey = 1;

  char msgContent[50];
  int msgStartIndex;

  msgStartIndex = getMsgContents(bufferReceived, msgContent);

  memset (tempKey, 0, 50);
  memset (tempValue, 0, 50);
  for (i=0;i<strlen(bufferReceived);i++) {
    // Reached msg contents; skip over it
    if (i==msgStartIndex) {
      memset (tempValue, 0, 50);
      for (j=0; j<strlen(msgContent); j++) {
        tempValue[j] = msgContent[j];
      }

      resetKeyValue(&k, &v, tempKey, tempValue);
      setFlagTo1(&buildingKey); // indicate that the key needs to be parsed next
      i += strlen(msgContent);
    } 
    else {
      // check for end of key
      if (bufferReceived[i]==':') { 
        setFlagTo0(&buildingKey); // indicate that the value needs to be parsed next
      } 
      // check for end of key:value pair
      else if (bufferReceived[i]==' ') { 
        if (strcmp(tempKey,"toPort")==0) {
          toPortValue = atoi(tempValue);
        }
        
        resetKeyValue(&k, &v, tempKey, tempValue);
        setFlagTo1(&buildingKey); // indicate that the key needs to be parsed next
      } 
      // build the key/value
      else { 
        if (buildingKey==1) {
          tempKey[k] = bufferReceived[i];
          k++;
        } else {
          tempValue[v] = bufferReceived[i];
          v++;
        }
        
      }
    }
  }
  return toPortValue;
}

void findMessageDetailsFromInputString(char bufferReceived[], int *seqNum, int *toPort, int *fromPort) {
  int i;
  int j;
  int k=0; int v=0;

  char tempKey[50]; /* key extracted from key:value pair */
  char tempValue[50]; /* value extracted from key:value pair */
  int buildingKey = 1;

  char msgContent[50];
  int msgStartIndex;

  msgStartIndex = getMsgContents(bufferReceived, msgContent);

  memset (tempKey, 0, 50);
  memset (tempValue, 0, 50);
  for (i=0;i<strlen(bufferReceived);i++) {
    // Reached msg contents; skip over it
    if (i==msgStartIndex) {
      memset (tempValue, 0, 50);
      for (j=0; j<strlen(msgContent); j++) {
        tempValue[j] = msgContent[j];
      }

      resetKeyValue(&k, &v, tempKey, tempValue);
      setFlagTo1(&buildingKey); // indicate that the key needs to be parsed next
      i += strlen(msgContent);
    } 
    else {
      // check for end of key
      if (bufferReceived[i]==':') { 
        setFlagTo0(&buildingKey); // indicate that the value needs to be parsed next
      } 
      // check for end of key:value pair
      else if (bufferReceived[i]==' ') { 
        if (strcmp(tempKey,"toPort")==0) {
          *toPort = atoi(tempValue);
        } else if (strcmp(tempKey,"fromPort")==0) {
          *fromPort = atoi(tempValue);
        } else if (strcmp(tempKey,"seqNumber")==0) {
          *seqNum = atoi(tempValue);
        }
        
        resetKeyValue(&k, &v, tempKey, tempValue);
        setFlagTo1(&buildingKey); // indicate that the key needs to be parsed next
      } 
      // build the key/value
      else { 
        if (buildingKey==1) {
          tempKey[k] = bufferReceived[i];
          k++;
        } else {
          tempValue[v] = bufferReceived[i];
          v++;
        }
        
      }
    }
  }

  // Check last key:value pair
  if (strcmp(tempKey,"toPort")==0) {
    *toPort = atoi(tempValue);
  } else if (strcmp(tempKey,"fromPort")==0) {
    *fromPort = atoi(tempValue);
  } else if (strcmp(tempKey,"seqNumber")==0) {
    *seqNum = atoi(tempValue);
  }
  return;
}

void appendKeyAndIntToString(char str[], char key[], int value) {
  char myValueString[20];
  memset (myValueString, 0, 20);

  strcat(str,key); // e.g., strcat(str, " TTL:")
  sprintf(myValueString, "%d", value);
  strcat(str,myValueString);
  return;
}

void appendSeqNumToString(char str[]) {
  int i;
  int seqNum;
  char seqNumString[20];
  memset (seqNumString, 0, 20);

  /* update the seq number */
  for (i=0; i<configNum;i++) {
    if (configNodes[i].port==findToPort(str)) {
      // configNodes[i].sentACK[configNodes[i].seqNumber] = true; /* sentACK[0] corresponds to seqNum 1, sentACK[1] corresponds to seqNum 2, etc.*/
      configNodes[i].seqNumber += 1;
      seqNum = configNodes[i].seqNumber;
    }
  }
  strcat(str," seqNumber:");
  sprintf(seqNumString, "%d", seqNum);
  strcat(str,seqNumString);
  return;
}

void prepareNewMessage(char str[], int myPort, int loc) {
  ttl = TTL - 1;
  appendKeyAndIntToString(str, " fromPort:", myPort);
  appendKeyAndIntToString(str, " location:", loc);
  appendKeyAndIntToString(str, " TTL:", ttl);
  appendKeyAndIntToString(str, " send-path:", myPort);
  appendSeqNumToString(str);
  return;
}

/******************************************************************************************************************************************************************************/
/*                                                                Saved messages methods                                                                                      */
/******************************************************************************************************************************************************************************/
// int getNumElementsInMsgArray(struct messageDetails arr[]) {
//   return sizeof(arr[0])/sizeof(arr);
// }
void clearMessageDetailsStruct(struct messageDetails arr[], int i) {
  strcpy(arr[i].messageString,"");
  arr[i].ttl = 0;
  arr[i].seqNum = 0;
  arr[i].toPort = 0;
  arr[i].fromPort = 0;
  return;
}
void initArrayWithEmptyMessages(struct messageDetails arr[]) {
  int i;
  for (i=0;i<MAXMESSAGESSTORED;i++) {
    clearMessageDetailsStruct(arr, i);
  }
}
int findEmptySlot(struct messageDetails arr[], int numMessages) {
  int i;
  int pos = -1;
  if (numMessages<MAXMESSAGESSTORED) {
    for (i=0;i<MAXMESSAGESSTORED;i++) {
      if (arr[i].ttl == 0) {
        pos = i;
        break;
      }
    }
  }
  return pos;
}

void storeMessage(char msg[1000], struct messageDetails arr[], int *numMessages) {
  int emptySlot = findEmptySlot(arr, *numMessages);
  if ((*numMessages)>MAXMESSAGESSTORED || emptySlot < 0) {
      printf("Storage full. Unable to store message.\n");
  } else {
    // Copy the message into the char array in the struct
    strcpy(arr[emptySlot].messageString,msg);
    arr[emptySlot].ttl = 5;
    findMessageDetailsFromInputString(msg,&(arr[emptySlot].seqNum), &(arr[emptySlot].toPort), &(arr[emptySlot].fromPort));
    *numMessages+=1;
  }
  return;
}

bool messageAlreadyStored(struct messageDetails arr[], int seqNum, int toPort, int fromPort) {
  bool duplicate = false;
  int i;
  // Check every stored message
  for (i=0; i<MAXMESSAGESSTORED; i++) {
    // Duplicate message is same seq#, same toPort, same fromPort
    if (arr[i].seqNum==seqNum && arr[i].toPort==toPort && arr[i].fromPort==fromPort) {
      duplicate = true;
    }
  }
  return duplicate;
}

void sendAllStoredMessages(struct messageDetails arr[], int *numMessages, int myPort, int sd, struct sockaddr_in *server_address) {
  int j;
  // Check every stored message
  for (j=0; j<MAXMESSAGESSTORED; j++) {
    printf("(sending) j is %i\n", j);
    if (arr[j].ttl>0) {
      // Send to all drones
      sendToConfigList(arr[j].messageString, myPort, sd, server_address);
      printf("message sent with ttl as %i\n", arr[j].ttl);
      arr[j].ttl -= 1;
    } else {
      // Remove message from storage
      clearMessageDetailsStruct(arr, j);
      *numMessages -= 1;
    }
  }
  return;
}

void forwardAllStoredMessages(struct messageDetails arr[], int *numMessages, int sendPathPorts[50], int sendPathSize, int sd, struct sockaddr_in *server_address) {
  int j;
  // Check every stored message
  for (j=0; j<MAXMESSAGESSTORED; j++) {
    printf("(forwarding) j is %i\n", j);
    if (arr[j].ttl>0) {
      // Send to relevant drones
      forwardMessage(arr[j].messageString, sendPathPorts, sendPathSize, sd, server_address);
      printf("message forwarded with ttl as %i\n", arr[j].ttl);
      arr[j].ttl -= 1;
    } else {
      // Remove message from storage
      clearMessageDetailsStruct(arr, j);
      *numMessages -= 1;
    }
  }
  return;
}

/******************************************************************************************************************************************************************************/
/*                                                              Reading from file methods                                                                                     */
/******************************************************************************************************************************************************************************/
void checkLineReadError(char fileType[], int error) {
  if (error) {
    perror("Error: cannot read line ");
    printf("in %s file.\n", fileType);
    exit(1);
  } else {
    printf("Reached end of %s file!\n", fileType);
  }
  return;
}

void setUpConfig(FILE **fptr, int myPortNumber, int *myLocation) {
  int configFileReadError = 1; /* flag for error reading file */
  int c=0; /* counter for number of config lines in config.file */
  int i;
  
  char currentConfig[100]; /* each line in config.file */
  char *currentIPPtr; /* extracted IP address */
  char currentIP[20];
  char *currentPortPtr; /* extracted port number */
  char currentPortString[20];
  int currentPortNumber = 0;
  char *currentLocationPtr; /* extract location */
  int currentLocation = 0;

  memset (currentIP, 0, 20);
  memset (currentPortString, 0, 20);
  /* Go through each IP, portnum, & location combo in the config file */
  while (fgets(currentConfig, 100, *fptr) != NULL) {
    configFileReadError = 0; /* update flag - no error reading file */

    /* Get the IP address */
    currentIPPtr = strtok(currentConfig," ");
    strcpy(currentIP, currentIPPtr);
    checkValidIP(currentIP); 
    /* Get the port num */
    currentPortPtr = strtok(NULL, " ");
    strcpy(currentPortString, currentPortPtr);
    checkPortNum(currentPortString, &currentPortNumber); /* stores second input as currentPortNumber; use argv[2] for testing, currentIP for final version */
    /* Get the location */
    currentLocationPtr = strtok(NULL, " ");
    currentLocation = atoi(currentLocationPtr);
    if (currentPortNumber==myPortNumber) {
      *myLocation = currentLocation;
    }

    /* fill in struct information */
    strcpy(configNodes[c].ip, currentIP);
    configNodes[c].port = currentPortNumber;
    configNodes[c].location = currentLocation;
    configNodes[c].seqNumber = 0;
    for (i=0; i<MAXSEQNUMBER; i++) {
      configNodes[c].hasACKed[i] = false;
    }
    for (i=0; i<MAXSEQNUMBER; i++) {
      configNodes[c].sentACK[i] = false;
    }
    

    c = c+1;
    /* Clear char arrays */
    memset (currentIP, 0, 20);
    memset (currentPortString, 0, 20);
  }
  /* Check if file was never opened vs finished reading file */
  checkLineReadError("configs", configFileReadError);
  configNum = c;
  return;
}