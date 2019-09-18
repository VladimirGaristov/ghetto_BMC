#define AES256 1
#define BLOCK_SIZE 16

#define SERVER_PORT 420
#define SOCK_PROTOCOL SOCK_STREAM

#define HELLO_PACKET 1
#define NONCE_PACKET 2
#define COMMAND_PACKET 3
#define SUCCESS_PACKET 4
#define ERROR_PACKET 5

#define WRONG_PKT_TYPE 91
#define WRONG_NONCE 92
#define INVALID_CMD 93
#define PWR_ON 101
#define PWR_OFF 100
#define CMD_EXECUTED 102

#define STATUS 0
#define START 1
#define SHUTDOWN 2
#define REBOOT 3

const char* server_addr = "42.0.69.13";
const char* aes_key = "notmyactualkeyduhiaintthatdumb:)";
