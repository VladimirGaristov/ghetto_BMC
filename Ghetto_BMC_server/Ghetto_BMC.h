#define AES256 1
#define BLOCK_SIZE 16

#define SERVER_PORT 420

#define HELLO_PACKET 1
#define NONCE_PACKET 2
#define COMMAND_PACKET 3
#define SUCCESS_PACKET 4
#define ERROR_PACKET 5

#define STATUS 0
#define START 1
#define SHUTDOWN 2
#define REBOOT 3

#define WRONG_PKT_TYPE 91
#define WRONG_NONCE 92
#define INVALID_CMD 93
#define PWR_ON 101
#define PWR_OFF 100
#define CMD_EXECUTED 102

#define RST_PIN 2         //D3  GREEN
#define PWR_PIN 0         //D4  YELLOW
#define V5_SENSE_PIN 14   //D5  BROWN

#define RAND_MAX 2147483646

const char* ssid     = "Garistovi_1";
const char* password = "aztozjuk%_%";
const char* aes_key = "notmyactualkeyduhiaintthatdumb:)";
