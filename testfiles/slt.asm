	slt $t1, $zero, $zero

	addi $t2, $t2, 2
	addi $t3, $t3, -2

	slt $t1, $zero, $t2
	slt $t1, $t2, $zero
	slt $t1, $zero, $t3
	slt $t1, $t3, $zero
