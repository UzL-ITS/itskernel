; ITS kernel raw mutexes (used as base for higher-level locking).
; A raw mutex is represented by a single integer.
; If its value is
;    0, the mutex is acquired
;    1, the mutex is not acquired
;   <0, the mutex is acquired and a third thread tried to acquire it too

; IMPORTS
[extern sys_yield]

; Initializes a new raw mutex.
; Parameters:
;     - Pointer to mutex value.
[global raw_mutex_init]
raw_mutex_init:

	; Initial value is 1
	mov qword [rdi], 1
	ret

	
; Acquires the given raw mutex; if it is already acquired by another thread, this function blocks.
; Parameters:
;     - Pointer to mutex value.
[global raw_mutex_acquire]
raw_mutex_acquire:
	
	; Try to decrement mutex value (atomic)
	lock dec byte [rdi]
	
	; Value >= 0? -> Acquiring was successful
	jns raw_mutex_acquire_success
	
	; Acquiring failed, reset value
	lock inc byte [rdi]
	
	; This thread is blocked, ask scheduler to run another thread first (might the one we are waiting for)
	call sys_yield
	
	; Try again
	jmp raw_mutex_acquire

raw_mutex_acquire_success:

	; Acquiring successful
	ret


; Releases the given raw mutex.
; Parameters:
;     - Pointer to mutex value.
[global raw_mutex_release]
raw_mutex_release:
	
	; Release mutex by incrementing its value
	lock inc byte [rdi]
	
	; Done
	ret