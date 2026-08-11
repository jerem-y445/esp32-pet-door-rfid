/*
 * @file    main.c
 * @author  Jeremy Urena
 * @board   ESP-S3-DevKitC-1 v1.1
 * This is an RTOS-based firmware for authenticating a pet at a door using a PN532 RFID Reader
 * 
 * Main purpose: Get the servo to rotate 90 degrees when card is read. 
 *               Rotate 90 degrees back once IR break beam is unbroken 
 *               and after timer runs out.
 */

 #include "inc/main.h"

/* For IR Beam:
 *      First enable IO MUX for GPIO 6 to be in input mode
 *      Then, read from 6th bit position from GPIO input register
 */
uint32_t volatile * const IR_IO_MUX_GPIO6_REG  = (uint32_t *) (0x60009000 + (0x0004 + 4 * 6));
uint32_t volatile * const IR_GPIO_IN_REG       = (uint32_t *) (0x60004000 + 0x003C);

// Physical Tag UIDs
static const uint32_t UID_VAL = 0x97F6B001;

// ESP Log Tags
static const char *TAG_PN532 = "ntag_read";
static const char *TAG_SERVO = "servo_control";
static const char *TAG_IR    = "break_beam";
// static const char *TAG_MAIN    = "main";

// M996R Servo Config
servo_config_t servo_config = {
    .max_angle      = SERVO_MAX_ANGLE,
    .min_width_us   = SERVO_MIN_WIDTH,
    .max_width_us   = SERVO_MAX_WIDTH,
    .freq           = SERVO_FREQ,
    .timer_number   = SERVO_TIMER_NUM,
    .channels = {
        .servo_pin = {
            SERVO_PIN,
        },
        .ch = {
            SERVO_CHANNEL,
        }
    },
    .channel_number = SERVO_CHANNEL_NUM
};

// Param Inits
rfidParams_t rfid_params = {};
irParams_t ir_params = {};

// Handlers
TaskHandle_t task_rfid_detect_hdl;
TaskHandle_t task_ir_detect_hdl;

// Mutex
SemaphoreHandle_t rfid_hw_mutex;

void app_main() 
{
    printf("APP MAIN\n");

    /*
    * Start Init Section
    */
    
    ir_init(TAG_IR, &ir_params, IR_IO_MUX_GPIO6_REG, &rfid_hw_mutex, IR_GPIO_IN_REG, IR_GPIO_NUM);
    servo_init(TAG_SERVO, &servo_config, SERVO_SPEED_MODE);
    rfid_init(TAG_PN532, &rfid_params, UID_VAL, &rfid_hw_mutex, IR_GPIO_IN_REG);

    /*
    * End Init Section
    */

    rfid_hw_mutex = xSemaphoreCreateMutex();

    xTaskCreate(task_rfid_detect, "RFID Outside Detection Task", 4096, &rfid_params, 5, &task_rfid_detect_hdl);
    xTaskCreate(task_ir_detect, "IR Inside Detection Task", 4096, &ir_params, 5, &task_ir_detect_hdl);
}


/*
 * Things to do:
 *  Add global is_outside flag:         done
 *  Write RFID detection function:      done
 *  Write IR detection function:        done
 *  Write IR wait for cat function:     done
 *  
 *  
 * 

Changing the logic:
 *
 * When cat is outside: RFID -> Unlock door -> Have motion be detected by IR sensor -> Lock door after cat is out of the way and timer runs out
 * 
 * Before cat walks up:
 *  RFID detection function RUNNING: not detected
 *  IR inside_detection function RUNNING: not detected
 *  IR wait_for_cat function NOT RUNNING
 *  Outside_flag = true
 *  Door locked
 * 
 * As cat approaches:
 *  RFID detection function RUNNING: detected
 *  IR inside_detection function NOT RUNNING
 *  IR wait_for_cat function RUNNING: detected
 *  Outside_flag = false
 *  Door unlocked
 * 
 * Right after cat moves away from beam:
 *  RFID detection function RUNNING: not detected
 *  IR inside_detection function NOT RUNNING
 *  IR wait_for_cat function RUNNING: not detected
 *  Timer starts = 5 secs
 *  Outside_flag = false
 *  Door unlocked
 * 
 * After timer runs out:
 *  RFID detection function RUNNING: not detected
 *  IR inside_detection function RUNNING; not detected
 *  IR wait_for_cat function NOT RUNNING
 *  Outside_flag = true
 *  Door locked
 * 
 * **If cat detected before timer runs out:
 *  RFID detection function RUNNING: not detected
 *  IR inside_detection function NOT RUNNING
 *  IR wait_for_cat function RUNNING: not detected
 *  Timer resets = 5 secs
 *  Outside_flag = false
 *  Door unlocked
 * 
 * 
 * 
 * When cat is inside: IR beam broken with NO AUTHENTICATION (since it's not required when he's already inside the house) -> Lock door after cat is out of the way and timer runs out
 * 
 * Before cat walks up:
 *  RFID detection function RUNNING: not detected
 *  IR inside_detection function RUNNING: not detected
 *  IR wait_for_cat function NOT RUNNING
 *  Outside_flag = false
 *  Door locked
 * 
 * As cat approaches:
 *  RFID detection function NOT RUNNING
 *  IR inside_detection function RUNNING: detected
 *  IR wait_for_cat function NOT RUNNING
 *  Outside_flag = true
 *  Door unlocked
 * 
 * Right after cat moves away from beam
 *  RFID detection function NOT RUNNING
 *  IR inside_detection function RUNNING: not detected
 *  IR wait_for_cat function NOT RUNNING
 *  Outside_flag = true
 *  Timer starts = 5 secs
 *  Door unlocked
 * 
 * After timers runs out
 *  RFID detection function RUNNING: not detected
 *  IR inside_detection function RUNNING: not detected
 *  IR wait_for_cat function NOT RUNNING
 *  Outside_flag = true
 *  Door locked
 * 
 * **If cat detected before timer runs out:
 *  RFID detection function RUNNING: not detected
 *  IR inside_detection function NOT RUNNING
 *  IR wait_for_cat function RUNNING: not detected
 *  Timer resets = 5 secs
 *  Outside_flag = true
 *  Door unlocked

*/