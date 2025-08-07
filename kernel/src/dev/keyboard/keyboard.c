/**
 * @file keyboard.c
 * @author Zack Bostock
 * @brief Keyboard initialization and helpers
 * @date 2025
 * 
 * @copyright Copyright (c) 2025
 */

#include <stdio.h>
#include <dev/keyboard/keyboard.h>
#include <proc/callback.h>
#include <common/kprint.h>
#include <common/lock.h>

/* Configuration constants */
#define KB_TIMEOUT_CYCLES   (100000)
#define PS2_ACK             (0xFA)
#define PS2_RESEND          (0xFE)

/* Static variables */
static volatile KEYBOARD_MOUSE keyboard = {0};
static KEYBOARD_BUFFER kb_buffer = {0};
static LOCK kb_lock = {0};

/**
 * @brief Safely adds a character to the keyboard buffer
 * 
 * @param key Character to add
 * @return TRUE if successful, FALSE if buffer is full
 */
static uint8_t keyboard_buffer_put(uint8_t key) {
    uint8_t success = FALSE;
    
    LOCK_LOCK(&kb_lock);
    
    if (kb_buffer.buffer_length < KB_BUFF_SIZE) {
        kb_buffer.buffer[kb_buffer.write_index] = key;
        kb_buffer.write_index = (kb_buffer.write_index + 1) % KB_BUFF_SIZE;
        kb_buffer.buffer_length++;
        success = TRUE;
        kb_buffer.overflow_occurred = FALSE;
    } else {
        kb_buffer.overflow_occurred = TRUE;
        /* TODO: Remove from keyboard's buffer rather than from callback. */
        success = TRUE;     /* Remove this when updated. */
        klogd("KEYBOARD: Buffer overflow, dropping keypress\n");
    }
    
    UNLOCK_LOCK(&kb_lock);
    return success;
}

/**
 * @brief Gets a character from the keyboard buffer
 * 
 * @param key Pointer to store the retrieved character
 * @return TRUE if successful, FALSE if buffer is empty
 */
uint8_t keyboard_buffer_get(uint8_t *key) {
    uint8_t success = FALSE;
    
    if (!key) return FALSE;
    
    LOCK_LOCK(&kb_lock);
    
    if (kb_buffer.buffer_length > 0) {
        *key = kb_buffer.buffer[kb_buffer.read_index];
        kb_buffer.read_index = (kb_buffer.read_index + 1) % KB_BUFF_SIZE;
        kb_buffer.buffer_length--;
        success = TRUE;
    }
    
    UNLOCK_LOCK(&kb_lock);
    return success;
}

/**
 * @brief Helper for setting a key state on the keyboard
 *
 * @param state State of key (1 = pressed, 0 = released)
 * @param scancode Scancode to set
 */
void keyboard_set_key(uint8_t state, uint8_t scancode) {
    if (scancode >= MAX_SCANCODES) {
        klogw("KEYBOARD: Invalid scancode: 0x%02X\n", scancode);
        return;
    }
    
    keyboard.key_pressed[scancode] = state;
    
    if (state) {
        keyboard.last_keystroke = scancode;
    } else {
        keyboard.last_keystroke = 0;
    }
    
    /* Update external key state array if provided */
    if (keyboard.ptr_to_update) {
        keyboard.ptr_to_update[scancode] = state;
    }
}

/**
 * @brief Processes special key combinations
 * 
 * @param c Character to process
 * @param scancode Original scancode
 * @return TRUE if special combination was handled
 */
