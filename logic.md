
## Things to do
 * Add global is_outside flag:         done
 * Write RFID detection function:      done
 * Write IR detection function:        done
 * Write IR wait for cat function:     done

## Changing the logic

### When cat is outside: RFID -> Unlock door -> Have motion be detected by IR sensor -> Lock door after cat is out of the way and timer runs out
 
 Before cat walks up:
 * RFID detection function RUNNING: not detected
 * IR inside_detection function RUNNING: not detected
 * IR wait_for_cat function NOT RUNNING
 * Outside_flag = true
 * Door locked

As cat approaches:
 * RFID detection function RUNNING: detected
 * IR inside_detection function NOT RUNNING
 * IR wait_for_cat function RUNNING: detected
 * Outside_flag = false
 * Door unlocked

Right after cat moves away from beam:
 * RFID detection function RUNNING: not detected
 * IR inside_detection function NOT RUNNING
 * IR wait_for_cat function RUNNING: not detected
 * Timer starts = 5 secs
 * Outside_flag = false
 * Door unlocked
 * 
After timer runs out:
 * RFID detection function RUNNING: not detected
 * IR inside_detection function RUNNING; not detected
 * IR wait_for_cat function NOT RUNNING
 * Outside_flag = true
 * Door locked
 
**If cat detected before timer runs out:**
 * RFID detection function RUNNING: not detected
 * IR inside_detection function NOT RUNNING
 * IR wait_for_cat function RUNNING: not detected
 * Timer resets = 5 secs
 * Outside_flag = false
 * Door unlocked

### When cat is inside: IR beam broken with NO AUTHENTICATION (since it's not required when he's already inside the house) -> Lock door after cat is out of the way and timer runs out
 
Before cat walks up:
 * RFID detection function RUNNING: not detected
 * IR inside_detection function RUNNING: not detected
 * IR wait_for_cat function NOT RUNNING
 * Outside_flag = false
 * Door locked
 
As cat approaches:
 * RFID detection function NOT RUNNING
 * IR inside_detection function RUNNING: detected
 * IR wait_for_cat function NOT RUNNING
 * Outside_flag = true
 * Door unlocked
 
Right after cat moves away from beam
 * RFID detection function NOT RUNNING
 * IR inside_detection function RUNNING: not detected
 * IR wait_for_cat function NOT RUNNING
 * Outside_flag = true
 * Timer starts = 5 secs
 * Door unlocked
 
After timers runs out
 * RFID detection function RUNNING: not detected
 * IR inside_detection function RUNNING: not detected
 * IR wait_for_cat function NOT RUNNING
 * Outside_flag = true
 * Door locked
 
**If cat detected before timer runs out:**
 * RFID detection function RUNNING: not detected
 * IR inside_detection function NOT RUNNING
 * IR wait_for_cat function RUNNING: not detected
 * Timer resets = 5 secs
 * Outside_flag = true
 * Door unlocked