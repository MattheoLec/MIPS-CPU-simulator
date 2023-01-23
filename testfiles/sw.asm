	nor $t1, $t1, $t1
	sub $t2, $zero, $t1
	add $t2, $t2, $t2
	add $t2, $t2, $t2

	sw $t1, 0($zero)
	sw $t1, 4($zero)
	#sw $t2, -4($t2)



	lw $t3, 0($zero)
	lw $t4, 4($zero)
