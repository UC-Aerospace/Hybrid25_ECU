#include "test_servo.h"
#include "debug_io.h"
#include "can.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// Simple serial command interface over debug_io
// Commands:
//   ARM <mask>        - Arm servos bitmask (lower 4 bits), eg: ARM 0xF
//   DISARM            - Disarm all servos
//   POS <s0> <s1> <s2> <s3>  - Set 4 servo positions (0-255). Use - to keep current
//   HELP              - Show help
// Ex: POS 128 64 255 0
// Ex: ARM 0x3

static uint8_t servo_armed_mask = 0;      // bit i => servo i armed
static uint8_t servo_pos[3] = {0};
static char rx_buf[128];

static int hex_or_dec_to_int(const char *s, int *ok) {
    if(!s){*ok=0;return 0;} *ok=1; 
    if(strncasecmp(s,"0x",2)==0) {
        return (int)strtol(s,NULL,16);
    }
    return (int)strtol(s,NULL,10);
}

static void send_arm_state(void) {
    // commandType = CAN_CMD_SET_SERVO_ARM, command byte = mask (lower 4 bits)
    if(!can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_SERVO, CAN_CMD_SET_SERVO_ARM, (0x0F<<4) | (servo_armed_mask & 0x0F))) {
        dbg_printf("Failed to send ARM command\r\n");
    } else {
        dbg_printf("Sent ARM mask 0x%X\r\n", servo_armed_mask & 0x0F);
    }
}

static void send_positions(void) {
    uint8_t command = (servo_pos[0] & 0x3) << 6 | 
                      (servo_pos[1] & 0x1) << 5 | 
                      (servo_pos[2] & 0x1F);

    if(!can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_SERVO, CAN_CMD_SET_SERVO_POS, command)) {
        dbg_printf("Failed to send POS frame\r\n");
    } else {
        dbg_printf("Set servo %d with mode %d to pos %d (armed mask 0x%X)\r\n", servo_pos[0], servo_pos[1], servo_pos[2], servo_armed_mask & 0x0F);
    }
}

static void cmd_help(void) {
    dbg_printf("Commands:\r\n");
    dbg_printf("  ARM <mask>          Arm servos bit mask (hex or dec)\r\n");
    dbg_printf("  DISARM              Disarm all servos\r\n");
    dbg_printf("  POS <servoID(0-3)> <howSet(0-1)> <pos (0-3 if howSet=0, else 0-20)>\r\n");
    dbg_printf("  HELP                This help\r\n");
}

void test_servo_init(void) {
    dbg_printf("Servo test interface ready. Type HELP for commands.\r\n");
}

void test_servo_poll(void) {
    int n = dbg_recv(rx_buf, sizeof(rx_buf));
    if(n <= 0) return; // no data

    // Trim newline
    for(int i=0;i<n;i++) { if(rx_buf[i]=='\r'||rx_buf[i]=='\n') { rx_buf[i]='\0'; break;} }

    // Tokenize
    char *tok = strtok(rx_buf, " \t");
    if(!tok) return;
    if(strcasecmp(tok, "ARM") == 0) {
        char *maskStr = strtok(NULL, " \t");
        int ok; int val = hex_or_dec_to_int(maskStr, &ok);
        if(!ok) { dbg_printf("Bad mask\r\n"); return; }
        servo_armed_mask = (uint8_t)val;
        send_arm_state();
    } else if(strcasecmp(tok, "DISARM") == 0) {
        servo_armed_mask = 0; send_arm_state();
    } else if(strcasecmp(tok, "POS") == 0) {
        for(int i=0;i<3;i++) {
            char *p = strtok(NULL, " \t");
            if(!p) { dbg_printf("Need 3 values\r\n"); return; }
            if(strcmp(p,"-") == 0) continue; // keep
            int v = atoi(p);
            if(v < 0) v = 0; if(v>20) v=20;
            servo_pos[i] = (uint8_t)v;
        }
        send_positions();
    } else if(strcasecmp(tok, "HELP") == 0) {
        cmd_help();
    } else {
        dbg_printf("Unknown command. Type HELP.\r\n");
    }
}