static uint8_t handle_special_keys(char c, uint8_t scancode) {
    (void) scancode;

    /* Handle Ctrl+D (EOF) */
    if (keyboard.key_pressed[LEFT_CONTROL] && (c == 'd' || c == 'D')) {
        if (keyboard_buffer_put(EOF)) {
            cb_publish(0, CB_KEY_PRESS, EOF);
            klogd("KEYBOARD: EOF (Ctrl+D) processed\n");
        }
        return TRUE;
    }
    
    /* Handle Ctrl+C (interrupt) */
    if (keyboard.key_pressed[LEFT_CONTROL] && (c == 'c' || c == 'C')) {
        cb_publish(0, CB_KEY_INTERRUPT, c);
        klogd("KEYBOARD: Interrupt (Ctrl+C) processed\n");
        return TRUE;
    }

    /* Handle Ctrl+L (clear screen) */
    if (keyboard.key_pressed[LEFT_CONTROL] && (c == 'l' || c == 'L')) {
        cb_publish(0, CB_KEY_CLEAR_SCREEN, c);
        /* 
            TODO: Have the terminal subscribe to this somehow
                  and remove this call to terminal code
        */
        terminal_clear(TERM_MODE_TERM);
        klogd("KEYBOARD: Clear screen (Ctrl+L) processed\n");
        return true;
    }
    
    return FALSE;
}

/**
 * @brief Hardware interrupt handler for the keyboard
 * 
 * Handles both key press and release events, processes special
 * key combinations, and manages the keyboard buffer.
 */
static void keyboard_interrupt_handler() {
    uint8_t keycode = inb(PS2_DATA_PORT);
    uint8_t scancode = keycode & 0x7F;
    uint8_t key_state = !(keycode & 0x80);
    
    /* Validate scancode */
    if (scancode >= MAX_SCANCODES) {
        klogw("KEYBOARD: Invalid scancode received: %2x\n", scancode);
        return;
    }
    
    /* Update key state */
    keyboard_set_key(key_state, scancode);
    
    /* Process key press events */
    if (key_state) {
        char c = get_character_from_scancode(scancode,
                                           keyboard.key_pressed[LEFT_SHIFT] || 
                                           keyboard.key_pressed[RIGHT_SHIFT],
                                           keyboard.key_pressed[CAPS_LOCK]);
        
        if (c) {
            if (handle_special_keys(c, scancode)) {
                return;
            }
            
            /* Add regular character to buffer */
            if (keyboard_buffer_put(c)) {
                cb_publish(0, CB_KEY_PRESS, c);
                klogd("KEYBOARD: Key '%c' (scancode: %2x) processed\n", c, scancode);
            }
        } else {
            /* Handle non-printable keys (function keys, arrows, etc.) */
            cb_publish(0, CB_KEY_SPECIAL, scancode);
            klogd("KEYBOARD: Special key (scancode: %2x) processed\n", scancode);
        }
    }
}

/**
 * @brief Sends a command to the keyboard controller
 * 
 * @param command Command byte to send
 * @return TRUE if command was acknowledged, FALSE otherwise
 */
static uint8_t keyboard_send_command(uint8_t command) {
    uint32_t timeout = KB_TIMEOUT_CYCLES;
    
    /* Wait for input buffer to be empty */
    while ((inb(PS2_STATUS_REGISTER) & 0x02) && timeout--) {
        io_wait();
    }
    
    if (timeout == 0) {
        kloge("KEYBOARD: Timeout waiting for input buffer\n");
        return FALSE;
    }
    
    /* Send command */
    outb(PS2_DATA_PORT, command);
    
    /* Wait for response */
    timeout = KB_TIMEOUT_CYCLES;
    while (!(inb(PS2_STATUS_REGISTER) & 0x01) && timeout--) {
        io_wait();
    }
    
    if (timeout == 0) {
        kloge("KEYBOARD: Timeout waiting for command response\n");
        return FALSE;
    }
    
    uint8_t response = inb(PS2_DATA_PORT);
    return (response == PS2_ACK);
}

/**
 * @brief Flushes the PS/2 controller's output buffer
 */
static void keyboard_flush_buffer() {
    uint32_t timeout = KB_TIMEOUT_CYCLES;
    
    while ((inb(PS2_STATUS_REGISTER) & 0x01) && timeout--) {
        inb(PS2_DATA_PORT);
        io_wait();
        klogd("KEYBOARD: Flushing stale data\n");
    }
    
    if (timeout == 0) {
        klogw("KEYBOARD: Buffer flush timeout\n");
    }
}

/**
 * @brief Tests the PS/2 controller self-test
 * 
 * @return TRUE if self-test passed, FALSE otherwise
 */
