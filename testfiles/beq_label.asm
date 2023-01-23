l1:		beq $zero, $zero, l2
l2:		beq $zero, $zero, l3
l3:		beq $zero, $zero, l4
l4:		beq $zero, $zero, l6
l5:		nop
l6:		addi $t1, $t1, -1
l7:		beq $t1, $zero, end
cool:		beq $t1, $t1, end
l8:		nop
l9:		nop
l10:	nop
l11:	nop
l12:	nop
end: 	beq $zero, $zero, end
