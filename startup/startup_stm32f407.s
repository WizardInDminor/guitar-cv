.syntax unified
.cpu cortex-m4
.thumb

.section .isr_vector, "a"
.word _estack
.word Reset_Handler
.word Default_Handler
.word Default_Handler
.word Default_Handler
.word Default_Handler
.word Default_Handler
.word Default_Handler
.word Default_Handler
.word Default_Handler
.word Default_Handler
.word Default_Handler
.word Default_Handler
.word Default_Handler
.word Default_Handler
.word Default_Handler

.section .text

.global Reset_Handler
.type Reset_Handler, %function
.thumb_func
Reset_Handler:
    @ Enable FPU - set CP10 and CP11 full access
    ldr r0, =0xE000ED88
    ldr r1, [r0]
    orr r1, r1, #(0xF << 20)
    str r1, [r0]
    dsb
    isb
    ldr r0, =_sdata
    ldr r1, =_edata
    ldr r2, =_la_data

copy_data:
    cmp r0, r1
    bge zero_bss
    ldr r3, [r2], #4
    str r3, [r0], #4
    b copy_data

zero_bss:
    ldr r0, =_sbss
    ldr r1, =_ebss
    mov r2, #0

zero_loop:
    cmp r0, r1
    bge call_main
    str r2, [r0], #4
    b zero_loop

call_main:
    bl main
    b .

.global Default_Handler
.type Default_Handler, %function
.thumb_func
Default_Handler:
    b .