static uint8_t keyboard_controller_test() {
    outb(PS2_COMMAND_REGISTER, 0xAA); /* Self-test command */
    
    uint32_t timeout = KB_TIMEOUT_CYCLES;
    while (!(inb(PS2_STATUS_REGISTER) & 0x01) && timeout--) {
        io_wait();
    }
    
    if (timeout == 0) {
        kloge("KEYBOARD: Controller self-test timeout\n");
        return FALSE;
    }
    
    uint8_t result = inb(PS2_DATA_PORT);
    if (result == 0x55) {
        klogd("KEYBOARD: Controller self-test passed\n");
        return TRUE;
    } else {
        kloge("KEYBOARD: Controller self-test failed (result: 0x%02X)\n", result);
        return FALSE;
    }
}

/**
 * @brief Initialization function for the keyboard
 * 
 * Initializes the PS/2 keyboard controller, sets up interrupt handling,
 * and prepares the keyboard buffer for use.
 * 
 * @return TRUE if initialization successful, FALSE otherwise
 */
uint8_t keyboard_init() {
    klogs("KEYBOARD: Initializing...\n");
    
    /* Initialize buffer state */
    memset((void*)&kb_buffer, 0, sizeof(kb_buffer));
    
    /* Disable interrupts during initialization */
    disable_interrupts();
    klogd("KEYBOARD: Interrupts disabled\n");
    
    /* Disable both PS/2 ports */
    outb(PS2_COMMAND_REGISTER, COMMAND_DISABLE_FIRST_PS2_PORT);
    outb(PS2_COMMAND_REGISTER, COMMAND_DISABLE_SECOND_PS2_PORT);
    klogd("KEYBOARD: PS/2 ports disabled\n");
    
    /* Flush any pending data */
    keyboard_flush_buffer();
    
    /* Test PS/2 controller */
    if (!keyboard_controller_test()) {
        kloge("KEYBOARD: Controller test failed, initialization aborted\n");
        enable_interrupts();
        return FALSE;
    }
    
    /* Enable first PS/2 port (keyboard) */
    outb(PS2_COMMAND_REGISTER, COMMAND_ENABLE_FIRST_PS2_PORT);
    klogd("KEYBOARD: First PS/2 port enabled\n");
    
    /* Reset keyboard and wait for acknowledgment */
    if (!keyboard_send_command(0xFF)) {
        klogw("KEYBOARD: Reset command failed, continuing anyway\n");
    }
    
    /* Register interrupt handler */
    irq_register_handler(1, keyboard_interrupt_handler);
    klogd("KEYBOARD: Interrupt handler registered\n");
    
    /* Enable keyboard interrupt */
    pic_unmask(1);
    klogd("KEYBOARD: Keyboard interrupt unmasked\n");
    
    /* Re-enable interrupts */
    enable_interrupts();
    klogd("KEYBOARD: Interrupts re-enabled\n");
    
    klogs("KEYBOARD: Initialization completed successfully\n");
    return TRUE;
}

/**
 * @brief Gets the current keyboard buffer status
 * 
 * @param buffer_used Pointer to store number of used buffer slots
 * @param overflow_occurred Pointer to store overflow flag
 */
void keyboard_get_buffer_status(uint8_t *buffer_used, uint8_t *overflow_occurred) {
    if (!buffer_used || !overflow_occurred) return;
    
    LOCK_LOCK(&kb_lock);
    *buffer_used = kb_buffer.buffer_length;
    *overflow_occurred = kb_buffer.overflow_occurred;
    UNLOCK_LOCK(&kb_lock);
}

/**
 * @brief Clears the keyboard buffer
 */
void keyboard_clear_buffer() {
    LOCK_LOCK(&kb_lock);
    kb_buffer.read_index = 0;
    kb_buffer.write_index = 0;
    kb_buffer.buffer_length = 0;
    kb_buffer.overflow_occurred = FALSE;
    UNLOCK_LOCK(&kb_lock);
    klogd("KEYBOARD: Buffer cleared\n");
}