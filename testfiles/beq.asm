	beq $zero, $zero, 0
	beq $zero, $zero, 0
	beq $zero, $zero, 0
	beq $zero, $zero, 1
	nop
	addi $t1, $t1, -1
	beq $t1, $zero, end
	nop
	nop
	nop
	nop
	nop
end: beq $zero, $zero, -2
